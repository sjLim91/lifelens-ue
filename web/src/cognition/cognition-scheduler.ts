import {
  parseCognitiveProposal,
  type CognitiveProposalDto,
  type CognitiveRequestDto,
} from './cognitive-contract';
import { CognitiveProviderError, type CognitiveProvider } from './cognitive-provider';

export type CognitionStatus = 'validated' | 'rejected' | 'timeout' | 'error' | 'cancelled' | 'stale' | 'busy' | 'unavailable';
export type CognitionReason = 'invalid_json' | 'invalid_schema' | 'http_error' | 'malformed_response' | 'provider_error' | 'no_proposal' | 'context_error' | 'target_unavailable' | CognitionStatus;
export interface CognitionOutcome {
  status: CognitionStatus;
  reason: CognitionReason;
  proposal: CognitiveProposalDto | null;
}
const outcome = (status: CognitionStatus, reason: CognitionReason = status): CognitionOutcome => ({ status, reason, proposal: null });

export interface CognitionSchedulerConfig {
  maxConcurrent: number;
  maxQueued: number;
  timeoutMs: number;
  staleAfterSimulationMinutes: number;
}

interface QueueItem {
  sequence: number;
  request: CognitiveRequestDto;
  resolve: (result: CognitionOutcome) => void;
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

  submit(request: CognitiveRequestDto): Promise<CognitiveProposalDto | null> {
    return this.submitObserved(request).then((result) => result.proposal);
  }

  submitObserved(request: CognitiveRequestDto): Promise<CognitionOutcome> {
    if (this.disposed) return Promise.resolve(outcome('cancelled'));
    if (request.allowedIntents.length === 0) return Promise.resolve(outcome('rejected', 'invalid_schema'));
    const canStart = this.active < this.config.maxConcurrent;
    if (!canStart && this.queue.length >= this.config.maxQueued) {
      return Promise.resolve(outcome('busy'));
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
      this.queue.shift()?.resolve(outcome('cancelled'));
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
    let timedOut = false;
    const abortedResult = new Promise<CognitionOutcome>((resolve) => {
      onAbort = () => resolve(outcome(timedOut ? 'timeout' : 'cancelled'));
      controller.signal.addEventListener('abort', onAbort, { once: true });
    });
    const providerResult = Promise.resolve()
      .then(() => this.provider.reason(item.request, controller.signal))
      .then((proposal): CognitionOutcome => proposal === null
        ? outcome('unavailable', 'no_proposal')
        : { status: 'validated', reason: 'validated', proposal })
      .catch((error: unknown): CognitionOutcome => error instanceof CognitiveProviderError
        ? outcome(error.reason === 'invalid_json' || error.reason === 'invalid_schema' ? 'rejected' : 'error', error.reason)
        : outcome('error', 'provider_error'));
    const timeoutResult = new Promise<CognitionOutcome>((resolve) => {
      timer = setTimeout(() => {
        timedOut = true;
        controller.abort();
        resolve(outcome('timeout'));
      }, this.config.timeoutMs);
    });
    try {
      let result = await Promise.race([providerResult, timeoutResult, abortedResult]);
      if (this.disposed || controller.signal.aborted) {
        result = outcome(timedOut ? 'timeout' : 'cancelled');
      } else if (this.latestByActor.get(item.request.actor) !== item.sequence) {
        result = outcome('stale');
      } else {
        const now = this.simulationMinute();
        if (!Number.isFinite(now) || now < item.request.minute
          || now - item.request.minute > this.config.staleAfterSimulationMinutes) {
          result = outcome('stale');
        }
      }
      if (result.proposal !== null) {
        try {
          result = { ...result, proposal: parseCognitiveProposal(result.proposal, item.request) };
        } catch {
          result = outcome('rejected', 'invalid_schema');
        }
      }
      item.resolve(result);
    } catch {
      item.resolve(outcome('error', 'context_error'));
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
