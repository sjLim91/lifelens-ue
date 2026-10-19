import type { WildlifeAnimalObservation, WildlifeSpecies } from '../runtime/core-types';

type Behavior = WildlifeAnimalObservation['behavior'];
export type WildlifeRigClip =
  'idle' | 'walk' | 'run' | 'hunt' | 'forage' | 'drink' | 'rest' | 'swim';

/**
 * Only interpolate movement between positions supplied by the Core.
 * A predator pursuit or genuine flee should be visibly faster than quiet
 * foraging, without inventing any new destination, attack or animal action.
 */
export function wildlifeVisualTravelMultiplier(behavior: Behavior): number {
  switch (behavior) {
    case 'Flee': return 2.2;
    case 'Hunt': return 1.6;
    case 'Forage': return 0.75;
    default: return 1;
  }
}

/**
 * One source of truth for both native four-species and the independently
 * rigged tiger. A fleeing animal never uses the gentle walk cycle if it is
 * actually translating; a stationary animal does NOT run on the spot.
 * The original bone animations and ground-contact IK remain authoritative.
 */
export function wildlifeRigClip(
  species: WildlifeSpecies, behavior: Behavior, speed: number, scale: number,
): WildlifeRigClip {
  const moving = Number.isFinite(speed) && speed > .005;
  if (species === 'FreshwaterFish')
    return behavior === 'Swim' || moving ? 'swim' : 'idle';
  if (moving) {
    if (behavior === 'Flee') return 'run';
    if (behavior === 'Hunt') return 'hunt';
    if (Number.isFinite(scale) && scale > 0 && speed / scale > 1.5)
      return 'run';
    return 'walk';
  }
  if (behavior === 'Rest') return 'rest';
  if (behavior === 'Forage') return 'forage';
  if (behavior === 'Drink') return 'drink';
  return 'idle';
}
