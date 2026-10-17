import * as THREE from 'three';
import { GLTFLoader, type GLTF } from 'three/addons/loaders/GLTFLoader.js';
import { clone } from 'three/addons/utils/SkeletonUtils.js';
import { ImportedTigerFar, nativeTigerFarMesh } from './imported-tiger-far';
import { NativeTigerGrounding, type NativeTigerPaw } from './native-tiger-grounding';
import type { WildlifeAnimalObservation } from '../runtime/core-types';

type Tier = 'near' | 'mid' | 'far';
type Clip = { name: string; duration: number; cycle_distance: number };
type Contract = {
  version: number; tier: Tier; units: string; forward: string; up: string;
  creator: string; license: string; source_sha256: string;
  clips: Record<string, Clip>; paws: NativeTigerPaw[]; bone_count: number;
  status: string; render_mode?: string; head_joint?: string;
};
type Asset = { gltf: GLTF; contract: Contract };
type Actor = {
  tier: Tier; root: THREE.Group; frame: THREE.Group; model: THREE.Object3D; mixer: THREE.AnimationMixer;
  actions: Map<string, THREE.AnimationAction>; current: string;
  grounded: boolean; oriented: boolean; blendRemaining: number; feet: NativeTigerGrounding;
};
type GroundHeight = (x: number, z: number) => number | null;
export type TigerPose = {
  id: string; x: number; y: number; z: number; heading: number; scale: number;
  speed: number; phase: number; behavior: WildlifeAnimalObservation['behavior'];
};
export type TigerAssetLoader = (tier: Tier) => Promise<GLTF>;
const SOURCE_HASH = '6e152d1d98c6149d4241c292bf21cbc9e8ab16f81a456ea1515d174f7f7d1e40';
const CLIPS = ['idle', 'walk', 'run', 'hunt', 'forage', 'drink', 'rest'];

function inspect(gltf: GLTF, tier: Tier): Asset {
  const contract = gltf.parser.json.asset?.extras?.lifelensTiger as Contract | undefined;
  if (!contract || contract.version !== 2 || contract.tier !== tier
    || contract.units !== 'metres' || contract.forward !== '+Z' || contract.up !== '+Y'
    || contract.license !== 'CC-BY-4.0' || contract.creator !== 'Amil (francescolima74)'
    || contract.source_sha256 !== SOURCE_HASH || contract.status === 'INSPECTION_ONLY'
    || gltf.parser.json.asset?.extras?.lifelensInspectionOnly) throw new Error('Unrecognized tiger asset');
  if (tier === 'far') {
    if (contract.render_mode !== 'instanced-static' || contract.bone_count !== 0) throw new Error('Far rig must be baked');
    for (const pose of ['Idle', 'Rest', 'Forage']) {
      const mesh = nativeTigerFarMesh(gltf, `LifeLensTigerFar${pose}`);
      if (!mesh || mesh instanceof THREE.SkinnedMesh || !mesh.geometry.getAttribute('_lifelens_leg')
        || !(mesh.material instanceof THREE.MeshStandardMaterial)
        || !mesh.geometry.index || mesh.geometry.index.count > 3300) throw new Error('Far source pose or budget changed');
    }
    return { gltf, contract };
  }
  const skins: THREE.SkinnedMesh[] = [];
  gltf.scene.traverse(object => { if (object instanceof THREE.SkinnedMesh) skins.push(object); });
  const skin = skins[0];
  if (!skin || skin.skeleton.bones.length !== 47 || contract.bone_count !== 47
    || !skin.geometry.getAttribute('skinWeight') || !skin.geometry.index
    || skin.geometry.index.count > (tier === 'near' ? 15000 : 10500)
    || CLIPS.some(key => {
      const clip = contract.clips?.[key];
      return !clip || !Number.isFinite(clip.duration) || clip.duration <= 0
        || !Number.isFinite(clip.cycle_distance) || clip.cycle_distance < 0
        || !gltf.animations.some(animation => animation.name === clip.name && animation.tracks.length > 0);
    })) throw new Error('Tiger skin or animation contract missing');
  const bones = new Set(skin.skeleton.bones.map(bone => bone.name));
  const vertices = skin.geometry.getAttribute('position').count;
  if (!Array.isArray(contract.paws) || contract.paws.length !== 4 || contract.paws.some(paw =>
    !bones.has(paw.name) || !bones.has(paw.upper) || !Array.isArray(paw.sole_vertices)
    || paw.sole_vertices.length < 8 || paw.sole_vertices.some(index => !Number.isInteger(index) || index < 0 || index >= vertices)))
    throw new Error('Native paw contact metadata missing');
  return { gltf, contract };
}

