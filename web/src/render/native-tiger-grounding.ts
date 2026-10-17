import * as THREE from 'three';
import type { TigerPose } from './imported-tiger';

type Ground = (x: number, z: number) => number | null;
export type NativeTigerPaw = { name: string; upper: string; sole_vertices: number[] };
type Foot = { paw: THREE.Bone; chain: THREE.Bone[]; indices: number[]; points: THREE.Vector3[];
  anchor: THREE.Vector3; planted: boolean; key: string };

/** Applies terrain/stance correction to the existing imported bones only. */
export class NativeTigerGrounding {
  private readonly feet: Foot[] = [];
  private readonly skin: THREE.SkinnedMesh;
  private readonly authored = new Map<THREE.Bone, THREE.Quaternion>();
  private corrected = false;
  private supportDrop = 0;
  private readonly inverseFrame = new THREE.Matrix4();
  private readonly previousRoot = new THREE.Vector3(Infinity, 0, Infinity);
  private readonly point = new THREE.Vector3();
  private readonly position = new THREE.Vector3();
  private readonly goal = new THREE.Vector3();
  private readonly nativePosition = new THREE.Vector3();
  private readonly pivot = new THREE.Vector3();
  private readonly a = new THREE.Vector3();
  private readonly b = new THREE.Vector3();
  private readonly axis = new THREE.Vector3();
  private readonly q = new THREE.Quaternion();
  private readonly parentQ = new THREE.Quaternion();
  private readonly deltaQ = new THREE.Quaternion();
  private readonly orientation = new THREE.Quaternion();

  constructor(private readonly root: THREE.Group, private readonly frame: THREE.Group,
    model: THREE.Object3D, definitions: NativeTigerPaw[]) {
    const skins: THREE.SkinnedMesh[] = [];
    model.traverse(object => { if (object instanceof THREE.SkinnedMesh) skins.push(object); });
    if (!skins[0]) throw new Error('Native tiger skin missing');
    this.skin = skins[0]; root.updateWorldMatrix(true, false); root.updateMatrixWorld(true); this.skin.skeleton.update();
    for (const definition of definitions) {
      const paw = model.getObjectByName(definition.name);
      if (!(paw instanceof THREE.Bone)) throw new Error('Native paw joint missing');
      const chain: THREE.Bone[] = [];
      for (let parent = paw.parent; parent instanceof THREE.Bone; parent = parent.parent) {
        chain.push(parent); if (parent.name === definition.upper) break;
      }
      if (chain[chain.length - 1]?.name !== definition.upper || chain.length < 2 || chain.length > 3)
        throw new Error('Native limb chain changed');
      const candidates = definition.sole_vertices.map(index => {
        const point = new THREE.Vector3(); this.skin.getVertexPosition(index, point);
        return { index, point };
      });
      const indices = new Set<number>();
      for (const axis of ['x', 'y', 'z'] as const) {
        indices.add(candidates.reduce((a, b) => a.point[axis] < b.point[axis] ? a : b).index);
        indices.add(candidates.reduce((a, b) => a.point[axis] > b.point[axis] ? a : b).index);
      }
      indices.add(candidates[Math.floor(candidates.length / 3)].index);
      indices.add(candidates[Math.floor(candidates.length * 2 / 3)].index);
      for (const joint of [...chain, paw]) this.authored.set(joint, joint.quaternion.clone());
      this.feet.push({ paw, chain, indices: [...indices], points: [...indices].map(() => new THREE.Vector3()),
        anchor: new THREE.Vector3(), planted: false, key: '' });
    }
  }

  restore(): void {
    // AnimationMixer may skip unchanged constant tracks. Restore the authored
    // pose before its next evaluation so this IK overlay cannot accumulate.
    if (this.corrected) for (const [bone, quaternion] of this.authored) bone.quaternion.copy(quaternion);
    this.frame.position.y += this.supportDrop;
    this.supportDrop = 0;
    this.corrected = false;
  }

