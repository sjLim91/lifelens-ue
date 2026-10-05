import {
  parseCognitiveProposal,
  type CognitiveProposalDto,
  type CognitiveRequestDto,
} from './cognitive-contract';
import type { CognitiveProvider } from './cognitive-provider';

export interface CognitionSchedulerConfig {
  maxConcurrent: number;
  maxQueued: number;
  timeoutMs: number;
  staleAfterSimulationMinutes: number;
}

interface QueueItem {
  sequence: number;
  request: CognitiveRequestDto;
  resolve: (proposal: CognitiveProposalDto | null) => void;
}

function validPositiveInteger(value: number): boolean {
  return Number.isInteger(value) && value > 0;
}

export class CognitionScheduler {
  private readonly queue: QueueItem[] = [];
  private readonly latestByActor = new Map<string, number>();
  private readonly activeControllers = new Set<AbortController>();
  private sequence = 0;
  private active = 0;
  private disposed = false;

  constructor(
    private readonly provider: CognitiveProvider,
    private readonly simulationMinute: () => number,
    private readonly config: CognitionSchedulerConfig,
  ) {
    if (!validPositiveInteger(config.maxConcurrent)) {
      throw new Error('cognition maxConcurrent must be a positive integer');
    }
    if (!Number.isInteger(config.maxQueued) || config.maxQueued < 0) {
      throw new Error('cognition maxQueued must be a non-negative integer');
    }
    if (!Number.isFinite(config.timeoutMs) || config.timeoutMs <= 0) {
      throw new Error('cognition timeoutMs must be positive');
    }
    if (
      !Number.isFinite(config.staleAfterSimulationMinutes)
      || config.staleAfterSimulationMinutes < 0
    ) {
      throw new Error('cognition stale window must be non-negative');
    }
  }

  submit(
    request: CognitiveRequestDto,
  ): Promise<CognitiveProposalDto | null> {
    if (this.disposed || request.allowedIntents.length === 0) {
      return Promise.resolve(null);
    }

    const canStart = this.active < this.config.maxConcurrent;
    if (!canStart && this.queue.length >= this.config.maxQueued) {
      return Promise.resolve(null);
    }

    const sequence = ++this.sequence;
    this.latestByActor.set(request.actor, sequence);

    return new Promise((resolve) => {
      this.queue.push({ sequence, request, resolve });
      this.pump();
    });
  }

  reset(): void {
    this.sequence += 1;
    this.latestByActor.clear();
    for (const controller of this.activeControllers) controller.abort();
    while (this.queue.length > 0) {
      this.queue.shift()?.resolve(null);
    }
  }

  dispose(): void {
    if (this.disposed) return;
    this.disposed = true;
    this.reset();
  }

  private pump(): void {
    while (
      !this.disposed
      && this.active < this.config.maxConcurrent
      && this.queue.length > 0
    ) {
      const item = this.queue.shift();
      if (!item) break;
      this.active += 1;
      void this.execute(item);
    }
  }

  private async execute(item: QueueItem): Promise<void> {
    const controller = new AbortController();
    this.activeControllers.add(controller);

    let timer: ReturnType<typeof setTimeout> | undefined;
    let onAbort: (() => void) | undefined;
    const abortedResult = new Promise<null>((resolve) => {
      onAbort = () => resolve(null);
      controller.signal.addEventListener('abort', onAbort, { once: true });
    });
    const providerResult = Promise.resolve()
      .then(() => this.provider.reason(item.request, controller.signal))
      .catch(() => null);

    const timeoutResult = new Promise<null>((resolve) => {
      timer = setTimeout(() => {
        controller.abort();
        resolve(null);
      }, this.config.timeoutMs);
    });

    let proposal: CognitiveProposalDto | null = null;
    try {
      proposal = await Promise.race([providerResult, timeoutResult, abortedResult]);

      if (this.disposed || controller.signal.aborted) {
        proposal = null;
      } else if (this.latestByActor.get(item.request.actor) !== item.sequence) {
        proposal = null;
      } else {
        const now = this.simulationMinute();
        if (
          !Number.isFinite(now)
          || now < item.request.minute
          || now - item.request.minute
            > this.config.staleAfterSimulationMinutes
        ) {
          proposal = null;
        }
      }

      if (proposal !== null) {
        try {
          proposal = parseCognitiveProposal(proposal, item.request);
        } catch {
          proposal = null;
        }
      }

      item.resolve(proposal);
    } catch {
      // A failed context reader must settle the caller just like provider
      // failure, and must not leak an unhandled execute() rejection.
      item.resolve(null);
    } finally {
      if (timer !== undefined) clearTimeout(timer);
      if (onAbort) controller.signal.removeEventListener('abort', onAbort);
      this.activeControllers.delete(controller);
      if (this.latestByActor.get(item.request.actor) === item.sequence) {
        this.latestByActor.delete(item.request.actor);
      }
      this.active = Math.max(0, this.active - 1);
      this.pump();
    }
  }
}
