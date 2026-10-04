import type { Resident, ResidentLifeEvent } from '../runtime/core-types';
import { WORLD_PRESENTATION } from './world-presentation-config';
const C = WORLD_PRESENTATION.residentLife;
export type LifeVisualStage = 'Baby' | 'Toddler' | 'Child' | 'Teen' | 'Adult' | 'Elderly';
const semanticStages: Record<string, LifeVisualStage> = {
  Baby: 'Baby', Toddler: 'Toddler', Child: 'Child', Teen: 'Teen',
  YoungAdult: 'Adult', Adult: 'Adult', MiddleAge: 'Adult', Elderly: 'Elderly',
};
export const PARENTING_ACTIONS = ['Feed', 'PutToSleep', 'Bathe', 'ToiletAssist', 'Hold',
  'Play', 'Educate', 'Discipline', 'Comfort', 'HealthCare'] as const;
export function deriveResidentLifeVisualState(resident: Resident) {
  const age = Number(resident.ageYears);
  const fallback: LifeVisualStage = age < 2 ? 'Baby' : age < 6 ? 'Toddler'
    : age < 13 ? 'Child' : age < 18 ? 'Teen' : age >= 70 ? 'Elderly' : 'Adult';
  // Known Core stage takes precedence. An unknown future stage stays neutral;
  // age fallback is compatibility only when the DTO omits stage entirely.
  const stage = resident.lifeStage ? semanticStages[resident.lifeStage] ?? 'Adult' : fallback;
  const p = resident.pregnancy;
  const pregnancyStage = resident.alive !== false && p?.role === 'GestationalParent'
    && Object.prototype.hasOwnProperty.call(C.belly, p.stage ?? '') ? p.stage as keyof typeof C.belly : null;
  return { stage, age: Number.isFinite(age) ? Math.max(0, age) : null,
    headScale: C.headScale[stage], bodyWidth: C.bodyWidth[stage], gaitBias: C.gait[stage],
    elderLean: stage === 'Elderly' ? C.elderLeanRadians : 0,
    pregnancyStage, belly: pregnancyStage ? C.belly[pregnancyStage] : 0,
    isGestationalParent: pregnancyStage !== null, isNewborn: stage === 'Baby',
    isChild: stage === 'Baby' || stage === 'Toddler' || stage === 'Child',
  };
}
export function parentingPresentationPair(resident: Resident, byId: ReadonlyMap<string, Resident>) {
  const p = resident.presentation, target = byId.get(p?.targetResidentId ?? '');
  if (resident.alive === false || !resident.hasPosition || !p?.active || p.kind !== 'Parenting'
    || (p.phase !== 'Moving' && p.phase !== 'Interacting') || !target || target.id === resident.id
    || target.alive === false || !target.hasPosition
    || !PARENTING_ACTIONS.some(action => action === p.parentingAction)) return null;
  return { sourceId: resident.id, targetId: target.id, action: p.parentingAction!, interacting: p.phase === 'Interacting' };
}
export function lifeEventPresentationKey(id: string, e: ResidentLifeEvent): string {
  return JSON.stringify([id, e.type, e.minute, [...(e.relatedCharacterIds ?? [])].sort()]);
}
export const lifePairKey = (a: string, b: string): string => JSON.stringify([a, b].sort());
