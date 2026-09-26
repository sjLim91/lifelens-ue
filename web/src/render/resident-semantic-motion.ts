import type { ResidentPresentationDirective } from '../runtime/core-types';

export type ResidentSemanticMotion =
  | 'idle'
  | 'walk'
  | 'talk'
  | 'interact'
  | 'crouch'
  | 'work';

export interface ResidentSemanticMotionContext {
  moving: boolean;
  nearbyResident: boolean;
}

export function resolveResidentSemanticMotion(
  presentation: ResidentPresentationDirective | null | undefined,
  context: ResidentSemanticMotionContext,
): ResidentSemanticMotion {
  if (context.moving) return 'walk';

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

      // Eat / Drink / Sleep intentionally stay neutral until a dedicated,
      // semantically correct clip exists. A generic arm wave or sitting pose
      // would be a visual lie even when the Core action itself is real.
      return 'idle';

    case 'Civilization':
      if (!presentation.hasTargetGrid) return 'idle';

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
