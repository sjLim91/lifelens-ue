import * as THREE from 'three';
import { GLTFLoader, type GLTF } from 'three/addons/loaders/GLTFLoader.js';
import { clone } from 'three/addons/utils/SkeletonUtils.js';
import type { WildlifeAnimalObservation } from '../runtime/core-types';
import { NativeWildlifeContact, type WildlifePaw, type WildlifeContactClip } from './native-wildlife-contact';
import { NATIVE_WILDLIFE_SOURCES } from './native-wildlife-manifest';

export type NativeWildlifeSpecies = keyof typeof NATIVE_WILDLIFE_SOURCES;
type Tier = 'near' | 'mid' | 'far';
type Ground = (x: number, z: number) => number | null;
type Clip = WildlifeContactClip & { name: string; duration: number; cycle_distance: number };
type Contract = { version: number; species: NativeWildlifeSpecies; tier: Tier;
  units: string; forward: string; up: string; license: string; creator: string;
  source_sha256: string; body: string; bone_count: number; status: string;
  clips: Record<string, Clip>; paws: WildlifePaw[]; boundsHeight: number; render_mode?: string };
type Asset = { gltf: GLTF; contract: Contract };
export type NativeWildlifePose = { id: string; species: NativeWildlifeSpecies; sex: 'Male' | 'Female';
  x: number; y: number; z: number; heading: number; scale: number; speed: number;
  phase: number; behavior: WildlifeAnimalObservation['behavior'] };
export type NativeWildlifeLoader = (species: NativeWildlifeSpecies, tier: Tier) => Promise<GLTF>;
type Actor = { tier: Tier; species: NativeWildlifeSpecies; root: THREE.Group; frame: THREE.Group; model: THREE.Object3D;
  mixer: THREE.AnimationMixer; actions: Map<string, THREE.AnimationAction>; current: string;
  antlers: THREE.Object3D[]; contact: NativeWildlifeContact | null; blend: number; grounded: boolean };

function inspect(gltf: GLTF, species: NativeWildlifeSpecies, tier: Tier): Asset {
  const c = gltf.parser.json.asset?.extras?.lifelensWildlife as Contract | undefined;
  if (!c || c.version !== 1 || c.species !== species || c.tier !== tier
    || c.source_sha256 !== NATIVE_WILDLIFE_SOURCES[species].sha256 || c.license !== 'CC-BY-4.0'
    || c.units !== 'metres' || c.forward !== '+Z' || c.up !== '+Y'
    || c.status === 'INSPECTION_ONLY' || gltf.parser.json.asset?.extras?.lifelensInspectionOnly)
    throw new Error('Unrecognized native animal source');
  const skins: THREE.SkinnedMesh[] = [];
  let triangles = 0;
  gltf.scene.traverse(o => { if (o instanceof THREE.Mesh) {
    triangles += (o.geometry.index?.count ?? o.geometry.getAttribute('position').count) / 3;
    if (o instanceof THREE.SkinnedMesh) skins.push(o);
  } });
  if (tier === 'far') {
    if (skins.length || c.bone_count !== 0 || c.render_mode !== 'instanced-static' || triangles > 4000)
      throw new Error('Native far batch is not static or exceeds budget');
    for (const pose of ['Idle', 'Rest', 'Forage']) {
      const mesh = gltf.scene.getObjectByName('LifeLensNativeFar' + pose + 'Body');
      if (!(mesh instanceof THREE.Mesh) || !mesh.geometry.getAttribute('color')) throw new Error('Source-derived far pose missing');
    }
  } else {
    const body = gltf.scene.getObjectByName(c.body);
    const required = species === 'FreshwaterFish' ? ['idle', 'swim'] : ['idle', 'walk', 'run', 'hunt', 'forage', 'drink', 'rest'];
    if (!(body instanceof THREE.SkinnedMesh) || !skins.length || c.bone_count > 128
      || body.skeleton.bones.length !== c.bone_count || triangles > (tier === 'near' ? 80000 : 10000)
      || required.some(key => { const clip = c.clips?.[key]; return !clip || !(clip.duration > 0)
        || !Number.isFinite(clip.cycle_distance) || clip.cycle_distance < 0
        || !gltf.animations.some(a => a.name === clip.name && a.tracks.length > 0); }))
      throw new Error('Native skin, motion or topology budget missing');
    if (species !== 'FreshwaterFish' && (!Array.isArray(c.paws) || c.paws.length !== 4))
      throw new Error('Native four-foot contact metadata absent');
    if (species === 'FreshwaterFish' && (!Number.isFinite(c.boundsHeight) || c.boundsHeight > .35))
      throw new Error('Native fish exceeds established water-clearance budget');
  }
  return { gltf, contract: c };
}

