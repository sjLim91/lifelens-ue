export interface ResidentMind {
  curiosity: number;
  habit: number;
  attachment: number;
  riskTolerance: number;
  trust: number;
  eligibleEpisodes: number;
  omittedSummaries: number;
  summaries: {
    domain: string;
    source: 'DirectWitness' | 'ToldByOther' | 'Inferred';
    subject: string;
    episodes: number;
    successes: number;
    failures: number;
    firstMinute: number;
    lastMinute: number;
    stance: number;
    omittedEvidence: number;
    evidenceIds: string[];
  }[];
  places: {
    place: string;
    minute: number;
    source: 'DirectWitness' | 'ToldByOther' | 'Inferred';
    confidence: number;
    evidenceId: string;
  }[];
  perceptions: {
    channel: '시각' | '청각' | '흔적';
    subject: string;
    subjectId: string;
    minute: number;
    distanceCells: number;
    hasPosition: boolean;
    x?: number;
    y?: number;
  }[];
}

const object = (value: unknown): Record<string, unknown> => {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error('invalid resident mind');
  return value as Record<string, unknown>;
};
const integer = (value: unknown, max = Number.MAX_SAFE_INTEGER): boolean =>
  Number.isSafeInteger(value) && Number(value) >= 0 && Number(value) <= max;
const number = (value: unknown, min = 0): boolean =>
  typeof value === 'number' && Number.isFinite(value) && value >= min && value <= 1;
const id = (value: unknown): boolean => typeof value === 'string'
  && /^(0|[1-9][0-9]{0,19})$/.test(value) && BigInt(value) <= 18446744073709551615n;
const source = (value: unknown): boolean =>
  value === 'DirectWitness' || value === 'ToldByOther' || value === 'Inferred';
const domains = new Set(['resource', 'tool', 'person', 'action', 'ordinary', 'danger', 'animal', 'place', 'trace']);

export function parseResidentMind(value: unknown, minute: number): ResidentMind {
  const mind = object(value);
  if (!['curiosity', 'habit', 'attachment', 'riskTolerance', 'trust'].every(key => number(mind[key]))
    || !integer(mind.eligibleEpisodes) || !integer(mind.omittedSummaries)
    || !Array.isArray(mind.summaries) || mind.summaries.length > 24
    || !Array.isArray(mind.places) || mind.places.length > 12
    || !Array.isArray(mind.perceptions) || mind.perceptions.length > 12) throw new Error('invalid resident mind limits');
  for (const value of mind.summaries) {
    const item = object(value);
    if (!domains.has(String(item.domain)) || !source(item.source) || !id(item.subject)
      || !integer(item.episodes) || !integer(item.successes) || !integer(item.failures)
      || Number(item.successes) + Number(item.failures) > Number(item.episodes)
      || !integer(item.firstMinute, minute) || !integer(item.lastMinute, minute)
      || Number(item.firstMinute) > Number(item.lastMinute) || !number(item.stance, -1)
      || !integer(item.omittedEvidence) || !Array.isArray(item.evidenceIds)
      || item.evidenceIds.length > 8 || !item.evidenceIds.every(id)
      || item.evidenceIds.length + Number(item.omittedEvidence) !== Number(item.episodes)
      || new Set(item.evidenceIds).size !== item.evidenceIds.length) throw new Error('invalid mind summary evidence');
  }
  for (const value of mind.places) {
    const item = object(value);
    if (typeof item.place !== 'string' || item.place.length > 96 || !item.place.startsWith('grid:')
      || !integer(item.minute, minute) || !source(item.source) || !number(item.confidence)
      || !id(item.evidenceId)) throw new Error('invalid personal place evidence');
  }
  for (const value of mind.perceptions) {
    const item = object(value);
    if (!['시각', '청각', '흔적'].includes(String(item.channel)) || !domains.has(String(item.subject))
      || !id(item.subjectId) || item.minute !== minute || !integer(item.distanceCells, 8)
      || typeof item.hasPosition !== 'boolean'
      || (item.channel === '청각'
        ? item.hasPosition || item.subjectId !== '0' || item.x !== undefined || item.y !== undefined
        : !item.hasPosition || item.subjectId === '0' || !Number.isSafeInteger(item.x) || !Number.isSafeInteger(item.y))) {
      throw new Error('invalid resident sensory provenance');
    }
  }
  return mind as unknown as ResidentMind;
}
