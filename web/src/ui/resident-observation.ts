import type { Resident, ResidentPersonality } from '../runtime/core-types';
import type { AutonomousGoalCondition, AutonomousGoalEvidenceKind } from '../cognition/autonomous-goal-preview';
import { CORE_MATERIAL_NAMES } from '../runtime/generated-core-contract';
import { formatMaterial, formatResidentName } from '../localization/korean';

export const PERSONALITY_LABELS: Record<keyof ResidentPersonality, string> = {
  introversion: '내향성', conscientiousness: '성실성', openness: '개방성',
  agreeableness: '친화성', emotionalStability: '정서 안정', empathy: '공감',
  impulsiveness: '충동성', riskTolerance: '위험 감수', ambition: '성취 욕구',
  patience: '인내심', sociability: '사교성', curiosity: '호기심',
  orderliness: '질서 선호', adaptability: '적응력',
};
export const GOAL_EVIDENCE_LABELS: Record<AutonomousGoalEvidenceKind, string> = {
  ActorNeed: '현재 욕구', Remembered: '개인 기억', Believed: '현재 믿음',
  Related: '관계 정보', ObservedLocalResource: '가까운 자원 정보',
};
export function residentPersonalityFacts(resident: Resident) {
  return (Object.keys(PERSONALITY_LABELS) as (keyof ResidentPersonality)[])
    .flatMap(key => {
      const value = resident.personality?.[key];
      return typeof value === 'number' && Number.isFinite(value) && value >= 0 && value <= 1
        ? [{ key, label: PERSONALITY_LABELS[key], value }] : [];
    });
}
export function goalConditionText(condition: AutonomousGoalCondition, residents: Resident[]) {
  const name = (id: string) => {
    const target = residents.find(r => r.id === id);
    return target ? formatResidentName(target.name) : '관찰되지 않은 주민';
  };
  const relation = condition.operator === 'AtMost' ? '이하' : '이상';
  const needs = { Hunger: '배고픔', Thirst: '갈증', Sleep: '수면 필요', Bladder: '용변 필요', Hygiene: '위생 필요' };
  switch (condition.metric) {
    case 'ActorNeed': return `${needs[condition.need]} ${Math.round(condition.threshold * 100)}% ${relation}`;
    case 'ActorRawMaterialCount': return `${formatMaterial(CORE_MATERIAL_NAMES[condition.materialCode])} ${condition.threshold}개 ${relation}`;
    case 'RelationshipTrust': return `${name(condition.targetResident)}에 대한 신뢰 ${Math.round(condition.threshold * 100)}% ${relation}`;
    case 'KnownResidentAlive': return `${name(condition.targetResident)}의 생존`;
  }
}
