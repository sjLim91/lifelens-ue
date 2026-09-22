import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { clone as cloneSkeleton } from 'three/addons/utils/SkeletonUtils.js';
import type {
  Resident,
  TerrainWindow,
} from '../runtime/core-types';
import { residentToWorldPosition } from './resident-world-coordinates';

const BASE_MODEL_COMMIT = 'ddd5fc34a445bcded3cf9836607aaeebc19a5c78';
const BASE_MODEL_URL =
  `https://raw.githubusercontent.com/programasweights/avatar/${BASE_MODEL_COMMIT}/public/assets/character.glb`;

const ANIMATION_COMMIT = 'aa02a4e6d8337a0604d2da131bcbbeb1f01badf0';
const ANIMATION_URL =
  `https://raw.githubusercontent.com/Seyamalam/blood-league-kickoff/${ANIMATION_COMMIT}/public/assets/vendor/quaternius/universal-animation-library.glb`;

type MotionName = 'idle' | 'walk' | 'talk' | 'sit' | 'interact';

interface ResidentActor {
  root: THREE.Group;
  mixer: THREE.AnimationMixer;
  idle?: THREE.AnimationAction;
  walk?: THREE.AnimationAction;
  talk?: THREE.AnimationAction;
  sit?: THREE.AnimationAction;
  interact?: THREE.AnimationAction;
  active: MotionName | '';
  activityLabel: string;
  current: THREE.Vector3;
  target: THREE.Vector3;
  initialized: boolean;
}

function stableHash(value: string): number {
  let hash = 2166136261 >>> 0;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return hash >>> 0;
}

function findClip(
  clips: THREE.AnimationClip[],
  exact: string,
  contains: string,
): THREE.AnimationClip | undefined {
  const exactLower = exact.toLowerCase();
  return clips.find((clip) => clip.name.toLowerCase() === exactLower)
    ?? clips.find((clip) => (
      clip.name.toLowerCase().includes(contains.toLowerCase())
    ));
}

function cloneActorMaterials(
  root: THREE.Object3D,
  variantSeed: number,
): void {
  const hueShift = ((variantSeed % 17) - 8) * 0.006;

  root.traverse((object) => {
    if (!(object instanceof THREE.Mesh)) return;

    const cloneMaterial = (
      material: THREE.Material,
    ): THREE.Material => {
      const cloned = material.clone();
      if (cloned instanceof THREE.MeshStandardMaterial) {
        cloned.roughness = Math.max(0.48, cloned.roughness);
        cloned.metalness = Math.min(0.04, cloned.metalness);
        cloned.color.offsetHSL(hueShift, 0, 0);
      }
      return cloned;
    };

    object.material = Array.isArray(object.material)
      ? object.material.map(cloneMaterial)
      : cloneMaterial(object.material);
  });
}

export class ResidentWorldLayer {
  readonly group = new THREE.Group();

  private readonly loader = new GLTFLoader();
  private readonly actors = new Map<string, ResidentActor>();
  private template: THREE.Group | null = null;
  private clips: THREE.AnimationClip[] = [];
  private ready = false;
  private pendingResidents: Resident[] = [];
  private pendingTerrain: TerrainWindow | null = null;
  private pendingCenterX = 0;
  private pendingCenterY = 0;

  constructor() {
    void this.loadAssets();
  }

  setResidents(
    residents: Resident[],
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    this.pendingResidents = residents;
    this.pendingTerrain = terrain;
    this.pendingCenterX = centerX;
    this.pendingCenterY = centerY;

    if (!this.ready || !this.template) return;

    const terrainMap = new Map(
      terrain.chunks.map((chunk) => [`${chunk.x}:${chunk.y}`, chunk]),
    );
    const activeIds = new Set(residents.map((resident) => resident.id));

    for (const [id, actor] of this.actors) {
      actor.root.visible = activeIds.has(id);
    }

    for (const resident of residents) {
      if (
        !resident.hasPosition
        || resident.gridX === undefined
        || resident.gridY === undefined
      ) {
        continue;
      }

      const chunkX = Math.floor(resident.gridX / 32);
      const chunkY = Math.floor(resident.gridY / 32);
      const terrainChunk = terrainMap.get(`${chunkX}:${chunkY}`);
      const elevation = Number(terrainChunk?.elevation01) || 0;
      const position = residentToWorldPosition(
        resident,
        centerX,
        centerY,
        elevation,
      );
      if (!position) continue;

      const actor = this.ensureActor(resident);
      const next = new THREE.Vector3(
        position.x,
        position.y,
        position.z,
      );

      if (actor.initialized) {
        const direction = next.clone().sub(actor.target);
        if (direction.lengthSq() > 0.0004) {
          actor.root.rotation.y = Math.atan2(direction.x, direction.z);
        }
      }

      actor.activityLabel = resident.activityLabel ?? 'Idle';
      actor.target.copy(next);

      if (!actor.initialized) {
        actor.current.copy(next);
        actor.root.position.copy(next);
        actor.initialized = true;
      }

      actor.root.visible = true;
    }
  }

  update(deltaSeconds: number): void {
    const dt = Math.min(0.05, Math.max(0, deltaSeconds));
    const ease = 1 - Math.exp(-dt * 7.5);

    for (const actor of this.actors.values()) {
      if (!actor.root.visible) continue;

      const distance = actor.current.distanceTo(actor.target);
      actor.current.lerp(actor.target, ease);
      actor.root.position.copy(actor.current);
      this.setAction(actor, distance > 0.025);
      actor.mixer.update(dt);
    }
  }

