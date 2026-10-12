import type { ResidentMemory } from '../runtime/core-types';

export interface ResidentExperienceHighlight {
  key: string;
  title: string;
  detail: string;
  minute: number;
  location?: string;
}

const actionLabels: Record<string, string> = {
  Eat: '식사',
  Drink: '물 마시기',
  Sleep: '수면',
  UseToilet: '배변',
  Wash: '위생 관리',
};

/**
 * Read Core-owned firsthand memories only. Do not invent reasoning, knowledge
 * or actual world outcomes from a local model's textual suggestions.
 */
export function deriveResidentExperienceHighlights(
  memories: ResidentMemory[] | undefined,
  limit = 3,
): ResidentExperienceHighlight[] {
  if (!memories?.length || limit <= 0) return [];
  const seen=new Map<string,ResidentExperienceHighlight>();
  for (const memory of memories) {
    if (memory.source !== 'DirectWitness' || memory.witnessed === false) continue;
    const tags=memory.tags ?? [];
    let title: string | undefined;
    let detail: string | undefined;
    let location: string | undefined;
    let key: string | undefined;
    if (tags.length === 3 && tags[0] === 'action_experience'
      && (tags[2] === 'success' || tags[2] === 'failure')) {
      const action=actionLabels[tags[1]];
      if (!action) continue;
      title=`${action} 경험`;
      detail=tags[2] === 'success' ? '직접 시도에 성공함' : '직접 시도에 실패함';
      key=`action:${tags[1]}:${tags[2]}`;
    } else if (tags.includes('environment') && tags.includes('contamination')
      && memory.where?.startsWith('grid:')) {
      title='주변 위생 위험';
      detail='이 장소의 오염을 직접 경험함';
      location=memory.where;
      key=`environment:${memory.where}`;
    }
    if (!key || !title || !detail) continue;
    const minute=Number(memory.minute);
    if (!Number.isFinite(minute) || minute < 0) continue;
    const existing=seen.get(key);
    if (!existing || existing.minute<minute) {
      seen.set(key,{key,title,detail,minute,location});
    }
  }
  return Array.from(seen.values())
    .sort((a,b)=>b.minute-a.minute || a.key.localeCompare(b.key))
    .slice(0,limit);
}
