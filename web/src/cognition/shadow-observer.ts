import type { CognitiveRequestDto } from './cognitive-contract';
import type { CognitiveProvider } from './cognitive-provider';
import { CognitionScheduler, type CognitionOutcome } from './cognition-scheduler';
import { LocalCognitionProvider, isLoopbackCognitionUrl } from './local-cognition-provider';
import type { Resident, ResidentsPayload, WorldOverview } from '../runtime/core-types';

// This capability intentionally contains no Core mutation, save, or simulation API.
export interface ShadowSource {
  worldOverview(): WorldOverview;
  residentRuntime(): ResidentsPayload;
  cognitiveRequest(actor: string, trigger: 'Reflection'): CognitiveRequestDto | null;
}
export const SHADOW_BUDGET = Object.freeze({
  maxConcurrent: 1, maxQueued: 0, timeoutMs: 6000,
  staleAfterSimulationMinutes: 96, cadenceMs: 15000, cadenceMinutes: 30,
  maxResidents: 32, historyPerResident: 3,
});
export interface ShadowRecord {
  id: number;
  actor: string;
  name: string;
  minute: number;
  actual: Pick<Resident, 'activityKind' | 'activityLabel' | 'physicalGoal' | 'socialIntent' | 'presentation'>;
  context: CognitiveRequestDto;
  provider: string;
  outcome: CognitionOutcome;
}
export interface ShadowSnapshot {
  enabled: boolean;
  automatic: boolean;
  suspended: boolean;
  pendingActor: string | null;
  status: 'disabled' | 'ready' | 'pending' | 'configuration_error' | 'context_unavailable' | 'stale';
  records: readonly ShadowRecord[];
  configuration: { baseUrl: string; model: string } | null;
}
const identity = (world: WorldOverview): string => JSON.stringify([
  world.worldSeed, world.populationSeed, world.generationVersion,
]);
const living = (resident: Resident): boolean => resident.alive === true;

export class ShadowObserver {
  private state: ShadowSnapshot = {
    enabled: false, automatic: false, suspended: false,
    pendingActor: null, status: 'disabled', records: [], configuration: null,
  };
  private readonly listeners = new Set<() => void>();
  private source: ShadowSource | null = null;
  private scheduler: CognitionScheduler | null = null;
  private provider: CognitiveProvider | null = null;
  private epoch = 0;
  private recordId = 0;
  private worldIdentity: string | null = null;
  private observedMinute = -1;
  private lastCall = -Infinity;
  private lastSampleMinute = -Infinity;
  private disposed = false;

