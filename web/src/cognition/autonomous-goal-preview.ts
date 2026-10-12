// Read-only Core-generated hypotheses about possible future desired states.
// Not a Core command, not an accepted COG-3 journal event and not an action.
export const AUTONOMOUS_GOAL_METRICS = [
  'ActorNeed',
  'ActorRawMaterialCount',
  'RelationshipTrust',
  'KnownResidentAlive',
] as const;

export type AutonomousGoalMetric = (typeof AUTONOMOUS_GOAL_METRICS)[number];
export type AutonomousGoalNeed = 'Hunger' | 'Thirst' | 'Sleep' | 'Bladder' | 'Hygiene';
export type AutonomousGoalOperator = 'AtMost' | 'AtLeast';
export type AutonomousGoalEvidenceKind = 'ActorNeed' | 'Remembered' | 'Believed' | 'Related';

export interface AutonomousGoalCondition {
  metric: AutonomousGoalMetric;
  need: AutonomousGoalNeed;
  materialCode: number;
  targetResident: string;
  operator: AutonomousGoalOperator;
  threshold: number;
}

export interface AutonomousGoalEvidence {
  kind: AutonomousGoalEvidenceKind;
  index: number;
}

export interface AutonomousGoalCandidatePreview {
  priority: number;
  expiresMinute: number;
  conditions: AutonomousGoalCondition[];
  evidence: AutonomousGoalEvidence[];
}

export interface AutonomousGoalPreview {
  available: true;
  actor: string;
  minute: number;
  source: 'CoreLocalGoalComposer';
  affectsSimulation: false;
  candidates: AutonomousGoalCandidatePreview[];
}

const MAX_CANDIDATES = 6;
const MAX_CONDITIONS = 6;
const MAX_EVIDENCE = 8;
const MAX_HORIZON_MINUTES = 30 * 1440;
const NEEDS = new Set(['Hunger', 'Thirst', 'Sleep', 'Bladder', 'Hygiene']);
const OPERATORS = new Set(['AtMost', 'AtLeast']);
const METRICS = new Set<string>(AUTONOMOUS_GOAL_METRICS);
const EVIDENCE_KINDS = new Set(['ActorNeed', 'Remembered', 'Believed', 'Related']);

function asRecord(value: unknown): Record<string, unknown> {
  if (!value || typeof value !== 'object' || Array.isArray(value)) {
    throw new Error('Goal preview must be an object');
  }
  return value as Record<string, unknown>;
}

function boundedNumber(value: unknown, min: number, max: number): number {
  if (typeof value !== 'number' || !Number.isFinite(value)
    || value < min || value > max) {
    throw new Error('Invalid goal preview number');
  }
  return value;
}

function decimalId(value: unknown): string {
  if (typeof value !== 'string' || !/^(0|[1-9][0-9]{0,19})$/u.test(value)) {
    throw new Error('Invalid goal preview actor/target identifier');
  }
  return value;
}

export function parseAutonomousGoalPreview(value: unknown): AutonomousGoalPreview {
  const payload = asRecord(value);
  if (payload.available !== true || payload.affectsSimulation !== false
    || payload.source !== 'CoreLocalGoalComposer') {
    throw new Error('Not a read-only Core goal preview');
  }

  const actor = decimalId(payload.actor);
  if (actor === '0') throw new Error('Missing goal preview actor');
  const minute = boundedNumber(payload.minute, 0, Number.MAX_SAFE_INTEGER);
  if (!Number.isSafeInteger(minute)) throw new Error('Invalid goal preview minute');
  if (!Array.isArray(payload.candidates)
    || payload.candidates.length > MAX_CANDIDATES) {
    throw new Error('Invalid goal preview candidate count');
  }

  const candidates: AutonomousGoalCandidatePreview[] = payload.candidates.map((entry: unknown) => {
    const candidate = asRecord(entry);
    const priority = boundedNumber(candidate.priority, 0, 1);
    const expiresMinute = boundedNumber(
      candidate.expiresMinute, minute, minute + MAX_HORIZON_MINUTES,
    );
    if (!Number.isSafeInteger(expiresMinute)) throw new Error('Invalid goal expiry');
    if (!Array.isArray(candidate.conditions)
      || candidate.conditions.length < 1
      || candidate.conditions.length > MAX_CONDITIONS) {
      throw new Error('Invalid goal conditions');
    }
    if (!Array.isArray(candidate.evidence)
      || candidate.evidence.length < 1
      || candidate.evidence.length > MAX_EVIDENCE) {
      throw new Error('Invalid goal evidence');
    }

    const conditions: AutonomousGoalCondition[] = candidate.conditions.map((raw: unknown) => {
      const condition = asRecord(raw);
      if (!METRICS.has(String(condition.metric))
        || !NEEDS.has(String(condition.need))
        || !OPERATORS.has(String(condition.operator))) {
        throw new Error('Unknown Core goal predicate');
      }
      const materialCode = boundedNumber(condition.materialCode, 0, 64);
      if (!Number.isSafeInteger(materialCode)) throw new Error('Invalid material code');
      return {
        metric: condition.metric as AutonomousGoalMetric,
        need: condition.need as AutonomousGoalNeed,
        operator: condition.operator as AutonomousGoalOperator,
        materialCode,
        targetResident: decimalId(condition.targetResident),
        threshold: boundedNumber(condition.threshold, 0, 100000),
      };
    });
    const evidence: AutonomousGoalEvidence[] = candidate.evidence.map((raw: unknown) => {
      const fact = asRecord(raw);
      if (!EVIDENCE_KINDS.has(String(fact.kind))) {
        throw new Error('Unknown goal evidence kind');
      }
      const index = boundedNumber(fact.index, 0, 31);
      if (!Number.isSafeInteger(index)) throw new Error('Invalid evidence index');
      return { kind: fact.kind as AutonomousGoalEvidenceKind, index };
    });
    return { priority, expiresMinute, conditions, evidence };
  });

  return {
    available: true, actor, minute, source: 'CoreLocalGoalComposer',
    affectsSimulation: false, candidates,
  };
}
