import * as THREE from 'three';
import { GLTFLoader, type GLTF } from 'three/addons/loaders/GLTFLoader.js';
import { clone } from 'three/addons/utils/SkeletonUtils.js';
import type { WildlifeAnimalObservation } from '../runtime/core-types';

type Tier = 'near' | 'mid';
type Clip = { name: string; duration: number; cycle_distance: number };
type Contract = {
  version: number; tier: Tier; units: string; forward: string; up: string;
  creator: string; license: string; source_sha256: string;
  clips: Record<string, Clip>;
};
type Asset = { gltf: GLTF; contract: Contract };
type Actor = {
  tier: Tier; root: THREE.Group; frame: THREE.Group; model: THREE.Object3D; mixer: THREE.AnimationMixer;
  actions: Map<string, THREE.AnimationAction>; current: string;
  grounded: boolean;
};
type GroundHeight = (x: number, z: number) => number | null;
export type TigerPose = {
  id: string; x: number; y: number; z: number; heading: number; scale: number;
  speed: number; phase: number; behavior: WildlifeAnimalObservation['behavior'];
};
export type TigerAssetLoader = (tier: Tier) => Promise<GLTF>;
const SOURCE_HASH = 'f7e3b63e83efd92ba379f750f02e9971f52679173c6a268a29f352ad0c2092be';
const CLIPS = ['idle', 'walk', 'run', 'hunt', 'forage', 'drink', 'rest'];

function inspect(gltf: GLTF, tier: Tier): Asset {
  const contract = gltf.parser.json.asset?.extras?.lifelensTiger as Contract | undefined;
  if (!contract || contract.version !== 1 || contract.tier !== tier
    || contract.units !== 'metres' || contract.forward !== '+Z' || contract.up !== '+Y'
    || contract.license !== 'CC-BY-4.0' || contract.creator !== 'TopoForge'
    || contract.source_sha256 !== SOURCE_HASH) throw new Error('Unrecognized tiger asset');
  let skinned = false;
  gltf.scene.traverse(object => {
    if (object instanceof THREE.SkinnedMesh && object.skeleton.bones.length >= 24
      && object.geometry.getAttribute('skinWeight')) skinned = true;
  });
  if (!skinned || CLIPS.some(key => {
    const clip = contract.clips?.[key];
    return !clip || !Number.isFinite(clip.duration) || clip.duration <= 0
      || !Number.isFinite(clip.cycle_distance) || clip.cycle_distance < 0
      || !gltf.animations.some(animation => animation.name === clip.name && animation.tracks.length > 0);
  })) throw new Error('Tiger skin or animation contract missing');
  return { gltf, contract };
}

function disposeAsset(gltf: GLTF): void {
  const geometries = new Set<THREE.BufferGeometry>();
  const materials = new Set<THREE.Material>();
  const textures = new Set<THREE.Texture>();
  gltf.scene.traverse(object => {
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
  private nearCount = 0;
  private midCount = 0;
  private disposed = false;
  private readonly normal = new THREE.Vector3();
  private readonly up = new THREE.Vector3(0, 1, 0);
  private readonly headingInverse = new THREE.Quaternion();
  private readonly groundRotation = new THREE.Quaternion();
  constructor(private readonly load: TigerAssetLoader | null = typeof document === 'undefined' ? null : tier => {
    const name = tier === 'near' ? 'tiger-rigged.glb' : 'tiger-mid.glb';
    return new GLTFLoader().loadAsync(new URL(`vendor/wildlife/${name}`, document.baseURI).href);
  }) { this.group.name = 'LifeLensImportedTigers'; }

  begin(): void { this.used.clear(); this.nearCount = 0; this.midCount = 0; }

  draw(pose: TigerPose, tier: Tier, dt: number, ground?: GroundHeight): boolean {
    if (this.disposed || (tier === 'near' ? this.nearCount >= 2 : this.midCount >= 4)) return false;
    const asset = this.assets.get(tier);
    if (!asset) { this.request(tier); return false; }
    let actor = this.actors.get(pose.id);
    let phase = pose.phase / (Math.PI * 2);
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
      actor = { tier, root, frame, model, mixer, actions, current: '', grounded: false };
      this.actors.set(pose.id, actor);
    }
    const moving = pose.speed > .005;
    const key = moving ? pose.behavior === 'Hunt' ? 'hunt' : pose.speed / pose.scale > 1.5 ? 'run' : 'walk'
      : pose.behavior === 'Rest' ? 'rest' : pose.behavior === 'Forage' ? 'forage'
        : pose.behavior === 'Drink' ? 'drink' : 'idle';
    const action = actor.actions.get(key)!;
    if (actor.current !== key) {
      // No invented attack/feeding result; these clips only illustrate the Core state.
      actor.mixer.stopAllAction(); action.reset().play();
      action.time = (phase % 1) * action.getClip().duration;
      actor.current = key;
    }
    const clip = asset.contract.clips[key];
    action.timeScale = moving ? pose.speed / pose.scale * clip.duration / clip.cycle_distance : 1;
    actor.mixer.update(dt);
    actor.root.position.set(pose.x, pose.y, pose.z);
    actor.root.rotation.set(0, pose.heading, 0); actor.root.scale.setScalar(pose.scale);
    if (ground) this.alignGround(actor, pose, dt, ground);
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
    this.headingInverse.setFromAxisAngle(this.up, -pose.heading);
    this.normal.set(-slopeX, 1, -slopeZ).normalize().applyQuaternion(this.headingInverse);
    this.groundRotation.setFromUnitVectors(this.up, this.normal);
    const blend = actor.grounded ? 1 - Math.exp(-Math.max(0, dt) * 12) : 1;
    actor.frame.quaternion.slerp(this.groundRotation, blend);
    actor.frame.position.y = THREE.MathUtils.lerp(actor.frame.position.y, (height - pose.y) / pose.scale, blend);
    actor.grounded = true;
  }

  end(parent: THREE.Group): void {
    for (const id of this.actors.keys()) if (!this.used.has(id)) this.remove(id);
    if (this.actors.size && this.group.parent !== parent) parent.add(this.group);
    if (!this.actors.size) this.group.removeFromParent();
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
    this.assets.forEach(asset => disposeAsset(asset.gltf)); this.assets.clear();
    this.group.removeFromParent();
  }
}