  constructor(
    private readonly now: () => number = () => performance.now(),
    private readonly timeoutMs = SHADOW_BUDGET.timeoutMs,
  ) {}
  getSnapshot = (): ShadowSnapshot => this.state;
  subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    return () => { this.listeners.delete(listener); };
  };
  private update(patch: Partial<ShadowSnapshot>): void {
    this.state = { ...this.state, ...patch };
    for (const listener of this.listeners) listener();
  }
  attach(source: ShadowSource): void {
    this.resetWorld();
    this.disposed = false;
    this.source = source;
    this.replaceScheduler();
  }
  configure(baseUrl: string, model: string): boolean {
    if (!isLoopbackCognitionUrl(baseUrl) || !model.trim() || model.length > 200) {
      this.setEnabled(false);
      this.update({ status: 'configuration_error' });
      return false;
    }
    this.setProvider(new LocalCognitionProvider({ baseUrl, model: model.trim() }));
    this.update({ configuration: { baseUrl, model: model.trim() } });
    return true;
  }
  // Uses the same free-local abstraction; injection also permits deterministic tests.
  setProvider(provider: CognitiveProvider): void {
    this.resetWorld();
    this.provider = provider;
    this.replaceScheduler();
  }
  private replaceScheduler(): void {
    this.scheduler?.dispose();
    this.scheduler = this.source && this.provider && !this.disposed
      ? new CognitionScheduler(this.provider, () => this.source?.worldOverview().minute ?? NaN,
        { ...SHADOW_BUDGET, timeoutMs: this.timeoutMs })
      : null;
  }
  setEnabled(enabled: boolean): void {
    if (enabled && (!this.scheduler || this.disposed)) {
      this.update({ status: 'configuration_error' });
      return;
    }
    if (!enabled) this.resetWorld();
    this.update({ enabled, status: enabled ? 'ready' : 'disabled' });
  }
  setAutomatic(automatic: boolean): void { this.update({ automatic }); }
  suspend(suspended: boolean): void {
    if (suspended) this.resetWorld();
    this.update({ suspended });
  }
  resetWorld(): void {
    this.epoch += 1;
    this.scheduler?.reset();
    this.worldIdentity = null;
    this.observedMinute = -1;
    this.lastSampleMinute = -Infinity;
    // Keep the wall-time budget across resets, toggles, and provider changes.
    this.update({ records: [], pendingActor: null, status: this.state.enabled ? 'ready' : 'disabled' });
  }
  dispose(): void {
    this.disposed = true;
    this.resetWorld();
    this.scheduler?.dispose();
    this.scheduler = null;
    this.source = null;
    this.provider = null;
    this.update({ enabled: false, automatic: false, status: 'disabled', configuration: null });
  }
  // Called by the existing Core refresh cadence, never by the renderer frame loop.
  observe(world: WorldOverview, residents: readonly Resident[], selected: string | null): void {
    if (this.disposed) return;
    const minute = world.minute ?? NaN;
    const key = identity(world);
    if (this.worldIdentity !== null && (this.worldIdentity !== key || minute < this.observedMinute)) {
      this.resetWorld();
    }
    this.worldIdentity = key;
    this.observedMinute = minute;
    if (!this.state.enabled && this.state.records.length === 0 && !this.state.pendingActor) return;
    const liveIds = new Set(residents.filter(living).map((resident) => resident.id));
    const records = this.state.records.filter((record) => liveIds.has(record.actor)
      && (!record.outcome.proposal?.targetResident || liveIds.has(record.outcome.proposal.targetResident))
      && Number.isFinite(minute) && minute >= record.minute
      && minute - record.minute <= SHADOW_BUDGET.staleAfterSimulationMinutes);
    if (records.length !== this.state.records.length) this.update({ records });
    if (this.state.pendingActor && !liveIds.has(this.state.pendingActor)) {
      this.epoch += 1;
      this.scheduler?.reset();
      this.update({ pendingActor: null, status: 'ready' });
    }
    if (this.state.automatic && selected && liveIds.has(selected) && minute - this.lastSampleMinute >= SHADOW_BUDGET.cadenceMinutes) {
      void this.sample(selected);
    }
  }
  async sample(actor: string): Promise<void> {
    if (!this.state.enabled || this.state.suspended || this.disposed
      || !this.source || !this.scheduler || this.state.pendingActor
      || this.now() - this.lastCall < SHADOW_BUDGET.cadenceMs) return;
    this.lastCall = this.now();
    const source = this.source;
    let request: CognitiveRequestDto | null;
    let resident: Resident | undefined;
    let world: WorldOverview;
    try {
      world = source.worldOverview();
      resident = source.residentRuntime().residents?.find((entry) => entry.id === actor && living(entry));
      request = resident ? source.cognitiveRequest(actor, 'Reflection') : null;
    } catch {
      this.update({ status: 'context_unavailable' });
      return;
    }
    if (!request || !resident || request.actor !== actor || request.minute !== world.minute) {
      this.update({ status: 'context_unavailable' });
      return;
    }
    const key = identity(world);
    if (this.worldIdentity !== null && this.worldIdentity !== key) this.resetWorld();
    this.worldIdentity = key;
    this.observedMinute = request.minute;
    const epoch = this.epoch;
    this.lastSampleMinute = request.minute;
    this.update({ pendingActor: actor, status: 'pending' });
    let result = await this.scheduler.submitObserved(request);
    if (this.disposed || epoch !== this.epoch || source !== this.source) return;
    try {
      const latestWorld = source.worldOverview();
      const residents = source.residentRuntime().residents ?? [];
      if (identity(latestWorld) !== key || !residents.some((entry) => entry.id === actor && living(entry))) {
        this.resetWorld();
        return;
      }
      const minute = latestWorld.minute ?? NaN;
      if (!Number.isFinite(minute) || minute < request.minute
        || minute - request.minute > SHADOW_BUDGET.staleAfterSimulationMinutes) {
        result = { status: 'stale', reason: 'stale', proposal: null };
      }
      if (result.proposal?.targetResident
        && !residents.some((entry) => entry.id === result.proposal?.targetResident && living(entry))) {
        result = { status: 'rejected', reason: 'target_unavailable', proposal: null };
      }
    } catch {
      result = { status: 'error', reason: 'context_error', proposal: null };
    }
    // Obsolete simulation-minute results may expose status, never obsolete proposals/context.
    if (result.status === 'stale') {
      this.update({ pendingActor: null, status: 'stale', records: [] });
      return;
    }
    const record: ShadowRecord = {
      id: ++this.recordId, actor, name: resident.name, minute: request.minute,
      actual: {
        activityKind: resident.activityKind, activityLabel: resident.activityLabel,
        physicalGoal: resident.physicalGoal, socialIntent: resident.socialIntent,
        presentation: resident.presentation,
      },
      context: request, provider: this.provider?.name ?? '', outcome: result,
    };
    const previous = this.state.records.filter((entry) => entry.actor !== actor);
    const own = this.state.records.filter((entry) => entry.actor === actor).slice(0, SHADOW_BUDGET.historyPerResident - 1);
    const records = [record, ...own, ...previous];
    const actors = [...new Set(records.map((entry) => entry.actor))].slice(0, SHADOW_BUDGET.maxResidents);
    this.update({ pendingActor: null, status: 'ready', records: records.filter((entry) => actors.includes(entry.actor)) });
  }
}
export const shadowObserver = new ShadowObserver();