  apply(pose: TigerPose, key: string, phase: number, blending: boolean, ground: Ground): void {
    for (const [bone, quaternion] of this.authored) quaternion.copy(bone.quaternion);
    this.corrected = true;
    this.root.updateWorldMatrix(true, false); this.root.updateMatrixWorld(true);
    const rebased = this.previousRoot.distanceToSquared(this.root.position) > Math.max(1, pose.scale * 2) ** 2;
    this.previousRoot.copy(this.root.position);
    this.inverseFrame.copy(this.frame.matrixWorld).invert();
    const moving = key === 'walk' || key === 'hunt' || key === 'run';
    const offsets = key === 'run' ? [.32, .38, .84, .90] : [.25, .75, 0, .5];
    const stance = key === 'run' ? .36 : key === 'hunt' ? .70 : .62;
    // Let the shoulders settle when a planted, almost straight limb cannot
    // reach the slope. Keep this visual offset on the model frame, never on
    // the authoritative actor root, and remove it before the next evaluation.
    let supportGap = 0;
    if (!blending) for (let index = 0; index < this.feet.length; index++) {
      if (moving && ((phase + offsets[index]) % 1) >= stance) continue;
      let gap = Infinity;
      for (const vertex of this.feet[index].indices) {
        this.skin.getVertexPosition(vertex, this.point); this.point.applyMatrix4(this.skin.matrixWorld);
        const height = ground(this.point.x, this.point.z);
        if (height !== null) gap = Math.min(gap, this.point.y - height);
      }
      if (Number.isFinite(gap)) supportGap = Math.max(supportGap, gap);
    }
    this.supportDrop = Math.min(.08, Math.max(0, supportGap / pose.scale));
    this.frame.position.y -= this.supportDrop;
    this.root.updateMatrixWorld(true);
    this.inverseFrame.copy(this.frame.matrixWorld).invert();
    for (let index = 0; index < this.feet.length; index++) {
      const foot = this.feet[index];
      let lift = Infinity;
      for (let i = 0; i < foot.indices.length; i++) {
        const point = foot.points[i]; this.skin.getVertexPosition(foot.indices[i], point);
        point.applyMatrix4(this.skin.matrixWorld);
        lift = Math.min(lift, this.point.copy(point).applyMatrix4(this.inverseFrame).y);
      }
      lift = Math.max(0, lift) * pose.scale;
      foot.paw.getWorldPosition(this.position); foot.paw.getWorldQuaternion(this.orientation);
      this.nativePosition.copy(this.position);
      const contact = !moving || ((phase + offsets[index]) % 1) < stance;
      // Authored IK can retain a small residual on a nearly extended limb;
      // that residual is not an airborne phase and must not become a lift.
      if (contact && !blending) lift = 0;
      if (rebased || foot.key !== key || blending || !contact) foot.planted = false;
      foot.key = key;
      this.goal.copy(this.position);
      if (contact && !blending) {
        if (!foot.planted) { foot.anchor.copy(this.position); foot.planted = true; }
        if (Math.hypot(foot.anchor.x - this.position.x, foot.anchor.z - this.position.z) > .22 * pose.scale)
          foot.anchor.copy(this.position);
        this.goal.x = foot.anchor.x; this.goal.z = foot.anchor.z;
      }
      let error = Infinity;
      const dx = this.goal.x - this.position.x, dz = this.goal.z - this.position.z;
      for (const point of foot.points) {
        const height = ground(point.x + dx, point.z + dz);
        if (height !== null) error = Math.min(error, point.y - height);
      }
      if (!Number.isFinite(error)) continue;
      this.goal.y += THREE.MathUtils.clamp(lift - error, -.18 * pose.scale, .18 * pose.scale);
      this.solve(foot, this.goal, pose.scale);
      // A sharp turn can put the old planted location outside the limb's
      // reach. Release that anchor rather than stretch the leg or leave the
      // paw floating. Re-evaluate from this frame's authored pose.
      foot.paw.getWorldPosition(this.position);
      if (foot.planted && this.position.distanceTo(this.goal) > .012 * pose.scale) {
        for (const joint of [...foot.chain, foot.paw]) joint.quaternion.copy(this.authored.get(joint)!);
        foot.chain[foot.chain.length - 1]!.updateWorldMatrix(false, true);
        foot.planted = false;
        this.goal.copy(this.nativePosition);
        let nativeError = Infinity;
        for (const point of foot.points) {
          const height = ground(point.x, point.z);
          if (height !== null) nativeError = Math.min(nativeError, point.y - height);
        }
        if (Number.isFinite(nativeError)) {
          this.goal.y += THREE.MathUtils.clamp(lift - nativeError, -.18 * pose.scale, .18 * pose.scale);
          this.solve(foot, this.goal, pose.scale);
        }
      }
    }
    this.skin.skeleton.update();
  }

  private solve(foot: Foot, goal: THREE.Vector3, scale: number): void {
    for (let iteration = 0; iteration < 16; iteration++) {
      foot.paw.getWorldPosition(this.position);
      if (this.position.distanceToSquared(goal) < (.001 * scale) ** 2) break;
      for (const joint of foot.chain) {
        joint.getWorldPosition(this.pivot); foot.paw.getWorldPosition(this.a); this.a.sub(this.pivot);
        this.b.copy(goal).sub(this.pivot);
        if (this.a.lengthSq() < 1e-12 || this.b.lengthSq() < 1e-12) continue;
        this.a.normalize(); this.b.normalize(); this.axis.crossVectors(this.a, this.b);
        if (this.axis.lengthSq() < 1e-12) continue;
        const angle = Math.min(.22, Math.acos(THREE.MathUtils.clamp(this.a.dot(this.b), -1, 1)));
        this.deltaQ.setFromAxisAngle(this.axis.normalize(), angle);
        joint.getWorldQuaternion(this.q); this.q.premultiply(this.deltaQ);
        joint.parent!.getWorldQuaternion(this.parentQ).invert();
        joint.quaternion.copy(this.parentQ.multiply(this.q)); joint.updateWorldMatrix(false, true);
      }
    }
    foot.paw.parent!.getWorldQuaternion(this.parentQ).invert();
    foot.paw.quaternion.copy(this.parentQ.multiply(this.orientation)); foot.paw.updateWorldMatrix(false, true);
  }
}
