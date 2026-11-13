import type { Resident } from '../runtime/core-types';
import type { ResidentAIDecision } from '../cognition/live-resident-ai';

export const GOAL_OUTCOME_LABELS: Record<string, string> = {
  'already-reached-before-action': '실행 전에 이미 목표에 도달함',
  'achieved-during-action': '실제 실행 중 신뢰 목표에 도달함',
  'progress-but-not-achieved': '신뢰가 높아졌지만 목표에는 미달함',
  'not-achieved-no-progress': '신뢰 진전 없이 목표에 미달함',
};
export interface ResidentActionEvidence {
  token: string;
  minute: number;
  intent: 'Approach' | 'Repair';
  target?: string;
  outcome: string;
  learned: boolean;
}
const validMinute = (value: unknown, now: number): value is number =>
  typeof value === 'number' && Number.isSafeInteger(value) && value >= 0 && value <= now;
export function validActionToken(value: unknown): value is string {
  return typeof value === 'string' && /^[1-9]\d{0,19}$/.test(value)
    && BigInt(value) <= 18446744073709551615n;
}

/** Read-only evidence, never infer a proposal-to-action link from timing/intent. */
export function deriveResidentCausalEvidence(
  resident: Resident, decision: ResidentAIDecision | undefined, now: number,
) {
  const thought = decision?.actor === resident.id && validMinute(decision.minute, now)
    ? decision : undefined;
  const directive = resident.presentation;
  const attempt = resident.alive === true && directive?.active
    && validActionToken(directive.contextActionToken)
    && validMinute(directive.issuedMinute, now)
    && (directive.phase === 'Moving' || directive.phase === 'Interacting')
    ? { token: directive.contextActionToken, minute: directive.issuedMinute, phase: directive.phase }
    : undefined;
  const byToken = new Map<string, ResidentActionEvidence>();
  for (const memory of resident.memories ?? []) {
    const tags = memory.tags ?? [];
    if (memory.source !== 'DirectWitness' || memory.witnessed !== true
      || memory.sourceCharacter !== resident.id || !validActionToken(memory.who)
      || memory.who === resident.id || !validMinute(memory.minute, now)
      || tags.length !== 3 || tags[0] !== 'goal_action_feedback' || tags[1] !== 'self_trust'
      || (tags[2] !== 'Approach' && tags[2] !== 'Repair')
      || !memory.what || !Object.prototype.hasOwnProperty.call(GOAL_OUTCOME_LABELS, memory.what)) continue;
    const token = memory.where?.match(/^personal-goal-token:([1-9]\d*)$/)?.[1];
    if (!validActionToken(token)) continue;
    // This specific Core memory is emitted only after a matching completed
    // token. Goal reached before execution is explicitly excluded from learning.
    const learned = memory.what !== 'already-reached-before-action'
      && (resident.beliefs ?? []).some(belief => belief.subject === memory.who
        && belief.proposition === `experienced-self-trust-goal-progress:${tags[2]}`
        && belief.lastUpdatedMinute === memory.minute
        && Number.isFinite(belief.stance) && Number.isFinite(belief.confidence)
        && belief.confidence! > 0);
    const old = byToken.get(token);
    if (!old || old.minute < memory.minute) byToken.set(token, {
      token, minute: memory.minute, intent: tags[2], target: memory.who,
      outcome: memory.what, learned,
    });
  }
  const completed = [...byToken.values()]
    .sort((a, b) => b.minute - a.minute || a.token.localeCompare(b.token)).slice(0, 6);
  return { thought, attempt, completed };
}
