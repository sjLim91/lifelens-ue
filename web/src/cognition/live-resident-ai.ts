import type {
  CognitiveIntent, CognitiveProposalDto, CognitiveRequestDto, CognitiveTrigger,
} from './cognitive-contract';
import { CognitionScheduler } from './cognition-scheduler';
import type { CognitiveProvider } from './cognitive-provider';
import { LocalCognitionProvider, isLoopbackCognitionUrl, discoverAutomaticLocalCognition } from './local-cognition-provider';
import { ResidentAISessionAudit } from './resident-ai-session-audit';
import { createBrowserResidentCognition, BUILTIN_RESIDENT_MODEL } from './browser-cognition-provider';
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

// On-device GPU inference may need several seconds to compile kernels and
// generate JSON, especially on Android. A fixed 6-second network budget
// would cancel nearly every legitimate resident thought. This is still ONE
// resident cognition stream, not a second planner or a separate AI mode.
export function residentAIExecutionBudget(provider: CognitiveProvider) {
  return provider.name === 'resident-browser-webgpu'
    ? { ...LIVE_AI_BUDGET, timeoutMs: 24000 }
    : LIVE_AI_BUDGET;
}

export interface ResidentAISource {
  worldOverview(): WorldOverview;
  cognitiveRequest(actor: string, trigger: 'Reflection' | 'ResourceScarcity' | 'RepeatedFailure'): CognitiveRequestDto | null;
  submitLocalCognitiveProposal(
    request: CognitiveRequestDto,
    proposal: CognitiveProposalDto,
    observedWorldSeed: string,
  ): boolean;
}

// Advertise only strategies that have a real, bounded Core execution lane.
// Other intents can still exist in the read-only Shadow system, but the live
// model must not waste scarce local inference on journal-only promises.
const EXECUTABLE_RESIDENT_AI_INTENTS: ReadonlySet<CognitiveIntent> = new Set([
  'ImproveFoodSecurity',
  'ImproveWaterSecurity',
  'AcquireMaterials',
  'CraftUsefulTools',
  'ExpandCultivation',
  'ExploreOpportunity',
  'CooperateWithResident',
  'ResolveConflict',
]);

export function liveActionableCognitiveRequest(
  request: CognitiveRequestDto,
): CognitiveRequestDto {
  return {
    ...request,
    allowedIntents: request.allowedIntents.filter(
      (intent) => EXECUTABLE_RESIDENT_AI_INTENTS.has(intent),
    ),
  };
}