function disposeAsset(gltf: GLTF): void {
  const geometry = new Set<THREE.BufferGeometry>(), materials = new Set<THREE.Material>(), textures = new Set<THREE.Texture>();
  const skeletons = new Set<THREE.Skeleton>();
  for (const scene of new Set([gltf.scene, ...gltf.scenes])) scene.traverse(o => {
    if (!(o instanceof THREE.Mesh)) return;
    geometry.add(o.geometry); if (o instanceof THREE.SkinnedMesh) skeletons.add(o.skeleton);
    for (const m of Array.isArray(o.material) ? o.material : [o.material]) {
      materials.add(m); for (const v of Object.values(m)) if (v instanceof THREE.Texture) textures.add(v);
    }
  });
  skeletons.forEach(s => s.dispose()); geometry.forEach(g => g.dispose()); materials.forEach(m => m.dispose());
  textures.forEach(t => { t.dispose(); const data = t.source.data; if (typeof ImageBitmap !== 'undefined' && data instanceof ImageBitmap) data.close(); });
}

class NativeFar {
  readonly group = new THREE.Group();
  private readonly batches: { pose: string; antlers: boolean; mesh: THREE.InstancedMesh; count: number }[] = [];
  private readonly transform = new THREE.Object3D();
  private count = 0;
  private readonly ids: string[] = [];
  constructor(asset: Asset) {
    this.group.name = 'LifeLensNativeFar:' + asset.contract.species;
    this.group.userData.coreIds = this.ids;
    for (const pose of ['Idle', 'Rest', 'Forage']) for (const part of ['Body', 'Antlers']) {
      const source = asset.gltf.scene.getObjectByName('LifeLensNativeFar' + pose + part);
      if (!(source instanceof THREE.Mesh)) continue;
      const geometry = source.geometry.clone();
      for (const key of ['llNativePhase', 'llNativeMotion']) geometry.setAttribute(key, new THREE.InstancedBufferAttribute(new Float32Array(128), 1));
      const sourceMaterial = Array.isArray(source.material) ? source.material[0]! : source.material;
      const material = sourceMaterial.clone() as THREE.MeshStandardMaterial;
      material.onBeforeCompile = shader => {
        shader.vertexShader = shader.vertexShader.replace('#include <common>', '#include <common>\nattribute float llNativePhase;\nattribute float llNativeMotion;');
        shader.vertexShader = shader.vertexShader.replace('#include <begin_vertex>', `#include <begin_vertex>
float lowLeg = 1.0 - smoothstep(0.12, ${Math.max(.18, asset.contract.boundsHeight * .44).toFixed(5)}, position.y);
${asset.contract.species === 'FreshwaterFish'
  ? 'transformed.x += llNativeMotion * 0.025 * sin(llNativePhase + position.z * 12.0) * smoothstep(0.0, 0.30, -position.z);'
  : 'transformed.z += llNativeMotion * lowLeg * 0.04 * sin(llNativePhase + sign(position.x * position.z) * 1.5708);'}
`);
      };
      material.customProgramCacheKey = () => 'lifelens-native-far-' + asset.contract.species;
      const mesh = new THREE.InstancedMesh(geometry, material, 128);
      mesh.name = source.name; mesh.count = 0; mesh.frustumCulled = false;
      mesh.castShadow = asset.contract.species !== 'FreshwaterFish'; mesh.receiveShadow = true;
      this.group.add(mesh); this.batches.push({ pose, antlers: part === 'Antlers', mesh, count: 0 });
    }
  }
  begin(): void { this.count = 0; this.ids.length = 0; for (const b of this.batches) b.count = 0; }
  draw(pose: NativeWildlifePose, time: number): boolean {
    if (this.count >= 128) return false;
    const key = pose.behavior === 'Rest' ? 'Rest' : pose.behavior === 'Forage' || pose.behavior === 'Drink' ? 'Forage' : 'Idle';
    this.transform.position.set(pose.x, pose.y, pose.z); this.transform.rotation.set(0, pose.heading, 0);
    this.transform.scale.setScalar(pose.scale); this.transform.updateMatrix();
    for (const b of this.batches) if (b.pose === key && (!b.antlers || pose.sex === 'Male')) {
      b.mesh.setMatrixAt(b.count, this.transform.matrix);
      (b.mesh.geometry.getAttribute('llNativePhase') as THREE.InstancedBufferAttribute).setX(b.count, time * 8 + pose.phase);
      (b.mesh.geometry.getAttribute('llNativeMotion') as THREE.InstancedBufferAttribute).setX(b.count, pose.speed > .005 || pose.behavior === 'Swim' ? 1 : 0);
      b.count++;
    }
    this.ids.push(pose.id); this.count++; return true;
  }
  end(): void { for (const b of this.batches) {
    b.mesh.count = b.count; b.mesh.instanceMatrix.needsUpdate = true;
    for (const k of ['llNativePhase', 'llNativeMotion']) (b.mesh.geometry.getAttribute(k) as THREE.InstancedBufferAttribute).needsUpdate = true;
  } }
  get activeCount(): number { return this.count; }
  dispose(): void { for (const b of this.batches) {
    b.mesh.geometry.dispose(); (b.mesh.material as THREE.Material).dispose(); b.mesh.dispose();
  } this.group.clear(); }
}

