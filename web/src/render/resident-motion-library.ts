import * as THREE from 'three';
import type { ResidentSemanticMotion } from './resident-semantic-motion';

/** Names verified from the pinned binary GLBs. Never substring-match weapon/death clips. */
export function createResidentMotionLibrary(clips: readonly THREE.AnimationClip[]): Map<ResidentSemanticMotion, THREE.AnimationClip> {
  const exact = new Map(clips.map(clip => [clip.name, clip]));
  const motions = new Map<ResidentSemanticMotion, THREE.AnimationClip>();
  const bind = (motion: ResidentSemanticMotion, ...names: string[]): void => {
    const clip = names.map(name => exact.get(name)).find(Boolean);
    if (clip) motions.set(motion, clip);
  };
  bind('idle', 'Idle_Loop'); bind('walk', 'Walk_Loop'); bind('carry', 'Walk_Carry_Loop', 'Walk_Loop');
  bind('talk', 'Idle_Talking_Loop', 'Idle_Loop'); bind('reconcile', 'Idle_Talking_Loop', 'Idle_Loop');
  bind('comfort', 'Yes', 'Idle_Talking_Loop', 'Idle_Loop');
  bind('interact', 'Interact', 'Idle_Loop'); bind('crouch', 'Crouch_Idle_Loop', 'Idle_Loop');
  bind('consume', 'Consume', 'Interact', 'Idle_Loop');
  bind('work', 'Fixing_Kneeling', 'Interact', 'Idle_Loop');
  bind('construct', 'Fixing_Kneeling', 'Interact', 'Idle_Loop');
  bind('repair', 'Fixing_Kneeling', 'Interact', 'Idle_Loop');
  bind('gatherMineral', 'Fixing_Kneeling', 'Crouch_Idle_Loop', 'Idle_Loop');
  bind('gatherClay', 'Farm_PlantSeed', 'Fixing_Kneeling', 'Crouch_Idle_Loop');
  bind('plant', 'Farm_PlantSeed', 'Fixing_Kneeling', 'Crouch_Idle_Loop');
  bind('water', 'Farm_Watering', 'Interact', 'Idle_Loop');
  bind('harvest', 'Farm_Harvest', 'Fixing_Kneeling', 'Crouch_Idle_Loop');
  bind('collect', 'Farm_Harvest', 'Fixing_Kneeling', 'Crouch_Idle_Loop');
  bind('store', 'Farm_PlantSeed', 'Interact', 'Idle_Loop'); bind('retrieve', 'Farm_Harvest', 'Interact', 'Idle_Loop');
  bind('experiment', 'Idle_FoldArms_Loop', 'Interact', 'Idle_Loop');

  // Layer only real bone tracks: ground handwork has a crouched lower body,
  // while craft/teaching remain upright. No tool, partner or outcome is invented.
  const compose = (name: string, lowerName: string, upperName: string): THREE.AnimationClip | undefined => {
    const lower = exact.get(lowerName), upper = exact.get(upperName);
    if (!lower || !upper) return undefined;
    const isUpper = (track: THREE.KeyframeTrack): boolean => /^(spine_|neck_|Head\.|clavicle_|upperarm_|lowerarm_|hand_|index_|middle_|pinky_|ring_|thumb_)/.test(track.name);
    return new THREE.AnimationClip(name, Math.max(lower.duration, upper.duration), [
      ...lower.tracks.filter(track => !isUpper(track)), ...upper.tracks.filter(isUpper),
    ]);
  };
  const groundHands = compose('LifeLens_GroundHands', 'Crouch_Idle_Loop', exact.has('Farm_PlantSeed') ? 'Farm_PlantSeed' : 'Fixing_Kneeling');
  const groundFixing = compose('LifeLens_GroundFixing', 'Crouch_Idle_Loop', 'Fixing_Kneeling');
  const standingFixing = compose('LifeLens_StandingFixing', 'Idle_Loop', 'Fixing_Kneeling');
  const lowHarvest = compose('LifeLens_LowHarvest', 'Crouch_Idle_Loop', 'Farm_Harvest');
  const standingHands = compose('LifeLens_StandingHands', 'Idle_Loop', 'Interact');
  for (const motion of ['wash','care','fuel','ignite','loadFurnace'] as const) {
    if (groundHands) motions.set(motion, groundHands); else bind(motion, 'Fixing_Kneeling', 'Crouch_Idle_Loop', 'Idle_Loop');
  }
  for (const motion of ['gatherFiber','tend'] as const) {
    if (lowHarvest) motions.set(motion, lowHarvest); else if (groundHands) motions.set(motion, groundHands); else bind(motion, 'Fixing_Kneeling', 'Crouch_Idle_Loop');
  }
  if (standingHands) motions.set('teach', standingHands); else bind('teach', 'Idle_Talking_Loop', 'Interact');
  if (standingFixing) motions.set('craft', standingFixing); else bind('craft', 'Fixing_Kneeling', 'Interact');
  if (groundFixing) { motions.set('repair', groundFixing); motions.set('ignite', groundFixing); }
  // Wood is collected by hand. Even an owned axe does not prove it was equipped
  // or used by Core: TreeChopping_Loop is deliberately not selected.
  const reach = compose('LifeLens_WoodReach', 'Idle_Loop', exact.has('Farm_Harvest') ? 'Farm_Harvest' : 'Fixing_Kneeling');
  if (reach) {
    const rotation = new THREE.Quaternion().setFromEuler(new THREE.Euler(0, 0, -.25));
    for (const track of reach.tracks) {
      if (track.name !== 'upperarm_r.quaternion') continue;
      const adjusted = track.clone(), q = new THREE.Quaternion();
      for (let i = 0; i < adjusted.values.length; i += 4) {
        q.fromArray(adjusted.values, i).multiply(rotation).toArray(adjusted.values, i);
      }
      reach.tracks[reach.tracks.indexOf(track)] = adjusted;
    }
    motions.set('gatherWood', reach);
  } else bind('gatherWood', 'Interact', 'Idle_Loop');
  return motions;
}

/** Relative gesture tempo. Locomotion is separately tied to actual velocity. */
export function residentGestureRate(motion: ResidentSemanticMotion): number {
  switch (motion) {
    case 'comfort': case 'reconcile': return .65;
    case 'repair': case 'gatherClay': case 'care': return .8;
    case 'ignite': return .6;
    case 'loadFurnace': return .9;
    case 'teach': return .85;
    default: return 1;
  }
}
