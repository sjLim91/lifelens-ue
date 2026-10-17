import type { CognitiveProposalDto, CognitiveRequestDto } from './cognitive-contract';
import { CognitionScheduler } from './cognition-scheduler';
import type { CognitiveProvider } from './cognitive-provider';
import { LocalCognitionProvider, isLoopbackCognitionUrl } from './local-cognition-provider';
import type { Resident, WorldOverview } from '../runtime/core-types';

// One shared free-local model. Each resident gets a separate, bounded
// Core-authored personality/Need/memory/belief/relationship context.
export const LIVE_AI_BUDGET = Object.freeze({
  maxConcurrent: 1,
  maxQueued: 0,
  timeoutMs: 6000,
  staleAfterSimulationMinutes: 96,
  wallCadenceMs: 5000,
  maxResidents: 32,
});

export interface ResidentAISource {
  worldOverview(): WorldOverview;
  cognitiveRequest(actor: string, trigger: 'Reflection'): CognitiveRequestDto | null;
  submitLocalCognitiveProposal(
    request: CognitiveRequestDto,
    proposal: CognitiveProposalDto,
    observedWorldSeed: string,
  ): boolean;
}

export type ResidentAIDecisionStatus =
  'recorded' | 'stale' | 'rejected' | 'unavailable' | 'error';

export interface ResidentAIPersonalExperience {
  minute: number;
  what: string;
  kind: 'action' | 'social';
}

// This is the Core-provided evidence the local model *saw*, not a model
// claim about executing or completing any goal.
export function latestFirsthandAIExperience(
  request: CognitiveRequestDto,
): ResidentAIPersonalExperience | null {
  let latest: ResidentAIPersonalExperience | null = null;
  for(const memory of request.memories){
    if(memory.source !== 'DirectWitness' || memory.witnessed !== true
      ||memory.minute < 0 ||memory.minute > request.minute
      ||request.minute-memory.minute > 1440
      ||!Array.isArray(memory.tags)) continue;
    const kind = memory.tags[0];
    if(kind !== 'action_experience'
      &&kind !== 'goal_action_feedback'
      &&kind !== 'social_self_outcome') continue;
    if(latest === null || memory.minute > latest.minute){
      latest = {
        minute: memory.minute,
        what: memory.what.slice(0, 160),
        kind: kind === 'action_experience' ? 'action' : 'social',
      };
    }
  }
  return latest;
}

export interface ResidentAIDecision {
  actor: string;
  minute: number;
  status: ResidentAIDecisionStatus;
  intent: string | null;
  target: string | null;
  latestPersonalExperience?: ResidentAIPersonalExperience | null;
}

export interface ResidentAIState {
  enabled: boolean;
  suspended: boolean;
  pendingActor: string | null;
  configuration: { baseUrl: string; model: string } | null;
  accepted: number;
  rejected: number;
  lastDecision: ResidentAIDecision | null;
  byResident: Readonly<Record<string, ResidentAIDecision>>;
}

function worldIdentity(world: WorldOverview): string {
  return JSON.stringify([
    world.worldSeed, world.populationSeed, world.generationVersion,
  ]);
}

export class ResidentAIController {
  private readonly listeners = new Set<() => void>();
  // Keep only a bounded visible history; inference fairness covers ALL living
  // residents, not permanently the first N founders.
  private readonly residentHistory = new Map<string, ResidentAIDecision>();
  private state: ResidentAIState = {
    enabled: false, suspended: false, pendingActor: null,
    configuration: null, accepted: 0, rejected: 0,
    lastDecision: null, byResident: {},
  };
  private source: ResidentAISource | null = null;
  private scheduler: CognitionScheduler | null = null;
  private epoch = 0;
  private cursor = 0;
  private lastCall = -Infinity;
  private identity: string | null = null;
  private lastMinute = -1;
  private disposed = false;

