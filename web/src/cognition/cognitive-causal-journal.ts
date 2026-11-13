import { COGNITIVE_INTENTS, type CognitiveIntent } from './cognitive-contract';
export interface CognitiveTraceStep {
  token: string; previousToken: string; exchangeFact: string; issuedMinute: number; resultMinute: number;
  kind: number; source: number; result: number; attempted: boolean; target: string;
  action: string; material: string; technique: string; reason: string;
  learning: { changed: boolean; subject: string; proposition: string; beforeCount: string; afterCount: string; beforeStance: number; afterStance: number };
}
export interface CognitiveTraceRecord {
  id: string; accepted: boolean; selectionState: number; rejection: number; minute: number; observedMinute: number;
  contextFingerprint: string; observedFingerprint: string; intent: CognitiveIntent | 'None'; target: string;
  omittedSteps: number; steps: CognitiveTraceStep[];
}
export interface CognitiveCausalJournal {
  schema: 1; actor: string; minute: number; worldSeed: string; populationSeed: string; generationVersion: number;
  olderRecords: number; records: CognitiveTraceRecord[];
}
const object = (v: unknown): v is Record<string, unknown> => !!v && typeof v === 'object' && !Array.isArray(v);
const integer = (v: unknown, min = 0, max = 2147483647): v is number => typeof v === 'number' && Number.isSafeInteger(v) && v >= min && v <= max;
const id = (v: unknown, zero = false): v is string => typeof v === 'string' && /^(0|[1-9]\d{0,19})$/.test(v) && (zero || v !== '0') && BigInt(v) <= 18446744073709551615n;
const text = (v: unknown, max: number): v is string => typeof v === 'string' && v.length <= max;
const stance = (v: unknown): v is number => typeof v === 'number' && Number.isFinite(v) && v >= -1 && v <= 1;
/** Fail closed as a whole: an invalid owner, source or token cannot become a causal edge. */
export function parseCognitiveCausalJournal(value: unknown, actor: string): CognitiveCausalJournal | null {
  if (!object(value) || value.schema !== 1 || value.actor !== actor || !id(value.actor)
    || !integer(value.minute) || !id(value.worldSeed) || !id(value.populationSeed, true)
    || !integer(value.generationVersion, 1) || !integer(value.olderRecords)
    || !Array.isArray(value.records) || value.records.length > 8) return null;
  const records: CognitiveTraceRecord[] = [], ids = new Set<string>(), acceptedMinutes = new Set<number>(), actions = new Set<string>();
  for (const r of value.records) {
    if (!object(r) || !id(r.id) || ids.has(r.id) || typeof r.accepted !== 'boolean'
      || !integer(r.selectionState, 0, 3) || (r.accepted ? r.selectionState === 0 : r.selectionState !== 0)
      || !integer(r.rejection, 0, 3) || (r.accepted ? r.rejection !== 0 : r.rejection === 0)
      || !integer(r.minute, 0, value.minute) || !integer(r.observedMinute, 0, r.minute)
      || !id(r.contextFingerprint) || !id(r.observedFingerprint) || !id(r.target, true)
      || !text(r.intent, 64) || !(COGNITIVE_INTENTS as readonly string[]).includes(r.intent) && !(r.intent === 'None' && !r.accepted)
      || !integer(r.omittedSteps) || !Array.isArray(r.steps) || r.steps.length > 24 || !r.accepted && r.steps.length > 0) return null;
    if (r.accepted && acceptedMinutes.has(r.minute)) return null;
    if (r.accepted) acceptedMinutes.add(r.minute);
    ids.add(r.id);
    const steps: CognitiveTraceStep[] = [];
    for (const s of r.steps) {
      if (!object(s) || !id(s.token, true) || !id(s.previousToken, true) || !id(s.exchangeFact, true)
        || (s.token === '0') === (s.exchangeFact === '0') || s.previousToken !== '0' && s.previousToken === s.token
        || !integer(s.issuedMinute, r.minute, value.minute) || !integer(s.result, 0, 6)
        || !integer(s.resultMinute, s.result === 0 ? -1 : s.issuedMinute, s.result === 0 ? -1 : value.minute)
        || !integer(s.kind, 1, 5) || s.exchangeFact !== '0' && s.kind !== 5
        || !integer(s.source, 0, 2) || s.source === 2 && s.previousToken === '0'
        || typeof s.attempted !== 'boolean' || s.result === 1 && !s.attempted || !id(s.target, true)
        || !text(s.action, 64) || !s.action || !text(s.material, 64) || !text(s.technique, 64) || !text(s.reason, 128)) return null;
      const key = s.token === '0' ? 'fact:' + s.exchangeFact : 'token:' + s.token;
      if (actions.has(key)) return null; actions.add(key);
      const l = s.learning;
      if (!object(l) || typeof l.changed !== 'boolean' || !id(l.subject, true) || !text(l.proposition, 128)
        || !id(l.beforeCount, true) || !id(l.afterCount, true) || !stance(l.beforeStance) || !stance(l.afterStance)
        || l.changed && (s.result === 0 || !s.attempted || !l.proposition || BigInt(l.afterCount) <= BigInt(l.beforeCount))) return null;
      steps.push({ ...s, learning: { ...l } } as unknown as CognitiveTraceStep);
    }
    records.push({ ...r, steps } as unknown as CognitiveTraceRecord);
  }
  return { ...value, records } as unknown as CognitiveCausalJournal;
}
