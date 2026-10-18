import type { CognitiveRequestDto } from './cognitive-contract';
import type { Resident, WorldOverview } from '../runtime/core-types';

// Read-only audit of *observed Core evidence*. No extra model calls, Core
// mutations, raw memories, rationales or resident names are stored/exported.
// This is a single-world session, NOT an automatically matched counterfactual.
export type ResidentAIAuditMode = 'baseline' | 'local-ai';
export type ResidentAIAuditStatus =
  'recorded' | 'stale' | 'rejected' | 'unavailable' | 'error';

export interface ResidentAISessionReport {
  schemaVersion: 1;
  kind: 'lifelens-live-ai-observational-session';
  mode: ResidentAIAuditMode;
  modelName: string | null;
  worldSeed: string;
  populationSeed: string;
  generationVersion: number | null;
  startMinute: number;
  endMinute: number;
  observedRefreshes: number;
  livingResidentsAtEnd: number | null;
  readableEvidenceResidentSamples: number;
  modelRequests: number;
  requestsWithFirsthandExperience: number;
  decisions: Record<ResidentAIAuditStatus, number>;
  coreCivilizationSuccessEvidenceDelta: number;
  coreCivilizationFailureEvidenceDelta: number;
  corePhysicalSuccessEvidenceDelta: number;
  corePhysicalFailureEvidenceDelta: number;
  coverageGap: boolean;
  endedReason: 'running' | 'stopped' | 'mode-changed' | 'world-changed' | 'rewind' | 'fast-forward';
  comparableWithinObservedWindow: boolean;
  caveat: string;
}

type EvidenceCount = { success: number; failure: number };

function worldKey(world: WorldOverview): string {
  return JSON.stringify([
    world.worldSeed, world.populationSeed, world.generationVersion,
  ]);
}

function safeCount(value: unknown): number {
  return typeof value === 'number' && Number.isSafeInteger(value) && value >= 0
    ? value : 0;
}

function experienceEvidence(resident: Resident): Map<string, EvidenceCount> {
  const evidence = new Map<string, EvidenceCount>();
  for (const belief of resident.beliefs ?? []) {
    if (belief.subject !== '0' && belief.subject !== '' && belief.subject !== undefined) continue;
    const key = belief.proposition;
    if (typeof key !== 'string' || !key.startsWith('experienced-action-outcome:')) continue;
    evidence.set(key, {
      success: safeCount(belief.supportCount),
      failure: safeCount(belief.contradictionCount),
    });
  }
  return evidence;
}

function freshDecisions(): Record<ResidentAIAuditStatus, number> {
  return { recorded: 0, stale: 0, rejected: 0, unavailable: 0, error: 0 };
}

export class ResidentAISessionAudit {
  private report: ResidentAISessionReport | null = null;
  private readonly previous = new Map<string, Map<string, EvidenceCount>>();
  private key: string | null = null;
  private running = false;

  begin(world: WorldOverview, mode: ResidentAIAuditMode, modelName: string | null): boolean {
    const minute = Number(world.minute);
    if (!Number.isSafeInteger(minute) || minute < 0
      || world.worldSeed === undefined || world.populationSeed === undefined) return false;
    this.previous.clear();
    this.key = worldKey(world);
    this.running = true;
    this.report = {
      schemaVersion: 1,
      kind: 'lifelens-live-ai-observational-session',
      mode, modelName: mode === 'local-ai' ? modelName : null,
      worldSeed: String(world.worldSeed),
      populationSeed: String(world.populationSeed),
      generationVersion: typeof world.generationVersion === 'number'
        ? world.generationVersion : null,
      startMinute: minute, endMinute: minute, observedRefreshes: 0,
      livingResidentsAtEnd: null, readableEvidenceResidentSamples: 0,
      modelRequests: 0, requestsWithFirsthandExperience: 0,
      decisions: freshDecisions(),
      coreCivilizationSuccessEvidenceDelta: 0,
      coreCivilizationFailureEvidenceDelta: 0,
      corePhysicalSuccessEvidenceDelta: 0,
      corePhysicalFailureEvidenceDelta: 0,
      coverageGap: false, endedReason: 'running',
      comparableWithinObservedWindow: true,
      caveat: 'Core belief evidence observed during this session is not proof that an AI proposal caused the action. Approval is not execution. A same-seed baseline also requires a matching full initial state; this report does not prove GPT-like intelligence.',
    };
    return true;
  }

  finish(reason: Exclude<ResidentAISessionReport['endedReason'], 'running'>): void {
    if (!this.report || !this.running) return;
    this.running = false;
    this.report.endedReason = reason;
    if (reason !== 'stopped') this.report.comparableWithinObservedWindow = false;
  }

  observe(world: WorldOverview, residents: readonly Resident[], mode: ResidentAIAuditMode): void {
    const report = this.report;
    if (!this.running || !report) return;
    if (this.key !== worldKey(world)) { this.finish('world-changed'); return; }
    if (report.mode !== mode) { this.finish('mode-changed'); return; }
    const minute = Number(world.minute);
    if (!Number.isSafeInteger(minute) || minute < report.endMinute) {
      this.finish('rewind'); return;
    }
    // A skipped simulation interval may contain thousands of unobserved
    // plans. Counters remain factual, but a paired causal comparison is unsafe.
    if (minute - report.endMinute > 96) {
      report.coverageGap = true;
      report.comparableWithinObservedWindow = false;
    }
    report.endMinute = minute;
    report.observedRefreshes++;
    report.livingResidentsAtEnd = typeof world.livingResidents === 'number'
      ? world.livingResidents : residents.filter(r => r.alive === true).length;

    for (const resident of residents) {
      if (!Array.isArray(resident.beliefs)) continue;
      report.readableEvidenceResidentSamples++;
      let prior = this.previous.get(resident.id);
      const current = experienceEvidence(resident);
      if (!prior) {
        this.previous.set(resident.id, current);
        continue; // Existing beliefs at the first observation are baseline.
      }
      for (const [key, counts] of current) {
        const before = prior.get(key);
        if (!before) {
          prior.set(key, counts);
          continue; // Unknown preexisting record: don't manufacture delta.
        }
        const success = Math.max(0, counts.success - before.success);
        const failure = Math.max(0, counts.failure - before.failure);
        if (key.startsWith('experienced-action-outcome:Civilization:')) {
          report.coreCivilizationSuccessEvidenceDelta += success;
          report.coreCivilizationFailureEvidenceDelta += failure;
        } else {
          report.corePhysicalSuccessEvidenceDelta += success;
          report.corePhysicalFailureEvidenceDelta += failure;
        }
        prior.set(key, counts);
      }
    }
  }

  inference(request: CognitiveRequestDto): void {
    if (!this.running || !this.report) return;
    this.report.modelRequests++;
    if (request.memories.some(m => m.source === 'DirectWitness'
      && m.witnessed === true && Array.isArray(m.tags)
      && (m.tags[0] === 'action_experience'
        || m.tags[0] === 'goal_action_feedback'
        || m.tags[0] === 'social_self_outcome'))) {
      this.report.requestsWithFirsthandExperience++;
    }
  }

  result(status: ResidentAIAuditStatus): void {
    if (!this.running || !this.report) return;
    this.report.decisions[status]++;
  }

  snapshot(): ResidentAISessionReport | null {
    if (!this.report) return null;
    return {
      ...this.report,
      decisions: { ...this.report.decisions },
    };
  }
}