  dispose(): void {
    for (const actor of this.actors.values()) {
      actor.root.traverse((object) => {
        if (!(object instanceof THREE.Mesh)) return;
        const materials = Array.isArray(object.material)
          ? object.material
          : [object.material];
        for (const material of materials) material.dispose();
      });
    }
    this.actors.clear();
  }

  private async loadAssets(): Promise<void> {
    try {
      const base = await this.loader.loadAsync(BASE_MODEL_URL);

      let animationClips: THREE.AnimationClip[] = [];
      try {
        const animationAsset = await this.loader.loadAsync(ANIMATION_URL);
        animationClips = animationAsset.animations;
      } catch (error) {
        console.warn(
          'LifeLens Three World animation asset unavailable',
          error,
        );
      }

      const source = base.scene;
      source.updateMatrixWorld(true);
      const box = new THREE.Box3().setFromObject(source);
      const center = box.getCenter(new THREE.Vector3());
      const size = box.getSize(new THREE.Vector3());
      const modelHeight = Math.max(0.001, size.y);

      source.position.x -= center.x;
      source.position.y -= box.min.y;
      source.position.z -= center.z;
      source.rotation.y = Math.PI;

      const normalized = new THREE.Group();
      normalized.add(source);
      normalized.scale.setScalar(1 / modelHeight);

      this.template = normalized;
      this.clips = animationClips.length > 0
        ? animationClips
        : base.animations;
      this.ready = true;

      if (this.pendingTerrain) {
        this.setResidents(
          this.pendingResidents,
          this.pendingTerrain,
          this.pendingCenterX,
          this.pendingCenterY,
        );
      }
    } catch (error) {
      console.warn(
        'LifeLens Three World resident model unavailable',
        error,
      );
    }
  }

  private ensureActor(resident: Resident): ResidentActor {
    const existing = this.actors.get(resident.id);
    if (existing) return existing;
    if (!this.template) {
      throw new Error('Three World resident template is not loaded');
    }

    const variantSeed = stableHash(resident.id);
    const root = new THREE.Group();
    const model = cloneSkeleton(this.template) as THREE.Group;
    cloneActorMaterials(model, variantSeed);
    root.add(model);

    const baseHeight = 1.68;
    const heightJitter = (((variantSeed >>> 8) % 9) - 4) * 0.015;
    const bodyWidth = resident.sex === 'Female' ? 0.94 : 1;
    root.scale.set(
      bodyWidth,
      baseHeight + heightJitter,
      bodyWidth,
    );

    const mixer = new THREE.AnimationMixer(root);
    const idleClip = findClip(this.clips, 'Idle_Loop', 'idle');
    const walkClip = findClip(this.clips, 'Walk_Loop', 'walk');
    const talkClip = findClip(
      this.clips,
      'Idle_Talking_Loop',
      'talking',
    );
    const sitClip = findClip(
      this.clips,
      'Sitting_Idle_Loop',
      'sitting_idle',
    );
    const interactClip = findClip(this.clips, 'Interact', 'interact');

    const idle = idleClip ? mixer.clipAction(idleClip, root) : undefined;
    const walk = walkClip ? mixer.clipAction(walkClip, root) : undefined;
    const talk = talkClip ? mixer.clipAction(talkClip, root) : undefined;
    const sit = sitClip ? mixer.clipAction(sitClip, root) : undefined;
    const interact = interactClip
      ? mixer.clipAction(interactClip, root)
      : undefined;

    [idle, walk, talk, sit, interact].forEach((action) => {
      action?.setLoop(THREE.LoopRepeat, Infinity);
    });

    idle?.play();
    if (idle) {
      idle.time = ((variantSeed % 997) / 997)
        * Math.max(0.001, idle.getClip().duration);
    }

    const actor: ResidentActor = {
      root,
      mixer,
      idle,
      walk,
      talk,
      sit,
      interact,
      active: idle ? 'idle' : '',
      activityLabel: resident.activityLabel ?? 'Idle',
      current: new THREE.Vector3(),
      target: new THREE.Vector3(),
      initialized: false,
    };

    this.actors.set(resident.id, actor);
    this.group.add(root);
    return actor;
  }

  private actionFor(
    actor: ResidentActor,
    motion: MotionName,
  ): THREE.AnimationAction | undefined {
    switch (motion) {
      case 'walk': return actor.walk;
      case 'talk': return actor.talk;
      case 'sit': return actor.sit;
      case 'interact': return actor.interact;
      default: return actor.idle;
    }
  }

  private restMotion(actor: ResidentActor): MotionName {
    const activity = actor.activityLabel;

    if (
      (activity === 'Sleep' || activity === 'UseToilet')
      && actor.sit
    ) {
      return 'sit';
    }

    if (
      (activity === 'Eat'
        || activity === 'Drink'
        || activity === 'Wash')
      && actor.interact
    ) {
      return 'interact';
    }

    if (
      (activity === 'Approach'
        || activity === 'Repair'
        || activity === 'Comfort')
      && actor.talk
    ) {
      return 'talk';
    }

    return 'idle';
  }

  private setAction(actor: ResidentActor, moving: boolean): void {
    const desired: MotionName = moving && actor.walk
      ? 'walk'
      : this.restMotion(actor);
    if (actor.active === desired) return;

    const previous = actor.active
      ? this.actionFor(actor, actor.active)
      : undefined;
    const next = this.actionFor(actor, desired) ?? actor.idle;
    if (!next) return;

    previous?.fadeOut(0.18);
    next.reset().fadeIn(0.18).play();
    actor.active = desired;
  }
}
