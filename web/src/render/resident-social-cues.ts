import type { Resident } from '../runtime/core-types';

export type ResidentSocialCueKind =
  | 'Approach'
  | 'Comfort'
  | 'Repair'
  | 'Social'
  | 'KnowledgeTeaching'
  | 'Parenting';

export interface ResidentSocialCuePair {
  key: string;
  sourceId: string;
  targetId: string;
  kind: ResidentSocialCueKind;
}

function pairKey(first: string, second: string): string {
  return [first, second].sort().join('<>');
}

export function residentSocialCuePairs(
  residents: Resident[],
  maxPairs = 12,
): ResidentSocialCuePair[] {
  const residentIds = new Set(residents.map((resident) => resident.id));
  const seenPairs = new Set<string>();
  const pairs: ResidentSocialCuePair[] = [];

  for (const resident of residents) {
    if (pairs.length >= Math.max(0, maxPairs)) break;

    const presentation = resident.presentation;
    if (
      !presentation?.active
      || presentation.phase !== 'Interacting'
      || (
        presentation.kind !== 'Social'
        && presentation.kind !== 'KnowledgeTeaching'
        && presentation.kind !== 'Parenting'
      )
    ) {
      continue;
    }

    if (
      presentation.kind === 'Social'
      && presentation.socialIntent === 'Avoid'
    ) {
      continue;
    }

    const targetId = presentation.targetResidentId?.trim() ?? '';
    if (
      !targetId
      || targetId === '0'
      || targetId === resident.id
      || !residentIds.has(targetId)
    ) {
      continue;
    }

    const key = pairKey(resident.id, targetId);
    if (seenPairs.has(key)) continue;
    seenPairs.add(key);

    const kind: ResidentSocialCueKind = presentation.kind === 'Social'
      ? (
          presentation.socialIntent === 'Approach'
          || presentation.socialIntent === 'Comfort'
          || presentation.socialIntent === 'Repair'
            ? presentation.socialIntent
            : 'Social'
        )
      : presentation.kind;

    pairs.push({
      key,
      sourceId: resident.id,
      targetId,
      kind,
    });
  }

  return pairs;
}