function disposeAsset(gltf: GLTF): void {
  const geometries = new Set<THREE.BufferGeometry>();
  const materials = new Set<THREE.Material>();
  const textures = new Set<THREE.Texture>();
  for (const scene of new Set([gltf.scene, ...gltf.scenes])) scene.traverse(object => {
    if (!(object instanceof THREE.Mesh)) return;
    geometries.add(object.geometry);
    for (const material of Array.isArray(object.material) ? object.material : [object.material]) {
      materials.add(material);
      for (const value of Object.values(material)) if (value instanceof THREE.Texture) textures.add(value);
    }
    if (object instanceof THREE.SkinnedMesh) object.skeleton.dispose();
  });
  geometries.forEach(value => value.dispose());
  materials.forEach(value => value.dispose());
  textures.forEach(value => { value.dispose(); const source = value.source.data;
    if (typeof ImageBitmap !== 'undefined' && source instanceof ImageBitmap) source.close(); });
}

/** Presentation only. Core IDs, transforms and behaviour come from WildlifeLayer.
 * A failed/incomplete asset leaves that ID in the existing instanced fallback.
 * Two detailed plus four reduced skins bound mobile cost; far stays instanced.
 */
export class ImportedTigerPool {
  readonly group = new THREE.Group();
  private readonly assets = new Map<Tier, Asset>();
  private readonly requested = new Set<Tier>();
  private readonly actors = new Map<string, Actor>();
  private readonly used = new Set<string>();
  private far: ImportedTigerFar | null = null;
  private nearCount = 0;
  private midCount = 0;
  private disposed = false;
  private readonly normal = new THREE.Vector3();
  private readonly up = new THREE.Vector3(0, 1, 0);
  private readonly headingInverse = new THREE.Quaternion();
  private readonly groundRotation = new THREE.Quaternion();
  constructor(private readonly load: TigerAssetLoader | null = typeof document === 'undefined' ? null : tier => {
    const name = tier === 'near' ? 'tiger-rigged.glb' : `tiger-${tier}.glb`;
    return new GLTFLoader().loadAsync(new URL(`vendor/wildlife/${name}`, document.baseURI).href);
  }) { this.group.name = 'LifeLensImportedTigers'; }

  begin(): void { this.used.clear(); this.nearCount = 0; this.midCount = 0; this.far?.begin(); }

  draw(pose: TigerPose, tier: Tier, dt: number, ground?: GroundHeight): boolean {
    if (this.disposed || (tier === 'near' && this.nearCount >= 2) || (tier === 'mid' && this.midCount >= 4)) return false;
    const asset = this.assets.get(tier);
    if (!asset) { this.request(tier); return false; }
    if (tier === 'far') {
      this.far ??= new ImportedTigerFar(asset.gltf);
      const previous = this.actors.get(pose.id);
      const action = previous?.actions.get(previous.current);
      const phase = action ? action.time / action.getClip().duration * Math.PI * 2 : pose.phase;
      return this.far.draw({ ...pose, phase }, dt, ground);
    }
    let actor = this.actors.get(pose.id);
    const previousHeading = actor?.root.rotation.y;
    let phase = this.far?.phaseFor(pose.id) ?? pose.phase / (Math.PI * 2);
    if (actor && actor.tier !== tier) {
      const action = actor.actions.get(actor.current);
      if (action) phase = action.time / action.getClip().duration;
      this.remove(pose.id); actor = undefined;
    }
    if (!actor) {
      const root = new THREE.Group(); root.name = `LifeLensTiger:${pose.id}`;
      root.userData.coreId = pose.id;
      const model = clone(asset.gltf.scene);
      model.traverse(object => {
        if (object instanceof THREE.Mesh) { object.castShadow = true; object.receiveShadow = true;
          // Bounds are animated; at most six skins, so avoid static rest-box culling.
          object.frustumCulled = false; }
      });
      const frame = new THREE.Group(); frame.name = 'LifeLensTigerGroundFrame';
      frame.add(model); root.add(frame); this.group.add(root);
      const mixer = new THREE.AnimationMixer(model);
      const actions = new Map<string, THREE.AnimationAction>();
      for (const key of CLIPS) {
        const clip = asset.gltf.animations.find(value => value.name === asset.contract.clips[key].name)!;
        actions.set(key, mixer.clipAction(clip));
      }
      root.rotation.y = previousHeading ?? pose.heading;
      actor = { tier, root, frame, model, mixer, actions, current: '', grounded: false,
        oriented: previousHeading !== undefined, blendRemaining: 0,
        feet: new NativeTigerGrounding(root, frame, model, asset.contract.paws) };
      root.userData.headJoint = asset.contract.head_joint;
      this.actors.set(pose.id, actor);
    }
    actor.feet.restore();
    const moving = pose.speed > .005;
    const key = moving ? pose.speed / pose.scale > 1.5 ? 'run' : pose.behavior === 'Hunt' ? 'hunt' : 'walk'
      : pose.behavior === 'Rest' ? 'rest' : pose.behavior === 'Forage' ? 'forage'
        : pose.behavior === 'Drink' ? 'drink' : 'idle';
    const action = actor.actions.get(key)!;
    if (actor.current !== key) {
      // No invented attack/feeding result; these clips only illustrate the Core state.
      const previous = actor.actions.get(actor.current);
      const locomotion = ['walk', 'run', 'hunt'];
      if (previous && locomotion.includes(actor.current) && locomotion.includes(key)) {
        phase = previous.time / previous.getClip().duration;
      }
      action.reset().setEffectiveWeight(1).play();
      action.time = (phase % 1) * action.getClip().duration;
      if (previous) {
        previous.stopFading();
        actor.blendRemaining = actor.current === 'rest' || key === 'rest' ? .5 : .2;
        action.crossFadeFrom(previous, actor.blendRemaining, false);
      }
      actor.current = key;
    }
    const clip = asset.contract.clips[key];
    action.timeScale = moving ? pose.speed / pose.scale * clip.duration / clip.cycle_distance : 1;
    actor.mixer.update(dt);
    actor.blendRemaining = Math.max(0, actor.blendRemaining - Math.max(0, dt));
    // Retire faded clips so frequent Core behavior changes cannot accumulate work.
    for (const other of actor.actions.values()) {
      if (other !== action && other.isRunning() && other.getEffectiveWeight() <= 1e-6) other.stop();
    }
    actor.root.position.set(pose.x, pose.y, pose.z);
    const headingDelta = Math.atan2(Math.sin(pose.heading - actor.root.rotation.y), Math.cos(pose.heading - actor.root.rotation.y));
    actor.root.rotation.y = actor.oriented ? actor.root.rotation.y + headingDelta * (1 - Math.exp(-Math.max(0, dt) * 10)) : pose.heading;
    actor.oriented = true; actor.root.scale.setScalar(pose.scale);
    if (ground) {
      this.alignGround(actor, pose, dt, ground);
      actor.feet.apply(pose, key, action.time / action.getClip().duration, actor.blendRemaining > 0, ground);
    }
    actor.root.userData.targetHeading = pose.heading;
    actor.root.userData.behavior = pose.behavior;
    actor.root.userData.clip = key;
    actor.root.userData.tier = tier;
    this.used.add(pose.id);
    if (tier === 'near') this.nearCount++; else this.midCount++;
    return true;
  }

