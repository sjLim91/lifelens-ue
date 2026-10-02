import * as THREE from 'three';
import { RESIDENT_SLEEP_ENDPOINTS } from './resident-sleep-pose';

export function residentSleepFallbackClip(): THREE.AnimationClip {
  return new THREE.AnimationClip('LifeLens_CC0_SleepFallback', RESIDENT_SLEEP_ENDPOINTS.duration,
    RESIDENT_SLEEP_ENDPOINTS.tracks.map(track => {
      const type = track.type === 'quaternion' ? THREE.QuaternionKeyframeTrack : THREE.VectorKeyframeTrack;
      return new type(track.name, [...track.times], [...track.values]);
    }));
}

export function residentStandingFallbackClip(): THREE.AnimationClip {
  const clip = residentSleepFallbackClip();
  clip.name = 'LifeLens_CC0_IdleFallback';
  for (const track of clip.tracks) {
    const size = track.getValueSize();
    track.values = track.values.slice(size);
    track.times = new Float32Array([0]);
  }
  return clip;
}

export interface ResidentSleepCalibration {
  duration: number;
  offsets: Float32Array;
}

/** Once per shared model/library, never per actor or frame. Measure skinned vertices,
 * not the rest-pose bounding box. X/Z recentering removes the clip's body drift. */
export function calibrateResidentSleep(model: THREE.Group, clip: THREE.AnimationClip): ResidentSleepCalibration {
  const mixer = new THREE.AnimationMixer(model);
  const action = mixer.clipAction(clip).setLoop(THREE.LoopOnce, 1);
  action.clampWhenFinished = true;
  action.play();
  const samples = 97, offsets = new Float32Array(samples * 3);
  const point = new THREE.Vector3(), box = new THREE.Box3();
  const inverse = new THREE.Matrix4();
  const meshes: THREE.Mesh[] = [];
  model.traverse(object => { if (object instanceof THREE.Mesh) meshes.push(object); });
  for (let sample = 0; sample < samples; sample++) {
    action.paused = true; action.time = clip.duration * sample / (samples - 1);
    mixer.update(0); model.updateMatrixWorld(true);
    inverse.identity();
    if (model.parent) inverse.copy(model.parent.matrixWorld).invert();
    box.makeEmpty();
    for (const mesh of meshes) {
      if (mesh instanceof THREE.SkinnedMesh) mesh.skeleton.update();
      const positions = mesh.geometry.getAttribute('position');
      for (let vertex = 0; vertex < positions.count; vertex++) {
        point.fromBufferAttribute(positions, vertex);
        if (mesh instanceof THREE.SkinnedMesh) mesh.applyBoneTransform(vertex, point);
        point.applyMatrix4(mesh.matrixWorld).applyMatrix4(inverse);
        box.expandByPoint(point);
      }
    }
    offsets[sample * 3] = -(box.min.x + box.max.x) * .5;
    offsets[sample * 3 + 1] = -box.min.y;
    offsets[sample * 3 + 2] = -(box.min.z + box.max.z) * .5;
  }
  // Standing endpoint already matches the normalized source anchor. Preserve it.
  const endX = offsets[offsets.length - 3], endZ = offsets[offsets.length - 1];
  for (let sample = 0; sample < samples; sample++) {
    offsets[sample * 3] -= endX; offsets[sample * 3 + 2] -= endZ;
  }
  mixer.stopAllAction(); mixer.uncacheRoot(model);
  return { duration: clip.duration, offsets };
}

export class ResidentSleepMotion {
  private engaged = false;
  private time: number;
  private support = 0;
  private releasing = false;
  constructor(private readonly action: THREE.AnimationAction,
    private readonly calibration: ResidentSleepCalibration,
    private readonly visual: THREE.Group) {
    this.time = calibration.duration;
    action.setLoop(THREE.LoopOnce, 1); action.clampWhenFinished = true;
  }
  get active(): boolean { return this.engaged; }
  get resting(): boolean { return this.engaged && this.time === 0; }

  /** Explicit bounded presentation sequence. It never delays or changes Core movement. */
  update(sleeping: boolean, dt: number, supportWorld: number, heightWorld: number,
    mixer: THREE.AnimationMixer, actions: Iterable<THREE.AnimationAction>): boolean {
    if (sleeping && !this.engaged) {
      for (const action of actions) action.stop();
      this.engaged = true; this.releasing = false; this.time = this.calibration.duration;
      this.support = supportWorld;
      this.action.reset().setEffectiveWeight(1).play(); this.action.paused = true;
    }
    if (!this.engaged) return false;
    if (sleeping) {
      this.releasing = false;
      this.time = Math.max(0, this.time - Math.max(0, dt));
    } else {
      this.time = Math.min(this.calibration.duration, this.time + Math.max(0, dt));
    }
    this.action.time = this.time;
    this.action.enabled = true; this.action.paused = true;
    mixer.update(0);
    const data = this.calibration.offsets, count = data.length / 3;
    const index = this.time / this.calibration.duration * (count - 1);
    const lo = Math.min(count - 2, Math.floor(index)), blend = index - lo;
    const at = (axis: number): number => data[lo * 3 + axis] * (1 - blend) + data[(lo + 1) * 3 + axis] * blend;
    // Clearance covers interpolation and small appearance deformation. Latch the
    // support throughout rest/wake so snapshot refreshes cannot bob the body.
    const height = Math.max(.001, heightWorld);
    const lift = at(1) + (.045 + this.support) / height;
    this.visual.position.set(at(0), lift, at(2));
    this.visual.rotation.set(0, 0, 0);
    if (!sleeping && this.time === this.calibration.duration) {
      // Settle the upright visual offset before releasing the action. Avoid a
      // last-frame teleport from a raised bed/slope anchor back to ground.
      this.releasing = true;
      this.support = Math.max(0, this.support - Math.max(0, dt) * height * 2);
      this.visual.position.y = at(1) + this.support / height;
      if (this.support === 0 && this.releasing) {
        this.action.stop(); this.engaged = false; this.visual.position.set(0, 0, 0);
      }
    }
    return true;
  }
}
