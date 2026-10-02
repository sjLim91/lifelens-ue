import type { ResidentPresentationDirective } from '../runtime/core-types';

export type ResidentSemanticMotion =
  | 'idle' | 'walk' | 'carry' | 'sleep'
  | 'talk' | 'comfort' | 'reconcile' | 'teach' | 'care'
  | 'interact' | 'crouch' | 'wash' | 'work' | 'consume'
  | 'gatherWood' | 'gatherMineral' | 'gatherClay' | 'gatherFiber'
  | 'construct' | 'repair' | 'craft' | 'experiment'
  | 'fuel' | 'ignite' | 'loadFurnace' | 'collect' | 'store' | 'retrieve'
  | 'harvest' | 'plant' | 'water' | 'tend';

export interface ResidentSemanticMotionContext {
  moving: boolean;
  nearbyResident: boolean;
  hasCarriedLoad?: boolean;
  hasWaterContainer?: boolean;
}

export function residentSleepPostureActive(
  presentation: ResidentPresentationDirective | null | undefined,
  moving: boolean,
): boolean {
  return !moving && presentation?.active === true
    && presentation.kind === 'Physical' && presentation.phase === 'Interacting'
    && presentation.physicalGoal === 'Sleep';
}

/** Only Core facts select an action. Visual interpolation has priority over Interacting. */
export function resolveResidentSemanticMotion(
  presentation: ResidentPresentationDirective | null | undefined,
  context: ResidentSemanticMotionContext,
): ResidentSemanticMotion {
  if (context.moving || (presentation?.active && presentation.phase === 'Moving')) {
    if (presentation?.active && context.hasCarriedLoad === true
      && (presentation.kind === 'Trade' || (presentation.kind === 'Civilization'
        && (presentation.facilityAction === 'DeliverMaterial' || presentation.civilizationIntent === 'Store')))) return 'carry';
    return 'walk';
  }
  if (!presentation?.active || presentation.phase !== 'Interacting') return 'idle';
  switch (presentation.kind) {
    case 'Trade': return context.nearbyResident ? 'interact' : 'idle';
    case 'Social':
      if (!context.nearbyResident || presentation.socialIntent === 'Avoid') return 'idle';
      if (presentation.socialIntent === 'Comfort') return 'comfort';
      if (presentation.socialIntent === 'Repair') return 'reconcile';
      return 'talk';
    case 'KnowledgeTeaching': return context.nearbyResident ? 'teach' : 'idle';
    case 'Parenting':
      if (!context.nearbyResident) return 'idle';
      if (presentation.parentingAction === 'Educate' || presentation.parentingAction === 'Discipline') return 'teach';
      if (presentation.parentingAction === 'Comfort' || presentation.parentingAction === 'PutToSleep') return 'comfort';
      if (presentation.parentingAction === 'Play') return 'talk';
      return 'care';
    case 'Physical':
      if (presentation.physicalGoal === 'Eat' || presentation.physicalGoal === 'Drink') return 'consume';
      if (presentation.physicalGoal === 'Sleep') return 'sleep';
      if (presentation.physicalGoal === 'UseToilet' && (presentation.designatedSanitationSite === true
        || (presentation.emergencyFallback === true && presentation.hasTargetGrid === true)
        || (presentation.hasObjectTarget === true && presentation.objectKind === 'Toilet'))) return 'crouch';
      if (presentation.physicalGoal === 'Wash' && ((presentation.hasObjectTarget === true && presentation.objectKind === 'Sink')
        || presentation.hasTargetGrid === true)) return presentation.directNaturalWaterSource ? 'wash' : 'interact';
      return 'idle';
    case 'Civilization':
      if (!presentation.hasTargetGrid) return 'idle';
      if (presentation.facilityKind === 'CultivatedPlot') {
        if (presentation.facilityAction === 'Plant') return 'plant';
        if (presentation.facilityAction === 'Water') return context.hasWaterContainer ? 'water' : 'interact';
        if (presentation.facilityAction === 'Harvest') return 'harvest';
        if (presentation.facilityAction === 'Tend') return 'tend';
      }
      if (presentation.facilityAction === 'Work') return 'construct';
      if (presentation.facilityAction === 'Repair') return 'repair';
      if (presentation.facilityAction === 'Fuel') return 'fuel';
      if (presentation.facilityAction === 'Ignite') return 'ignite';
      if (presentation.facilityAction === 'LoadSmeltCharge') return 'loadFurnace';
      if (presentation.facilityAction === 'CollectCharcoal' || presentation.facilityAction === 'CollectMetal') return 'collect';
      if (presentation.facilityAction === 'DeliverMaterial') return 'store';
      if (presentation.civilizationIntent === 'Gather') {
        switch (presentation.civilizationMaterial) {
          case 'Wood': return 'gatherWood';
          case 'Stone': case 'Flint': case 'CopperOre': case 'TinOre': return 'gatherMineral';
          case 'Clay': return 'gatherClay';
          case 'Fiber': return 'gatherFiber';
          case 'PlantFood': return 'harvest';
          default: return 'interact';
        }
      }
      if (presentation.civilizationIntent === 'Craft') return 'craft';
      if (presentation.civilizationIntent === 'Experiment') return 'experiment';
      if (presentation.civilizationIntent === 'Store') return 'store';
      if (presentation.civilizationIntent === 'Retrieve') return 'retrieve';
      return 'idle';
    default: return 'idle';
  }
}
