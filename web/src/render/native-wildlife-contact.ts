import * as THREE from 'three';

export type WildlifePaw = { name: string; upper: string; sole_vertices?: number[] };
export type WildlifeContactClip = { stance: number; offsets: number[] };
type Ground = (x: number, z: number) => number | null;
type Foot = { paw: THREE.Bone; chain: THREE.Bone[]; indices: number[];
  anchor: THREE.Vector3; planted: boolean; key: string };

/** Correct only the source skin's visual joints. Core transforms stay immutable. */
export class NativeWildlifeContact {
  private readonly feet: Foot[] = [];
  private readonly authored = new Map<THREE.Bone, THREE.Quaternion>();
  private corrected = false;
  private support = 0;
  private readonly lastRoot = new THREE.Vector3(Infinity, 0, Infinity);
  private readonly inverse = new THREE.Matrix4();
  private readonly p = new THREE.Vector3();
  private readonly q = new THREE.Quaternion();
  private readonly parentQ = new THREE.Quaternion();
  private readonly delta = new THREE.Quaternion();
  private readonly a = new THREE.Vector3();
  private readonly b = new THREE.Vector3();
  private readonly pivot = new THREE.Vector3();
  private readonly axis = new THREE.Vector3();
  constructor(private readonly root: THREE.Group, private readonly frame: THREE.Group,
    private readonly skin: THREE.SkinnedMesh, definitions: WildlifePaw[]) {
    root.updateMatrixWorld(true); skin.skeleton.update();
    for (const definition of definitions) {
      const paw = skin.skeleton.bones.find(bone => bone.name === definition.name);
      if (!paw) throw new Error('Native foot joint missing');
      const chain: THREE.Bone[] = [];
      for (let bone = paw.parent; bone instanceof THREE.Bone; bone = bone.parent) {
        chain.push(bone); if (bone.name === definition.upper) break;
      }
      if (chain[chain.length - 1]?.name !== definition.upper || chain.length < 2 || chain.length > 3)
        throw new Error('Native limb chain missing');
      let candidates = definition.sole_vertices;
      if (!candidates) {
        const family = new Set<number>(); paw.traverse(object => {
          if (object instanceof THREE.Bone) family.add(skin.skeleton.bones.indexOf(object));
        });
        const joints = skin.geometry.getAttribute('skinIndex'), weights = skin.geometry.getAttribute('skinWeight');
        const used = skin.geometry.index ? new Set(Array.from(skin.geometry.index.array)) : null;
        const points: { index: number; y: number }[] = [];
        for (let index = 0; index < skin.geometry.getAttribute('position').count; index++) {
          if (used && !used.has(index)) continue;
          let weight = 0;
          for (let k = 0; k < 4; k++) if (family.has(joints.getComponent(index, k))) weight += weights.getComponent(index, k);
          if (weight > .3) { skin.getVertexPosition(index, this.p); points.push({ index, y: this.p.applyMatrix4(skin.matrixWorld).y }); }
        }
        points.sort((a, b) => a.y - b.y); candidates = points.slice(0, 24).map(p => p.index);
      }
      if (candidates.length < 3 || candidates.some(i => !Number.isInteger(i) || i < 0 || i >= skin.geometry.getAttribute('position').count))
        throw new Error('Native contact surface missing');
      // Retain extrema, rather than sampling duplicate UV-seam positions.
      const indices = new Set<number>();
      const points = candidates.map(index => { const p = new THREE.Vector3(); skin.getVertexPosition(index, p); return { index, p }; });
      for (const axis of ['x', 'y', 'z'] as const) {
        indices.add(points.reduce((a, b) => a.p[axis] < b.p[axis] ? a : b).index);
        indices.add(points.reduce((a, b) => a.p[axis] > b.p[axis] ? a : b).index);
      }
      for (const bone of [...chain, paw]) this.authored.set(bone, bone.quaternion.clone());
      this.feet.push({ paw, chain, indices: [...indices], anchor: new THREE.Vector3(), planted: false, key: '' });
    }
  }
  restore(): void {
    if (this.corrected) for (const [bone, quaternion] of this.authored) bone.quaternion.copy(quaternion);
    this.frame.position.y += this.support; this.support = 0; this.corrected = false;
  }
  apply(key: string, phase: number, clip: WildlifeContactClip, blending: boolean, scale: number, ground: Ground): void {
    for (const [bone, quaternion] of this.authored) quaternion.copy(bone.quaternion);
    this.corrected = true; this.root.updateMatrixWorld(true);
    const reset = this.lastRoot.distanceToSquared(this.root.position) > Math.max(1, 2 * scale) ** 2;
    this.lastRoot.copy(this.root.position); this.inverse.copy(this.frame.matrixWorld).invert();
    const moving = key === 'walk' || key === 'run' || key === 'hunt';
    // Support follows actual deformed soles and actual ground, including slopes.
    let gap = 0;
    if (!blending) for (let i = 0; i < this.feet.length; i++) {
      if (moving && (phase + clip.offsets[i]!) % 1 >= clip.stance) continue;
      let low = Infinity;
      for (const index of this.feet[i]!.indices) {
        this.skin.getVertexPosition(index, this.p); this.p.applyMatrix4(this.skin.matrixWorld);
        const h = ground(this.p.x, this.p.z); if (h !== null) low = Math.min(low, this.p.y - h);
      }
      if (Number.isFinite(low)) gap = Math.max(gap, low / scale);
    }
    this.support = THREE.MathUtils.clamp(gap, 0, .10); this.frame.position.y -= this.support;
    this.root.updateMatrixWorld(true); this.inverse.copy(this.frame.matrixWorld).invert();
    for (let i = 0; i < this.feet.length; i++) {
      const foot = this.feet[i]!;
      const contact = !moving || (phase + clip.offsets[i]!) % 1 < clip.stance;
      const points = foot.indices.map(index => {
        const p = new THREE.Vector3(); this.skin.getVertexPosition(index, p); return p.applyMatrix4(this.skin.matrixWorld);
      });
      const lift = contact && !blending ? 0 : Math.max(0, Math.min(...points.map(p => p.clone().applyMatrix4(this.inverse).y))) * scale;
      const authoredPosition = foot.paw.getWorldPosition(new THREE.Vector3());
      const orientation = foot.paw.getWorldQuaternion(new THREE.Quaternion());
      if (reset || foot.key !== key || blending || !contact) foot.planted = false;
      foot.key = key;
      if (contact && !blending && !foot.planted) { foot.anchor.copy(authoredPosition); foot.planted = true; }
      const goal = authoredPosition.clone();
      if (foot.planted && foot.anchor.distanceTo(authoredPosition) < .22 * scale) {
        goal.x = foot.anchor.x; goal.z = foot.anchor.z;
      } else foot.planted = false;
      const dx = goal.x - authoredPosition.x, dz = goal.z - authoredPosition.z;
      let error = Infinity;
      for (const p of points) { const h = ground(p.x + dx, p.z + dz); if (h !== null) error = Math.min(error, p.y - h); }
      if (!Number.isFinite(error)) continue;
      goal.y += THREE.MathUtils.clamp(lift - error, -.20 * scale, .20 * scale);
      for (let step = 0; step < 16; step++) {
        if (foot.paw.getWorldPosition(this.p).distanceToSquared(goal) < (.001 * scale) ** 2) break;
        for (const joint of foot.chain) {
          joint.getWorldPosition(this.pivot); foot.paw.getWorldPosition(this.a); this.a.sub(this.pivot);
          this.b.copy(goal).sub(this.pivot);
          if (this.a.lengthSq() < 1e-12 || this.b.lengthSq() < 1e-12) continue;
          this.a.normalize(); this.b.normalize(); this.axis.crossVectors(this.a, this.b);
          if (this.axis.lengthSq() < 1e-12) continue;
          this.delta.setFromAxisAngle(this.axis.normalize(), Math.min(.22, Math.acos(THREE.MathUtils.clamp(this.a.dot(this.b), -1, 1))));
          joint.getWorldQuaternion(this.q).premultiply(this.delta);
          joint.parent!.getWorldQuaternion(this.parentQ).invert(); joint.quaternion.copy(this.parentQ.multiply(this.q));
          joint.updateWorldMatrix(false, true);
        }
      }
      foot.paw.parent!.getWorldQuaternion(this.parentQ).invert(); foot.paw.quaternion.copy(this.parentQ.multiply(orientation)); foot.paw.updateWorldMatrix(false, true);
    }
    this.skin.skeleton.update();
  }
}
