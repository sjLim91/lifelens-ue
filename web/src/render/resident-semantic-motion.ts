import type { ResidentPresentationDirective } from '../runtime/core-types';

export type ResidentSemanticMotion =
  | 'idle'
  | 'walk'
  | 'talk'
  | 'interact'
  | 'crouch'
  | 'work'
  | 'consume'
  | 'harvest'
  | 'carry';

export interface ResidentSemanticMotionContext {
  moving: boolean;
  nearbyResident: boolean;
}

export function residentSleepPostureActive(
  presentation: ResidentPresentationDirective | null | undefined,
  moving: boolean,
): boolean {
  return (
    !moving
    && presentation?.active === true
    && presentation.kind === 'Physical'
    && presentation.phase === 'Interacting'
    && presentation.physicalGoal === 'Sleep'
  );
}

export function resolveResidentSemanticMotion(
  presentation: ResidentPresentationDirective | null | undefined,
  context: ResidentSemanticMotionContext,
): ResidentSemanticMotion {
  if (context.moving) {
    if (
      presentation?.active
      && presentation.kind === 'Civilization'
      && presentation.facilityAction === 'DeliverMaterial'
    ) {
      return 'carry';
    }
    return 'walk';
  }

  if (
    !presentation?.active
    || presentation.phase !== 'Interacting'
  ) {
    return 'idle';
  }

  switch (presentation.kind) {
    case 'Social':
    case 'KnowledgeTeaching':
      return context.nearbyResident ? 'talk' : 'idle';

    case 'Parenting':
      if (
        context.nearbyResident
        && (
          presentation.parentingAction === 'Educate'
          || presentation.parentingAction === 'Discipline'
        )
      ) {
        return 'talk';
      }
      return 'idle';

    case 'Physical':
      if (
        presentation.physicalGoal === 'Eat'
        || presentation.physicalGoal === 'Drink'
      ) {
        return 'consume';
      }

      if (
        presentation.physicalGoal === 'UseToilet'
        && (
          presentation.designatedSanitationSite === true
          || (
            presentation.hasObjectTarget === true
            && presentation.objectKind === 'Toilet'
          )
        )
      ) {
        return 'crouch';
      }

      if (
        presentation.physicalGoal === 'Wash'
        && presentation.hasObjectTarget === true
        && presentation.objectKind === 'Sink'
      ) {
        return 'interact';
      }

      // Sleep still stays neutral until an authored lie-down / sleep / wake
      // sequence is available. Never turn "sleep" into sitting or standing
      // hand interaction just to keep the body moving.
      return 'idle';

    case 'Civilization':
      if (!presentation.hasTargetGrid) return 'idle';

      if (
        presentation.facilityAction === 'Work'
        || presentation.facilityAction === 'Repair'
      ) {
        return 'work';
      }

      if (
        presentation.facilityAction === 'DeliverMaterial'
        || presentation.facilityAction === 'Fuel'
        || presentation.facilityAction === 'Ignite'
        || presentation.facilityAction === 'CollectCharcoal'
        || presentation.facilityAction === 'LoadSmeltCharge'
        || presentation.facilityAction === 'CollectMetal'
      ) {
        return 'interact';
      }

      if (
        presentation.civilizationIntent === 'Gather'
        && presentation.civilizationMaterial === 'PlantFood'
      ) {
        return 'harvest';
      }

      if (
        presentation.civilizationIntent === 'Craft'
        || presentation.civilizationIntent === 'Experiment'
      ) {
        return 'work';
      }

      if (
        presentation.civilizationIntent === 'Gather'
        || presentation.civilizationIntent === 'Store'
        || presentation.civilizationIntent === 'Retrieve'
      ) {
        return 'interact';
      }

      return 'idle';

    default:
      return 'idle';
  }
}
