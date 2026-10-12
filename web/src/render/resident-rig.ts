import * as THREE from 'three';

export const RESIDENT_JOINTS = {
  pelvis: ['pelvis', 'Hips', 'mixamorigHips'], spine: ['spine_02', 'Spine1', 'mixamorigSpine1'],
  neck: ['neck_01', 'Neck', 'mixamorigNeck'], head: ['Head', 'head', 'mixamorigHead'],
  leftShoulder: ['clavicle_l', 'LeftShoulder', 'mixamorigLeftShoulder'], rightShoulder: ['clavicle_r', 'RightShoulder', 'mixamorigRightShoulder'],
  leftArm: ['upperarm_l', 'LeftArm', 'mixamorigLeftArm'], rightArm: ['upperarm_r', 'RightArm', 'mixamorigRightArm'],
  leftForearm: ['lowerarm_l', 'LeftForeArm', 'mixamorigLeftForeArm'], rightForearm: ['lowerarm_r', 'RightForeArm', 'mixamorigRightForeArm'],
  leftHand: ['hand_l', 'LeftHand', 'mixamorigLeftHand'], rightHand: ['hand_r', 'RightHand', 'mixamorigRightHand'],
  leftThigh: ['thigh_l', 'LeftUpLeg', 'mixamorigLeftUpLeg'], rightThigh: ['thigh_r', 'RightUpLeg', 'mixamorigRightUpLeg'],
  leftCalf: ['calf_l', 'LeftLeg', 'mixamorigLeftLeg'], rightCalf: ['calf_r', 'RightLeg', 'mixamorigRightLeg'],
  leftFoot: ['foot_l', 'LeftFoot', 'mixamorigLeftFoot'], rightFoot: ['foot_r', 'RightFoot', 'mixamorigRightFoot'],
} as const;
export type ResidentJoint = keyof typeof RESIDENT_JOINTS;

/** Explicit semantic binding also accepts named Object3D pivots for separated meshes.
 * An unskinned single mesh is reported, never silently treated as deformable. */
export class ResidentRig {
  readonly joints = new Map<ResidentJoint, THREE.Object3D>();
  readonly rest = new Map<ResidentJoint, THREE.Quaternion>();
  readonly missing: ResidentJoint[] = [];
  readonly skinned: boolean;
  constructor(readonly model: THREE.Object3D) {
    const objects = new Map<string, THREE.Object3D>(); let skinned = false;
    model.traverse(object => { objects.set(object.name, object); if (object instanceof THREE.SkinnedMesh) skinned = true; });
    this.skinned = skinned;
    for (const joint of Object.keys(RESIDENT_JOINTS) as ResidentJoint[]) {
      const object = RESIDENT_JOINTS[joint].map(name => objects.get(name)).find(Boolean);
      if (object) { this.joints.set(joint, object); this.rest.set(joint, object.quaternion.clone()); }
      else this.missing.push(joint);
    }
  }
}

/** Allocation-free analytic two-bone IK. Preserves the animated bend plane,
 * clamps unreachable targets and applies only a bounded presentation weight. */
export class ResidentTwoBoneIK {
  private readonly a = new THREE.Vector3(); private readonly b = new THREE.Vector3();
  private readonly c = new THREE.Vector3(); private readonly direction = new THREE.Vector3();
  private readonly pole = new THREE.Vector3(); private readonly elbow = new THREE.Vector3();
  private readonly from = new THREE.Vector3(); private readonly to = new THREE.Vector3();
  private readonly q = new THREE.Quaternion(); private readonly world = new THREE.Quaternion();
  private readonly parent = new THREE.Quaternion(); private readonly local = new THREE.Quaternion();
  solve(upper: THREE.Object3D, lower: THREE.Object3D, tip: THREE.Object3D, target: THREE.Vector3, weight: number): boolean {
    if (!Number.isFinite(target.x + target.y + target.z) || weight <= 0 || lower.parent !== upper || tip.parent !== lower) return false;
    upper.getWorldPosition(this.a); lower.getWorldPosition(this.b); tip.getWorldPosition(this.c);
    const l1 = this.a.distanceTo(this.b), l2 = this.b.distanceTo(this.c);
    if (l1 < 1e-5 || l2 < 1e-5) return false;
    this.direction.subVectors(target, this.a); const distance = this.direction.length();
    if (distance < 1e-5) return false;
    this.direction.divideScalar(distance);
    const d = THREE.MathUtils.clamp(distance, Math.abs(l1-l2)+1e-5, (l1+l2)*.995);
    this.pole.subVectors(this.b, this.a).addScaledVector(this.direction, -this.pole.dot(this.direction));
    if (this.pole.lengthSq() < 1e-8) {
      // Deterministic pole for a straight limb; no random elbow flips.
      this.pole.set(0, 0, 1).addScaledVector(this.direction, -this.direction.z);
      if (this.pole.lengthSq() < 1e-8) this.pole.set(1, 0, 0);
    }
    this.pole.normalize();
    const along = (l1*l1-l2*l2+d*d)/(2*d);
    this.elbow.copy(this.a).addScaledVector(this.direction, along).addScaledVector(this.pole, Math.sqrt(Math.max(0,l1*l1-along*along)));
    this.rotate(upper, this.a, this.b, this.elbow, weight);
    lower.getWorldPosition(this.b); tip.getWorldPosition(this.c);
    this.rotate(lower, this.b, this.c, target, weight);
    return true;
  }
  private rotate(bone: THREE.Object3D, origin: THREE.Vector3, current: THREE.Vector3, target: THREE.Vector3, weight: number): void {
    this.from.subVectors(current, origin).normalize(); this.to.subVectors(target, origin).normalize();
    this.q.setFromUnitVectors(this.from, this.to); bone.getWorldQuaternion(this.world);
    this.world.premultiply(this.q);
    if (bone.parent) bone.parent.getWorldQuaternion(this.parent); else this.parent.identity();
    this.local.copy(this.parent).invert().multiply(this.world);
    bone.quaternion.slerp(this.local, THREE.MathUtils.clamp(weight, 0, 1)); bone.updateWorldMatrix(false, true);
  }
}