// An autonomous resident's ONE mind must receive a distinct reflection event
// when its own attempted strategies repeatedly fail. Only actual firsthand
// Core outcome memories can trigger it; narrated or inferred failure cannot.
// Source exhaustion/removal remains a higher-priority factual scarcity event.
export function residentAIObservedTrigger(
  request: CognitiveRequestDto,
): Extract<CognitiveTrigger, 'Reflection' | 'ResourceScarcity' | 'RepeatedFailure'> {
  const outcomes = new Map<string, Map<number, 'success' | 'failure'>>();
  const latestLost = new Map<string, { minute: number; lost: boolean }>();
  for (const memory of request.memories) {
    if (memory.who !== request.actor
      || memory.source !== 'DirectWitness' || memory.witnessed !== true
      || memory.minute < 0 || memory.minute > request.minute
      || request.minute - memory.minute > 1440
      || memory.tags?.length !== 3
      || memory.tags[0] !== 'action_experience'
      || !memory.tags[1].startsWith('Civilization:')
      || (memory.tags[2] !== 'failure' && memory.tags[2] !== 'success')) continue;

    const key = memory.tags[1];
    const kind = memory.tags[2];
    let byMinute = outcomes.get(key);
    if (!byMinute) {
      byMinute = new Map();
      outcomes.set(key, byMinute);
    }
    // Same-minute success wins independent of memory iteration order.
    if (kind === 'success' || byMinute.get(memory.minute) !== 'success')
      byMinute.set(memory.minute, kind);

    if (key !== 'Civilization:Gather:PlantFood'
      && key !== 'Civilization:Gather:Water') continue;
    const prefix = '실제 문명 행동 ' + key +
      ' 수행을 시도했지만 실패했다 (직접 확인한 원인: ';
    const lost = kind === 'failure'
      && (memory.what === prefix + 'source-exhausted)'
        || memory.what === prefix + 'source-removed)');
    const prior = latestLost.get(key);
    if (!prior || memory.minute > prior.minute
      || (memory.minute === prior.minute && (!lost || prior.lost))) {
      latestLost.set(key, { minute: memory.minute, lost });
    }
  }

  if ([...latestLost.values()].some(item => item.lost))
    return 'ResourceScarcity';

  for (const byMinute of outcomes.values()) {
    let consecutiveFailures = 0;
    for (const [_, outcome] of [...byMinute.entries()].sort((a,b)=>a[0]-b[0])) {
      consecutiveFailures = outcome === 'success' ? 0 : consecutiveFailures + 1;
    }
    if (consecutiveFailures >= 2) return 'RepeatedFailure';
  }
  return 'Reflection';
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
  automatic: boolean;
  discovering: boolean;
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
  private readonly sessionAudit = new ResidentAISessionAudit();
  private observedWorld: WorldOverview | null = null;
  private observedResidents: readonly Resident[] = [];
  private state: ResidentAIState = {
    enabled: false, automatic: false, discovering: false,
    suspended: false, pendingActor: null,
    configuration: null, accepted: 0, rejected: 0,
    lastDecision: null, byResident: {},
  };
  private source: ResidentAISource | null = null;
  private scheduler: CognitionScheduler | null = null;
  private epoch = 0;
  private cursor = 0;
  private lastSelectedActor: string | null = null;
  private lastCall = -Infinity;
  private identity: string | null = null;
  private lastMinute = -1;
  private disposed = false;
  private automaticallyRunning = false;
  private browserModelAttempted = false;
  private browserModelLoading = false;
  private lastAutoDiscovery = -Infinity;
  private autoDiscoveryAbort: AbortController | null = null;
  private autoDiscoveryGeneration = 0;

  constructor(private readonly now: () => number = () => Date.now()) {}
  getSnapshot = (): ResidentAIState => this.state;
  getAuditReport = () => this.sessionAudit.snapshot();
  beginAuditSession(): boolean {
    if (this.disposed || this.state.suspended || !this.observedWorld) return false;
    const mode = this.state.enabled && this.scheduler ? 'local-ai' : 'baseline';
    if (!this.sessionAudit.begin(this.observedWorld, mode,
      this.state.configuration?.model ?? null)) return false;
    this.sessionAudit.observe(this.observedWorld, this.observedResidents, mode);
    this.update({});
    return true;
  }
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
    this.autoDiscoveryAbort?.abort();
    this.autoDiscoveryAbort = null;
    this.autoDiscoveryGeneration += 1;
    this.browserModelAttempted = false;
    this.browserModelLoading = false;
    this.resetWorld();
    this.source = source;
    this.rebuildScheduler();
    if (this.automaticallyRunning) void this.discoverAutomatically();
  }
  // Production LifeLens always starts ONE resident cognition stream when the
  // Core becomes available. No operator-facing on/off switch or model setup.
  // An absent native local service is an unavailable inference accelerator,
  // NOT an absent person; the same resident continues embodied Core agency.
  startAutomatically(): void {
    if (this.disposed || !this.source) return;
    this.automaticallyRunning = true;
    this.update({ enabled: true, automatic: true });
    void this.discoverAutomatically();
  }
  private async discoverAutomatically(): Promise<void> {
    if (!this.automaticallyRunning || this.disposed || !this.source
      || this.autoDiscoveryAbort || this.scheduler) return;
    const controller = new AbortController();
    this.autoDiscoveryAbort = controller;
    const generation = ++this.autoDiscoveryGeneration;
    const source = this.source;
    this.lastAutoDiscovery = this.now();
    this.update({ discovering: true });
    // Bounded wait: browsers may silently block a localhost model endpoint.
    const timeout = setTimeout(() => controller.abort(), 2500);
    try {
      const config = await discoverAutomaticLocalCognition(controller.signal);
      if (!controller.signal.aborted && config
        && !this.disposed && this.automaticallyRunning
        && this.source === source && this.autoDiscoveryGeneration === generation) {
        this.configure(config.baseUrl, config.model);
      }
    } catch {
      // Missing local inference must not stop the world or fabricate a model.
    } finally {
      clearTimeout(timeout);
      if (this.autoDiscoveryAbort === controller) {
        this.autoDiscoveryAbort = null;
        this.update({ discovering: false });
        // No native model server? Load the free language engine IN the
        // browser itself when hardware supports WebGPU. One resident agency
        // owns the resulting single provider; no user click or toggle.
        if (!this.scheduler) void this.prepareBrowserReasoning();
      }
    }
  }
  private async prepareBrowserReasoning(): Promise<void> {
    if (!this.automaticallyRunning || this.disposed || !this.source
      || this.scheduler || this.browserModelAttempted) return;
    this.browserModelAttempted = true;
    this.browserModelLoading = true;
    const source = this.source;
    this.update({ discovering: true });
    try {
      const provider = await createBrowserResidentCognition();
      if (provider && !this.disposed && this.automaticallyRunning
        && this.source === source && !this.scheduler) {
        this.setProvider(provider);
        this.update({ configuration: {
          baseUrl: 'browser-webgpu', model: BUILTIN_RESIDENT_MODEL,
        } });
      }
    } catch {
      // Hardware/weight failures cannot stop physical resident agency.
    } finally {
      this.browserModelLoading = false;
      if (!this.autoDiscoveryAbort)
        this.update({ discovering: false });
    }
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
        () => this.source?.worldOverview().minute ?? NaN,
        residentAIExecutionBudget(provider))
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
    if (enabled !== this.state.enabled) {
      this.sessionAudit.finish(enabled ? 'mode-changed' : 'stopped');
    }
    if (!enabled) this.resetWorld();
    this.update({ enabled });
    return true;
  }
  suspend(suspended: boolean): void {
    if (suspended) {
      this.sessionAudit.finish('fast-forward');
      this.resetWorld();
    }
    this.update({ suspended });
  }
  resetWorld(): void {
    this.sessionAudit.finish('world-changed');
    this.observedWorld = null;
    this.observedResidents = [];
    this.epoch += 1;
    this.scheduler?.reset();
    this.identity = null;
    this.lastMinute = -1;
    this.cursor = 0;
    this.lastSelectedActor = null;
    this.residentHistory.clear();
    this.update({
      pendingActor: null, accepted: 0, rejected: 0,
      lastDecision: null, byResident: {},
    });
  }
  dispose(): void {
    this.disposed = true;
    this.automaticallyRunning = false;
    this.browserModelAttempted = false;
    this.autoDiscoveryAbort?.abort();
    this.autoDiscoveryAbort = null;
    this.autoDiscoveryGeneration += 1;
    this.resetWorld();
    this.scheduler?.dispose();
    this.scheduler = null;
    this.source = null;
    this.update({ enabled: false, automatic: false, discovering: false, configuration: null });
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
    this.observedWorld = world;
    this.observedResidents = residents;
    this.sessionAudit.observe(world, residents,
      this.state.enabled && this.scheduler ? 'local-ai' : 'baseline');
    if (this.automaticallyRunning && !this.scheduler && !this.autoDiscoveryAbort
      && !this.browserModelLoading
      && this.now() - this.lastAutoDiscovery >= 60000)
      void this.discoverAutomatically();
    if (!this.state.enabled || this.state.suspended || !this.scheduler
      || !this.source || this.state.pendingActor
      || this.now() - this.lastCall < LIVE_AI_BUDGET.wallCadenceMs) return;

    // Rotate across every living resident who can independently decide.
    // Core refuses suggestions from Babies and Toddlers (requiresDirectCare);
    // do not spend scarce local-model inference on guaranteed rejections.
    // Missing lifeStage remains eligible for older save/observer contracts;
    // Core still performs the final authoritative age and intent checks.
    const candidates = residents.filter((resident) => resident.alive === true
      && resident.lifeStage !== 'Baby'
      && resident.lifeStage !== 'Toddler');
    if (candidates.length === 0) return;
    // Anchor the next turn to the previous ACTOR, not the index of a
    // changing eligible subset. When infants mature or residents die, an
    // index-only cursor can select the same resident twice in succession.
    const previousIndex = candidates.findIndex(
      (candidate) => candidate.id === this.lastSelectedActor);
    const nextIndex = previousIndex >= 0
      ? (previousIndex + 1) % candidates.length
      : this.cursor % candidates.length;
    const resident = candidates[nextIndex];
    this.cursor += 1;
    if (resident) {
      this.lastSelectedActor = resident.id;
      void this.sample(resident.id);
    }
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
      let observed = source.cognitiveRequest(actor, 'Reflection');
      if (observed !== null) {
        const trigger = residentAIObservedTrigger(observed);
        if (trigger !== 'Reflection') {
          const context = source.cognitiveRequest(actor, trigger);
          // Core must re-authorize the same resident, simulation minute and
          // factual trigger. A model never chooses its own retrospective proof.
          if (context && context.actor === observed.actor
            && context.minute === observed.minute
            && context.trigger === trigger) observed = context;
        }
      }
      request = observed === null ? null : liveActionableCognitiveRequest(observed);
    } catch {
      // The Core may be resetting while the observer refreshes. A failed
      // source read must NOT become an unhandled rejected Promise on the
      // renderer's fire-and-forget loop, or submit a fabricated request.
      this.lastCall = this.now();
      const lastDecision: ResidentAIDecision = {
        actor, minute: -1, status: 'error', intent: null, target: null,
      };
      this.sessionAudit.result('error');
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
      || request.minute !== world.minute
      || request.allowedIntents.length === 0) return;

    const epoch = this.epoch;
    this.sessionAudit.inference(request);
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
    this.sessionAudit.result(status);
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
