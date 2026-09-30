import type {
  Resident,
  ResidentPresentationDirective,
} from '../runtime/core-types';
import {
  formatCivilizationIntent,
  formatFacilityKind,
  formatMaterial,
  formatObjectKind,
  formatParentingAction,
  formatSocialIntent,
  formatTechnique,
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

function koreanParticle(
  text: string,
  withFinalConsonant: string,
  withoutFinalConsonant: string,
): string {
  if (!text) return '';
  const lastCode = text.charCodeAt(text.length - 1);
  const hasFinalConsonant = (
    lastCode >= 0xac00
    && lastCode <= 0xd7a3
    && (lastCode - 0xac00) % 28 !== 0
  );
  return `${text}${hasFinalConsonant
    ? withFinalConsonant
    : withoutFinalConsonant}`;
}

function socialActionCue(
  directive: ResidentPresentationDirective,
  target: string,
  phase: 'Moving' | 'Interacting',
): string {
  const moving = phase === 'Moving';
  const named = (withoutTarget: string, withTarget: string): string => (
    target ? withTarget : withoutTarget
  );

  switch (directive.socialIntent) {
    case 'Approach':
      return moving
        ? named('다가가는 중', `${target}에게 다가가는 중`)
        : named('교류를 시작함', `${koreanParticle(target, '과', '와')} 교류를 시작함`);
    case 'Comfort':
      return moving
        ? named('위로하러 이동 중', `${koreanParticle(target, '을', '를')} 위로하러 이동 중`)
        : named('위로하는 중', `${koreanParticle(target, '을', '를')} 위로하는 중`);
    case 'Repair':
      return moving
        ? named('관계를 회복하러 이동 중', `${koreanParticle(target, '과', '와')} 관계를 회복하러 이동 중`)
        : named('관계 회복을 시도하는 중', `${koreanParticle(target, '과', '와')} 관계 회복을 시도하는 중`);
    case 'Avoid':
      return named('거리를 두는 중', `${koreanParticle(target, '과', '와')} 거리를 두는 중`);
    default: {
      const action = formatSocialIntent(directive.socialIntent);
      return target
        ? `${target}와 ${action} · ${moving ? '이동 중' : '진행 중'}`
        : `${action} · ${moving ? '이동 중' : '진행 중'}`;
    }
  }
}

function parentingActionCue(
  directive: ResidentPresentationDirective,
  target: string,
  phase: 'Moving' | 'Interacting',
): string {
  const moving = phase === 'Moving';
  const objectTarget = target
    ? koreanParticle(target, '을', '를')
    : '';
  const withTarget = target
    ? koreanParticle(target, '과', '와')
    : '';
  const toTarget = target ? `${target}에게` : '';

  switch (directive.parentingAction) {
    case 'Feed':
      return target
        ? `${toTarget} 먹이를 ${moving ? '주러 이동 중' : '주는 중'}`
        : `먹이를 ${moving ? '주러 이동 중' : '주는 중'}`;
    case 'PutToSleep':
      return `${objectTarget ? `${objectTarget} ` : ''}${moving ? '재우러 이동 중' : '재우는 중'}`;
    case 'Bathe':
      return `${objectTarget ? `${objectTarget} ` : ''}${moving ? '씻겨주러 이동 중' : '씻겨주는 중'}`;
    case 'ToiletAssist':
      return target
        ? `${target}의 용변을 ${moving ? '도우러 이동 중' : '돕는 중'}`
        : `용변을 ${moving ? '도우러 이동 중' : '돕는 중'}`;
    case 'Hold':
      return `${objectTarget ? `${objectTarget} ` : ''}${moving ? '안아주러 이동 중' : '안아주는 중'}`;
    case 'Play':
      return `${withTarget ? `${withTarget} ` : ''}${moving ? '함께 놀러 이동 중' : '함께 노는 중'}`;
    case 'Educate':
      return `${objectTarget ? `${objectTarget} ` : ''}${moving ? '가르치러 이동 중' : '가르치는 중'}`;
    case 'Discipline':
      return `${objectTarget ? `${objectTarget} ` : ''}${moving ? '훈육하러 이동 중' : '훈육하는 중'}`;
    case 'Comfort':
      return `${objectTarget ? `${objectTarget} ` : ''}${moving ? '달래러 이동 중' : '달래는 중'}`;
    case 'HealthCare':
      return `${objectTarget ? `${objectTarget} ` : ''}${moving ? '돌보러 이동 중' : '돌보는 중'}`;
    default: {
      const action = formatParentingAction(directive.parentingAction);
      return target
        ? `${target} 돌봄 · ${action} · ${moving ? '이동 중' : '진행 중'}`
        : `${action} · ${moving ? '이동 중' : '진행 중'}`;
    }
  }
}

function teachingActionCue(
  directive: ResidentPresentationDirective,
  target: string,
  phase: 'Moving' | 'Interacting',
): string {
  const technique = directive.knowledgeTeachingTechnique?.trim();
  const techniqueLabel = technique && technique !== 'None'
    ? formatTechnique(technique)
    : '지식';
  const targetPrefix = target ? `${target}에게 ` : '';
  return phase === 'Moving'
    ? `${targetPrefix}${techniqueLabel} 지식을 가르치러 이동 중`
    : `${targetPrefix}${techniqueLabel} 지식을 가르치는 중`;
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
  visuallyMoving = false,
): ResidentActionCue | null {
  const directive = resident.presentation;
  if (!directive?.active) return null;

  const corePhase = directive.phase ?? 'Idle';
  // Core may reach the authoritative target between observer snapshots while
  // the Three.js actor is still visually interpolating there. Never announce
  // or animate an interaction before the visible body has arrived.
  const phase = visuallyMoving && corePhase === 'Interacting'
    ? 'Moving'
    : corePhase;
  if (phase === 'Idle') return null;

  const residentTarget = targetResidentName(directive, residents);
  const objectTarget = targetObjectName(directive);

  let action = '';
  switch (directive.kind) {
    case 'Physical':
      if (directive.physicalGoal === 'Wash') {
        if (directive.directNaturalWaterSource) {
          return {
            text: phase === 'Moving'
              ? '물가로 씻으러 이동 중'
              : '물가에서 씻는 중',
            phase,
          };
        }
        return {
          text: phase === 'Moving'
            ? '소지한 물로 씻으러 이동 중'
            : '소지한 물로 씻는 중',
          phase,
        };
      }
      if (directive.physicalGoal === 'Drink') {
        if (directive.directNaturalWaterSource) {
          return {
            text: phase === 'Moving'
              ? '물가로 마시러 이동 중'
              : '물가에서 마시는 중',
            phase,
          };
        }
        return {
          text: phase === 'Moving'
            ? '소지한 물을 마시러 이동 중'
            : '소지한 물을 마시는 중',
          phase,
        };
      }
      if (directive.physicalGoal === 'UseToilet') {
        if (directive.designatedSanitationSite) {
          return {
            text: phase === 'Moving'
              ? '위생 장소로 이동 중'
              : '위생 장소 이용 중',
            phase,
          };
        }
        if (directive.emergencyFallback) {
          return {
            text: phase === 'Moving'
              ? '야외 용변 장소로 이동 중'
              : '야외 용변 중',
            phase,
          };
        }
      }
      action = physicalAction(directive.physicalGoal);
      break;
    case 'Social':
      return {
        text: socialActionCue(directive, residentTarget, phase),
        phase,
      };
    case 'Civilization': {
      action = formatCivilizationIntent(directive.civilizationIntent);
      const material = directive.civilizationMaterial?.trim();
      const materialLabel = material && material !== 'Unknown'
        ? formatMaterial(material)
        : '';
      const facilityKind = directive.facilityKind?.trim();
      const facilityLabel = facilityKind && facilityKind !== 'Unknown'
        ? formatFacilityKind(facilityKind)
        : '시설';

      if (directive.civilizationIntent === 'Explore' && materialLabel) {
        return {
          text: phase === 'Moving'
            ? `${materialLabel} 탐색 지역으로 이동 중`
            : `${materialLabel} 탐색 중`,
          phase,
        };
      }
      if (directive.civilizationIntent === 'Gather' && materialLabel) {
        return {
          text: phase === 'Moving'
            ? `${materialLabel} 있는 곳으로 이동 중`
            : `${materialLabel} 채집 중`,
          phase,
        };
      }
      if (directive.civilizationIntent === 'Retrieve' && materialLabel) {
        return {
          text: phase === 'Moving'
            ? `공동 저장소에서 ${materialLabel} 가져오러 이동 중`
            : `공동 저장소에서 ${materialLabel} 꺼내는 중`,
          phase,
        };
      }
      if (directive.civilizationIntent === 'Store' && materialLabel) {
        return {
          text: phase === 'Moving'
            ? `공동 저장소에 ${materialLabel} 보관하러 이동 중`
            : `공동 저장소에 ${materialLabel} 보관하는 중`,
          phase,
        };
      }
      if (directive.civilizationIntent === 'Craft') {
        switch (directive.facilityAction) {
          case 'Plan':
            return {
              text: phase === 'Moving'
                ? `${facilityLabel} 부지를 정하러 이동 중`
                : `${facilityLabel} 건설 계획 중`,
              phase,
            };
          case 'DeliverMaterial':
            return {
              text: materialLabel
                ? (phase === 'Moving'
                  ? `${materialLabel} 자재를 ${facilityLabel} 작업지로 운반 중`
                  : `${facilityLabel}에 ${materialLabel} 자재 전달 중`)
                : `${facilityLabel} 자재 ${phase === 'Moving' ? '운반 중' : '전달 중'}`,
              phase,
            };
          case 'Work':
            return {
              text: phase === 'Moving'
                ? `${facilityLabel} 작업지로 이동 중`
                : `${facilityLabel} 건설 작업 중`,
              phase,
            };
          case 'Repair':
            return {
              text: phase === 'Moving'
                ? `${facilityLabel} 수리·복구하러 이동 중`
                : `${facilityLabel} 수리·복구 작업 중`,
              phase,
            };
          default:
            break;
        }
      }
      if (materialLabel) {
        action = `${action} · ${materialLabel}`;
      }
      break;
    }
    case 'Parenting':
      return {
        text: parentingActionCue(directive, residentTarget, phase),
        phase,
      };
    case 'KnowledgeTeaching':
      return {
        text: teachingActionCue(directive, residentTarget, phase),
        phase,
      };
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