  constructor(private readonly now: () => number = () => Date.now()) {}
  getSnapshot = (): ResidentAIState => this.state;
  subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  };
  private latestResidentDecisions(decision: ResidentAIDecision): Readonly<Record<string, ResidentAIDecision>> {
    this.residentHistory.delete(decision.actor);
    this.residentHistory.set(decision.actor, decision);
    while (this.residentHistory.size > LIVE_AI_BUDGET.maxResidents) {
      const oldest = this.residentHistory.keys().next().value;
      if (oldest === undefined) break;
      this.residentHistory.delete(oldest);
    }
    return Object.fromEntries(this.residentHistory);
  }
  private update(delta: Partial<ResidentAIState>): void {
    this.state = { ...this.state, ...delta };
    for (const listener of this.listeners) listener();
  }
  attach(source: ResidentAISource): void {
    this.disposed = false;
    this.resetWorld();
    this.source = source;
    this.rebuildScheduler();
  }
  configure(baseUrl: string, model: string): boolean {
    if (!isLoopbackCognitionUrl(baseUrl)
      || !model.trim() || model.length > 200) return false;
    this.update({ configuration: { baseUrl, model: model.trim() } });
    this.rebuildScheduler();
    return true;
  }
  // Deterministic local fakes are injected only by tests; the production
  // configuration path is restricted to loopback/local inference servers.
  setProvider(provider: CognitiveProvider): void {
    this.scheduler?.dispose();
    this.resetWorld();
    this.scheduler = this.source && !this.disposed
      ? new CognitionScheduler(provider,
        () => this.source?.worldOverview().minute ?? NaN, LIVE_AI_BUDGET)
      : null;
  }
  private rebuildScheduler(): void {
    this.scheduler?.dispose();
    const config = this.state.configuration;
    this.scheduler = config && this.source && !this.disposed
      ? new CognitionScheduler(
        new LocalCognitionProvider(config),
        () => this.source?.worldOverview().minute ?? NaN,
        LIVE_AI_BUDGET,
      )
      : null;
  }
  setEnabled(enabled: boolean): boolean {
    if (enabled && (!this.scheduler || !this.source || this.disposed)) return false;
    if (!enabled) this.resetWorld();
    this.update({ enabled });
    return true;
  }
  suspend(suspended: boolean): void {
    if (suspended) this.resetWorld();
    this.update({ suspended });
  }
  resetWorld(): void {
    this.epoch += 1;
    this.scheduler?.reset();
    this.identity = null;
    this.lastMinute = -1;
    this.cursor = 0;
    this.residentHistory.clear();
    this.update({
      pendingActor: null, accepted: 0, rejected: 0,
      lastDecision: null, byResident: {},
    });
  }
  dispose(): void {
    this.disposed = true;
    this.resetWorld();
    this.scheduler?.dispose();
    this.scheduler = null;
    this.source = null;
    this.update({ enabled: false, configuration: null });
  }
  // Called from the existing 500ms observer refresh, never the render loop.
  observe(world: WorldOverview, residents: readonly Resident[]): void {
    if (this.disposed) return;
    const identity = worldIdentity(world);
    const minute = Number(world.minute);
    if (this.identity !== null
      && (identity !== this.identity || minute < this.lastMinute)) {
      this.resetWorld();
    }
    this.identity = identity;
    this.lastMinute = minute;
    if (!this.state.enabled || this.state.suspended || !this.scheduler
      || !this.source || this.state.pendingActor
      || this.now() - this.lastCall < LIVE_AI_BUDGET.wallCadenceMs) return;

    // Rotate across the entire living population. The model concurrency
    // remains 1 and displayed history is bounded separately to 32 actors.
    const candidates = residents
      .filter((resident) => resident.alive === true);
    if (candidates.length === 0) return;
    const resident = candidates[this.cursor % candidates.length];
    this.cursor += 1;
    if (resident) void this.sample(resident.id);
  }
  async sample(actor: string): Promise<void> {
    if (!this.state.enabled || this.state.suspended
      || this.disposed || !this.source || !this.scheduler
      || this.state.pendingActor
      || this.now() - this.lastCall < LIVE_AI_BUDGET.wallCadenceMs) return;
    const source = this.source;
    let world: WorldOverview;
    let request: CognitiveRequestDto | null;
    try {
      world = source.worldOverview();
      request = source.cognitiveRequest(actor, 'Reflection');
    } catch {
      // The Core may be resetting while the observer refreshes. A failed
      // source read must NOT become an unhandled rejected Promise on the
      // renderer's fire-and-forget loop, or submit a fabricated request.
      this.lastCall = this.now();
      const lastDecision: ResidentAIDecision = {
        actor, minute: -1, status: 'error', intent: null, target: null,
      };
      this.update({
        pendingActor: null, rejected: this.state.rejected + 1,
        lastDecision,
        byResident: this.latestResidentDecisions(lastDecision),
      });
      return;
    }
    const initialIdentity = worldIdentity(world);
    this.lastCall = this.now();
    if (!request || request.actor !== actor
      || request.minute !== world.minute) return;

    const epoch = this.epoch;
    this.update({ pendingActor: actor });
    const result = await this.scheduler.submitObserved(request);
    if (this.disposed || epoch !== this.epoch || this.source !== source) return;

    let status: ResidentAIDecisionStatus = result.status === 'validated'
      ? 'rejected'
      : result.status === 'rejected' ? 'rejected'
      : result.status === 'stale' || result.status === 'cancelled'
        ? 'stale'
        : result.status === 'error' || result.status === 'timeout'
          ? 'error'
          : 'unavailable';

    try {
      const latest = source.worldOverview();
      const latestMinute = Number(latest.minute);
      if (worldIdentity(latest) !== initialIdentity
        || !Number.isFinite(latestMinute)
        || latestMinute < request.minute
        || latestMinute - request.minute
          > LIVE_AI_BUDGET.staleAfterSimulationMinutes) {
        status = 'stale';
      } else if (result.proposal !== null && result.status === 'validated') {
        status = source.submitLocalCognitiveProposal(
          request, result.proposal, String(world.worldSeed),
        ) ? 'recorded' : 'rejected';
      }
    } catch {
      status = 'error';
    }

    const lastDecision: ResidentAIDecision = {
      actor, minute: request.minute, status,
      intent: result.proposal?.intent ?? null,
      target: result.proposal?.targetResident ?? null,
      latestPersonalExperience: latestFirsthandAIExperience(request),
    };
    this.update({
      pendingActor: null,
      accepted: this.state.accepted + (status === 'recorded' ? 1 : 0),
      rejected: this.state.rejected + (status === 'recorded' ? 0 : 1),
      lastDecision,
      byResident: this.latestResidentDecisions(lastDecision),
    });
  }
}

export const residentAI = new ResidentAIController();
