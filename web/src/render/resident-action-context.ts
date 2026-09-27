import type {
  Resident,
  ResidentPresentationDirective,
} from '../runtime/core-types';
import {
  formatCivilizationIntent,
  formatObjectKind,
  formatParentingAction,
  formatSocialIntent,
} from '../localization/korean';

export interface ResidentActionCue {
  text: string;
  phase: 'Moving' | 'Interacting' | 'Idle';
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
  return formatObjectKind(directive.objectKind);
}

function physicalAction(goal: string | undefined): string {
  switch (goal) {
    case 'Eat': return '식사';
    case 'Drink': return '물 마시기';
    case 'Sleep': return '잠자기';
    case 'UseToilet': return '용변';
    case 'Wash': return '씻기';
    case 'Idle':
    case undefined:
      return '생활 행동';
    default:
      console.error(`[LifeLens 한글 UI] 번역 등록 누락: 신체행동:${goal}`);
      return '생활 행동 확인 중';
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
      action = physicalAction(directive.physicalGoal);
      break;
    case 'Social':
      action = formatSocialIntent(directive.socialIntent);
      break;
    case 'Civilization':
      action = formatCivilizationIntent(directive.civilizationIntent);
      break;
    case 'Parenting':
      action = formatParentingAction(directive.parentingAction);
      break;
    case 'KnowledgeTeaching':
      action = '가르치기';
      break;
    default:
      console.error(
        `[LifeLens 한글 UI] 번역 등록 누락: 행동종류:${directive.kind ?? '없음'}`,
      );
      return {
        text: phase === 'Moving' ? '행동 위치로 이동 중' : '행동 진행 중',
        phase,
      };
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