/** Four reviewed native species; the existing tiger pool remains independent. */
export class NativeWildlifePool {
  readonly group = new THREE.Group();
  private readonly assets = new Map<string, Asset>();
  private readonly requested = new Set<string>();
  private readonly actors = new Map<string, Actor>();
  private readonly far = new Map<NativeWildlifeSpecies, NativeFar>();
  private readonly used = new Set<string>();
  private near = 0;
  private mid = 0;
  private disposed = false;
  private time = 0;
  private readonly normal = new THREE.Vector3();
  private readonly up = new THREE.Vector3(0, 1, 0);
  private readonly headingInverse = new THREE.Quaternion();
  private readonly groundRotation = new THREE.Quaternion();
  constructor(private readonly load: NativeWildlifeLoader | null = typeof document === 'undefined' ? null : (species, tier) => {
    const slug = NATIVE_WILDLIFE_SOURCES[species].slug;
    const file = tier === 'near' ? slug + '-rigged.glb' : slug + '-' + tier + '.glb';
    return new GLTFLoader().loadAsync(new URL('vendor/wildlife/' + file, document.baseURI).href);
  }) { this.group.name = 'LifeLensNativeWildlife'; }
  begin(dt: number): void {
    this.time += Math.max(0, dt); this.near = this.mid = 0; this.used.clear(); this.far.forEach(f => f.begin());
  }
  draw(pose: NativeWildlifePose, tier: Tier, dt: number, ground?: Ground): boolean {
    if (this.disposed || tier === 'near' && this.near >= 4 || tier === 'mid' && this.mid >= 8) return false;
    const key = pose.species + ':' + tier, asset = this.assets.get(key);
    if (!asset) { this.request(pose.species, tier); return false; }
    if (tier === 'far') {
      let far = this.far.get(pose.species);
      if (!far) { far = new NativeFar(asset); this.far.set(pose.species, far); }
      return far.draw(pose, this.time);
    }
    let actor = this.actors.get(pose.id);
    let phase = pose.phase / (Math.PI * 2);
    if (actor && (actor.tier !== tier || actor.species !== pose.species)) {
      const action = actor.actions.get(actor.current); if (action && actor.species === pose.species) phase = action.time / action.getClip().duration;
      this.remove(pose.id); actor = undefined;
    }
    if (!actor) {
      const root = new THREE.Group(), frame = new THREE.Group(), model = clone(asset.gltf.scene);
      root.name = 'LifeLensNativeAnimal:' + pose.id; root.userData.coreId = pose.id;
      const antlers: THREE.Object3D[] = [];
      model.traverse(o => { if (o instanceof THREE.Mesh) {
        o.frustumCulled = false; o.castShadow = pose.species !== 'FreshwaterFish'; o.receiveShadow = true;
        if ((Array.isArray(o.material) ? o.material : [o.material]).some(m => m.name.includes('jiao_tex'))) antlers.push(o);
      } });
      frame.add(model); root.add(frame); this.group.add(root);
      const mixer = new THREE.AnimationMixer(model), actions = new Map<string, THREE.AnimationAction>();
      for (const [name, clip] of Object.entries(asset.contract.clips)) {
        const source = asset.gltf.animations.find(a => a.name === clip.name)!; actions.set(name, mixer.clipAction(source));
      }
      const skin = model.getObjectByName(asset.contract.body) as THREE.SkinnedMesh;
      let contact: NativeWildlifeContact | null;
      try { contact = pose.species === 'FreshwaterFish' ? null : new NativeWildlifeContact(root, frame, skin, asset.contract.paws); }
      catch { mixer.stopAllAction(); mixer.uncacheRoot(model); root.removeFromParent(); return false; }
      actor = { tier, species: pose.species, root, frame, model, mixer, actions, current: '', antlers, contact, blend: 0, grounded: false };
      root.rotation.y = pose.heading; this.actors.set(pose.id, actor);
    }
    actor.contact?.restore();
    const moving = pose.speed > .005;
    const name = pose.species === 'FreshwaterFish' ? moving || pose.behavior === 'Swim' ? 'swim' : 'idle'
      : moving ? pose.speed / pose.scale > 1.5 ? 'run' : pose.behavior === 'Hunt' ? 'hunt' : 'walk'
      : pose.behavior === 'Rest' ? 'rest' : pose.behavior === 'Forage' ? 'forage' : pose.behavior === 'Drink' ? 'drink' : 'idle';
    const action = actor.actions.get(name)!, clip = asset.contract.clips[name]!;
    if (actor.current !== name) {
      const previous = actor.actions.get(actor.current);
      if (previous && ['walk', 'run', 'hunt'].includes(name) && ['walk', 'run', 'hunt'].includes(actor.current)) phase = previous.time / previous.getClip().duration;
      action.reset().setEffectiveWeight(1).play(); action.time = phase % 1 * action.getClip().duration;
      if (previous) { actor.blend = actor.current === 'rest' || name === 'rest' ? .4 : .2; action.crossFadeFrom(previous, actor.blend, false); }
      actor.current = name;
    }
    action.timeScale = moving && clip.cycle_distance > 0 ? pose.speed / pose.scale * clip.duration / clip.cycle_distance : 1;
    actor.mixer.update(Math.max(0, dt)); actor.blend = Math.max(0, actor.blend - Math.max(0, dt));
    for (const other of actor.actions.values()) if (other !== action && other.isRunning() && other.getEffectiveWeight() < 1e-6) other.stop();
    actor.root.position.set(pose.x, pose.y, pose.z); actor.root.scale.setScalar(pose.scale);
    const delta = Math.atan2(Math.sin(pose.heading - actor.root.rotation.y), Math.cos(pose.heading - actor.root.rotation.y));
    actor.root.rotation.y += delta * (1 - Math.exp(-Math.max(0, dt) * 10));
    if (ground && pose.species !== 'FreshwaterFish') {
      const h = ground(pose.x, pose.z);
      if (h !== null) {
        actor.frame.position.y = (h - pose.y) / pose.scale;
        const span = .35 * pose.scale, left = ground(pose.x - span, pose.z), right = ground(pose.x + span, pose.z);
        const back = ground(pose.x, pose.z - span), front = ground(pose.x, pose.z + span);
        let sx = left !== null && right !== null ? (right - left) / (2 * span) : 0;
        let sz = back !== null && front !== null ? (front - back) / (2 * span) : 0;
        const limit = Math.min(1, Math.tan(25 * Math.PI / 180) / Math.max(1e-6, Math.hypot(sx, sz)));
        sx *= limit; sz *= limit;
        this.headingInverse.setFromAxisAngle(this.up, -actor.root.rotation.y);
        this.normal.set(-sx, 1, -sz).normalize().applyQuaternion(this.headingInverse);
        this.groundRotation.setFromUnitVectors(this.up, this.normal);
        actor.frame.quaternion.slerp(this.groundRotation, actor.grounded ? 1 - Math.exp(-Math.max(0, dt) * 12) : 1);
        actor.grounded = true;
      }
      actor.contact?.apply(name, action.time / action.getClip().duration, clip, actor.blend > 0, pose.scale, ground);
    }
    for (const antler of actor.antlers) antler.visible = pose.sex === 'Male';
    Object.assign(actor.root.userData, { species: pose.species, behavior: pose.behavior, clip: name, tier });
    this.used.add(pose.id); if (tier === 'near') this.near++; else this.mid++;
    return true;
  }
  end(parent: THREE.Group): void {
    for (const id of this.actors.keys()) if (!this.used.has(id)) this.remove(id);
    let farCount = 0;
    for (const far of this.far.values()) { far.end(); farCount += far.activeCount;
      if (far.activeCount && far.group.parent !== this.group) this.group.add(far.group);
      else if (!far.activeCount) far.group.removeFromParent();
    }
    if (this.actors.size || farCount) { if (this.group.parent !== parent) parent.add(this.group); }
    else this.group.removeFromParent();
  }
  private request(species: NativeWildlifeSpecies, tier: Tier): void {
    const key = species + ':' + tier;
    if (!this.load || this.requested.has(key)) return; this.requested.add(key);
    void this.load(species, tier).then(gltf => {
      if (this.disposed) { disposeAsset(gltf); return; }
      try { this.assets.set(key, inspect(gltf, species, tier)); }
      catch { disposeAsset(gltf); }
    }).catch(() => { /* Existing instancing remains visible for this ID. */ });
  }
  private remove(id: string): void {
    const actor = this.actors.get(id); if (!actor) return;
    actor.mixer.stopAllAction(); actor.mixer.uncacheRoot(actor.model);
    const skeletons = new Set<THREE.Skeleton>(); actor.model.traverse(o => { if (o instanceof THREE.SkinnedMesh) skeletons.add(o.skeleton); });
    skeletons.forEach(s => s.dispose()); actor.root.removeFromParent(); this.actors.delete(id);
  }
  dispose(): void {
    this.disposed = true; for (const id of this.actors.keys()) this.remove(id);
    this.far.forEach(f => f.dispose()); this.far.clear(); this.assets.forEach(a => disposeAsset(a.gltf)); this.assets.clear(); this.group.removeFromParent();
  }
}