  private alignGround(actor: Actor, pose: TigerPose, dt: number, ground: GroundHeight): void {
    const height = ground(pose.x, pose.z);
    if (height === null) return;
    const span = .4 * pose.scale;
    const left = ground(pose.x - span, pose.z), right = ground(pose.x + span, pose.z);
    const back = ground(pose.x, pose.z - span), front = ground(pose.x, pose.z + span);
    // One-sided differences at a visible-window edge; missing terrain is never zero height.
    let slopeX = left !== null && right !== null ? (right - left) / (2 * span)
      : right !== null ? (right - height) / span : left !== null ? (height - left) / span : 0;
    let slopeZ = back !== null && front !== null ? (front - back) / (2 * span)
      : front !== null ? (front - height) / span : back !== null ? (height - back) / span : 0;
    const magnitude = Math.hypot(slopeX, slopeZ);
    const limit = Math.min(1, Math.tan(25 * Math.PI / 180) / Math.max(magnitude, 1e-6));
    slopeX *= limit; slopeZ *= limit;
    this.headingInverse.setFromAxisAngle(this.up, -actor.root.rotation.y);
    this.normal.set(-slopeX, 1, -slopeZ).normalize().applyQuaternion(this.headingInverse);
    this.groundRotation.setFromUnitVectors(this.up, this.normal);
    const blend = actor.grounded ? 1 - Math.exp(-Math.max(0, dt) * 12) : 1;
    actor.frame.quaternion.slerp(this.groundRotation, blend);
    actor.frame.position.y = THREE.MathUtils.lerp(actor.frame.position.y, (height - pose.y) / pose.scale, blend);
    actor.grounded = true;
  }

  end(parent: THREE.Group): void {
    for (const id of this.actors.keys()) if (!this.used.has(id)) this.remove(id);
    this.far?.end();
    if (this.far && this.far.activeCount) { if (this.far.group.parent !== this.group) this.group.add(this.far.group); }
    else this.far?.group.removeFromParent();
    if ((this.actors.size || this.far?.activeCount) && this.group.parent !== parent) parent.add(this.group);
    if (!this.actors.size && !this.far?.activeCount) this.group.removeFromParent();
  }

  private request(tier: Tier): void {
    if (!this.load || this.requested.has(tier)) return;
    this.requested.add(tier);
    void this.load(tier).then(gltf => {
      if (this.disposed) { disposeAsset(gltf); return; }
      try { this.assets.set(tier, inspect(gltf, tier)); }
      catch { disposeAsset(gltf); }
    }).catch(() => { /* Keep the existing fallback; never hide the animal. */ });
  }
  private remove(id: string): void {
    const actor = this.actors.get(id);
    if (!actor) return;
    actor.mixer.stopAllAction(); actor.mixer.uncacheRoot(actor.model);
    actor.model.traverse(object => { if (object instanceof THREE.SkinnedMesh) object.skeleton.dispose(); });
    actor.root.removeFromParent(); this.actors.delete(id);
  }
  dispose(): void {
    this.disposed = true;
    for (const id of this.actors.keys()) this.remove(id);
    this.far?.dispose(); this.far = null;
    this.assets.forEach(asset => disposeAsset(asset.gltf)); this.assets.clear();
    this.group.removeFromParent();
  }
}
