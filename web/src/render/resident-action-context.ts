import type {
  Resident,
  ResidentPresentationDirective,
} from '../runtime/core-types';

export interface ResidentActionCue {
  text: string;
  phase: 'Moving' | 'Interacting' | 'Idle';
}

const PHYSICAL_LABELS: Record<string, string> = {
  Eat: '식사',
  Drink: '물 마시기',
  Sleep: '잠자기',
  UseToilet: '용변',
  Wash: '씻기',
};

const SOCIAL_LABELS: Record<string, string> = {
  Approach: '다가가기',
  Avoid: '피하기',
  Repair: '관계 회복',
  Comfort: '위로하기',
};

const CIVILIZATION_LABELS: Record<string, string> = {
  Gather: '채집',
  Store: '저장',
  Retrieve: '가져오기',
  Experiment: '실험',
  Craft: '제작',
};

const PARENTING_LABELS: Record<string, string> = {
  Feed: '먹이기',
  PutToSleep: '재우기',
  Bathe: '씻기기',
  ToiletAssist: '용변 돕기',
  Hold: '안아주기',
  Play: '함께 놀기',
  Educate: '가르치기',
  Discipline: '훈육',
  Comfort: '달래기',
  HealthCare: '돌보기',
};

function readable(
  value: string | undefined,
  labels: Record<string, string>,
  fallback: string,
): string {
  const key = value?.trim();
  if (!key || key === 'None' || key === 'Idle') return fallback;
  return labels[key] ?? key;
}

function targetResidentName(
  directive: ResidentPresentationDirective,
  residents: Resident[],
): string {
  const targetId = directive.targetResidentId?.trim();
  if (!targetId || targetId === '0') return '';
  return residents.find((resident) => resident.id === targetId)?.name ?? '';
}

function targetObjectName(
  directive: ResidentPresentationDirective,
): string {
  if (!directive.hasObjectTarget) return '';
  switch (directive.objectKind) {
    case 'Bed': return '잠자리';
    case 'Toilet': return '화장실';
    case 'Sink': return '씻는 곳';
    case 'Fridge': return '식량 보관소';
    case 'Chair': return '의자';
    case 'Table': return '작업대';
    case 'Sofa': return '휴식 장소';
    default: return '';
  }
}

export function residentActionCue(
  resident: Resident,
  residents: Resident[],
): ResidentActionCue | null {
  const directive = resident.presentation;
  if (!directive?.active) return null;

  const phase = directive.phase ?? 'Idle';
  if (phase === 'Idle') return null;

  const residentTarget = targetResidentName(directive, residents);
  const objectTarget = targetObjectName(directive);

  let action = '';
  switch (directive.kind) {
    case 'Physical':
      action = readable(
        directive.physicalGoal,
        PHYSICAL_LABELS,
        '생활 행동',
      );
      break;
    case 'Social':
      action = readable(
        directive.socialIntent,
        SOCIAL_LABELS,
        '사회 행동',
      );
      break;
    case 'Civilization':
      action = readable(
        directive.civilizationIntent,
        CIVILIZATION_LABELS,
        '작업',
      );
      break;
    case 'Parenting':
      action = readable(
        directive.parentingAction,
        PARENTING_LABELS,
        '돌보기',
      );
      break;
    case 'KnowledgeTeaching':
      action = '가르치기';
      break;
    default:
      return null;
  }

  const target = residentTarget || objectTarget;
  const phaseSuffix = phase === 'Moving' ? '이동 중' : '진행 중';

  return {
    text: target
      ? `${action} · ${target} · ${phaseSuffix}`
      : `${action} · ${phaseSuffix}`,
    phase,
  };
}
