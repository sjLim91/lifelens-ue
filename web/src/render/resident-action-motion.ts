import { WORLD_UNITS_PER_GRID_CELL } from '../runtime/lifelens-contract';

export type ContextRestMotion = 'idle' | 'talk' | 'interact';

export interface ContextRestMotionInput {
  actionContextActive: boolean;
  actionContextKind?: string;
  socialIntent?: string;
  legacyActivityLabel?: string;
  targetResidentDistanceWorldUnits?: number;
  spatialTargetDistanceWorldUnits?: number;
  canTalk: boolean;
  canInteract: boolean;
}

export const CONTEXT_RESIDENT_PROXIMITY_WORLD_UNITS =
  WORLD_UNITS_PER_GRID_CELL * 3.5;

export const CONTEXT_SPATIAL_PROXIMITY_WORLD_UNITS =
  WORLD_UNITS_PER_GRID_CELL * 3.5;

function isNear(
  distance: number | undefined,
  threshold: number,
): boolean {
  return Number.isFinite(distance)
    && Number(distance) <= threshold;
}

export function chooseContextRestMotion(
  input: ContextRestMotionInput,
): ContextRestMotion {
  const nearResident = isNear(
    input.targetResidentDistanceWorldUnits,
    CONTEXT_RESIDENT_PROXIMITY_WORLD_UNITS,
  );
  const nearSpatialTarget = isNear(
    input.spatialTargetDistanceWorldUnits,
    CONTEXT_SPATIAL_PROXIMITY_WORLD_UNITS,
  );

  if (input.actionContextActive) {
    switch (input.actionContextKind) {
      case 'Social':
        if (
          input.socialIntent !== 'Avoid'
          && nearResident
          && input.canTalk
        ) {
          return 'talk';
        }
        return 'idle';

      case 'KnowledgeTeaching':
        return nearResident && input.canTalk
          ? 'talk'
          : 'idle';

      case 'Parenting':
        return nearResident && input.canInteract
          ? 'interact'
          : 'idle';

      case 'Civilization':
        return nearSpatialTarget && input.canInteract
          ? 'interact'
          : 'idle';

      default:
        return 'idle';
    }
  }

  // Backward-compatible support for an older explicit Talk label. Other
  // object-bound activities stay neutral when no authoritative context exists.
  if (
    input.legacyActivityLabel === 'Talk'
    && nearResident
    && input.canTalk
  ) {
    return 'talk';
  }

  return 'idle';
}
