import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { clone as cloneSkeleton } from 'three/addons/utils/SkeletonUtils.js';
import type {
  Resident,
  ResidentPresentationDirective,
  TerrainWindow,
} from '../runtime/core-types';
import {
  RESIDENT_PRESENTATION_CONTRACT,
  RESIDENT_VISUAL_SPEED_WORLD_UNITS_PER_SECOND_AT_1X,
  SIMULATION_TIME_CONTRACT,
  WORLD_GRID_CONTRACT,
  normalizeSimulationSpeed,
} from '../runtime/lifelens-contract';
import type { SimulationSpeed } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';
import {
  applyResidentMaterialVariant,
  createResidentAppearanceProfile,
} from './resident-appearance';
import { residentActionCue } from './resident-action-context';
import { residentToWorldPosition } from './resident-world-coordinates';

const BASE_MODEL_COMMIT = 'ddd5fc34a445bcded3cf9836607aaeebc19a5c78';
const BASE_MODEL_URL =
  `https://raw.githubusercontent.com/programasweights/avatar/${BASE_MODEL_COMMIT}/public/assets/character.glb`;

const ANIMATION_COMMIT = 'aa02a4e6d8337a0604d2da131bcbbeb1f01badf0';
const ANIMATION_URL =
  `https://raw.githubusercontent.com/Seyamalam/blood-league-kickoff/${ANIMATION_COMMIT}/public/assets/vendor/quaternius/universal-animation-library.glb`;

type MotionName = 'idle' | 'walk' | 'talk' | 'sit' | 'interact';

interface ResidentActionCueSprite {
  sprite: THREE.Sprite;
  texture: THREE.CanvasTexture;
  canvas: HTMLCanvasElement;
  text: string;
}

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
  activityTargetId: string;
  presentation: ResidentPresentationDirective | null;
  actionCue: ResidentActionCueSprite | null;
  current: THREE.Vector3;
  target: THREE.Vector3;
  targetYaw: number;
  targetTravelSpeedWorldUnitsPerSecond: number;
  smoothedTravelSpeedWorldUnitsPerSecond: number;
  walkGraceRemainingSeconds: number;
  gaitRateBias: number;
  initialized: boolean;
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

function createActionCueSprite(): ResidentActionCueSprite | null {
  if (typeof document === 'undefined') return null;

  const canvas = document.createElement('canvas');
  canvas.width = 512;
  canvas.height = 96;
  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.minFilter = THREE.LinearFilter;
  texture.magFilter = THREE.LinearFilter;

  const material = new THREE.SpriteMaterial({
    map: texture,
    transparent: true,
    depthWrite: false,
    depthTest: true,
    opacity: 0.96,
  });
  const sprite = new THREE.Sprite(material);
  sprite.scale.set(3.6, 0.68, 1);
  sprite.renderOrder = 8;
  sprite.visible = false;

  return {
    sprite,
    texture,
    canvas,
    text: '',
  };
}

function paintActionCue(
  cue: ResidentActionCueSprite,
  text: string,
  phase: ResidentPresentationDirective['phase'],
): void {
  if (cue.text === text) return;
  cue.text = text;

  const context = cue.canvas.getContext('2d');
  if (!context) return;

  context.clearRect(0, 0, cue.canvas.width, cue.canvas.height);
  context.fillStyle = 'rgba(8, 15, 11, 0.84)';
  context.fillRect(4, 8, cue.canvas.width - 8, cue.canvas.height - 16);

  context.strokeStyle = phase === 'Moving'
    ? 'rgba(150, 190, 169, 0.72)'
    : 'rgba(222, 201, 143, 0.76)';
  context.lineWidth = 3;
  context.strokeRect(5.5, 9.5, cue.canvas.width - 11, cue.canvas.height - 19);

  const safeText = text.length > 28
    ? `${text.slice(0, 27)}…`
    : text;
  let fontSize = 30;
  context.font = `600 ${fontSize}px system-ui, sans-serif`;
  while (
    fontSize > 21
    && context.measureText(safeText).width > cue.canvas.width - 38
  ) {
    fontSize -= 1;
    context.font = `600 ${fontSize}px system-ui, sans-serif`;
  }

  context.fillStyle = '#f0f5ef';
  context.textAlign = 'center';
  context.textBaseline = 'middle';
  context.fillText(
    safeText,
    cue.canvas.width * 0.5,
    cue.canvas.height * 0.5,
  );
  cue.texture.needsUpdate = true;
}

export class ResidentWorldLayer {
  readonly group = new THREE.Group();

  private readonly loader = new GLTFLoader();
  private readonly actors = new Map<string, ResidentActor>();
  private readonly selectionRing = new THREE.Mesh(
    new THREE.RingGeometry(0.62, 0.84, 36),
    new THREE.MeshBasicMaterial({
      color: 0xdde9c8,
      transparent: true,
      opacity: 0.72,
      depthWrite: false,
      side: THREE.DoubleSide,
    }),
  );
  private selectedResidentId: string | null = null;
  private template: THREE.Group | null = null;
  private clips: THREE.AnimationClip[] = [];
  private ready = false;
  private pendingResidents: Resident[] = [];
  private pendingTerrain: TerrainWindow | null = null;
  private pendingCenterX = 0;
  private pendingCenterY = 0;
  private simulationSpeed: SimulationSpeed =
    SIMULATION_TIME_CONTRACT.defaultSpeed;

  constructor() {
    this.selectionRing.rotation.x = -Math.PI * 0.5;
    this.selectionRing.visible = false;
    this.selectionRing.renderOrder = 4;
    this.group.add(this.selectionRing);
    void this.loadAssets();
  }

  setResidents(
    residents: Resident[],
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    const previousCenterX = this.pendingCenterX;
    const previousCenterY = this.pendingCenterY;
    const centerChanged = this.pendingTerrain !== null
      && (
        previousCenterX !== centerX
        || previousCenterY !== centerY
      );

    if (centerChanged) {
      const offsetX = (previousCenterX - centerX) * WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      const offsetZ = (previousCenterY - centerY) * WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      for (const actor of this.actors.values()) {
        if (!actor.initialized) continue;
        actor.current.x += offsetX;
        actor.current.z += offsetZ;
        actor.target.x += offsetX;
        actor.target.z += offsetZ;
        actor.root.position.copy(actor.current);
      }
    }

    this.pendingResidents = residents;
    this.pendingTerrain = terrain;
    this.pendingCenterX = centerX;
    this.pendingCenterY = centerY;

    if (!this.ready || !this.template) return;

    const sampleElevation = createTerrainElevationSampler(terrain);
    const activeIds = new Set(residents.map((resident) => resident.id));

    for (const [id, actor] of this.actors) {
      actor.root.visible = activeIds.has(id);
      if (actor.actionCue) actor.actionCue.sprite.visible = false;
    }

    for (const resident of residents) {
      if (
        !resident.hasPosition
        || resident.gridX === undefined
        || resident.gridY === undefined
      ) {
        continue;
      }

      const gridCellsPerChunk = WORLD_GRID_CONTRACT.gridCellsPerChunk;
      const chunkX = Math.floor(resident.gridX / gridCellsPerChunk);
      const chunkY = Math.floor(resident.gridY / gridCellsPerChunk);
      const localX = (
        resident.gridX - (chunkX * gridCellsPerChunk)
      ) / gridCellsPerChunk;
      const localY = (
        resident.gridY - (chunkY * gridCellsPerChunk)
      ) / gridCellsPerChunk;
      const elevation = sampleElevation(
        chunkX,
        chunkY,
        localX,
        localY,
      );
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
        const targetDelta = next.clone().sub(actor.target);
        const targetChanged = targetDelta.lengthSq()
          > (
            RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits
            ** 2
          );

        if (targetChanged) {
          const travelDelta = next.clone().sub(actor.current);
          const travelDistance = travelDelta.length();

          if (
            travelDistance
            > RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits
          ) {
            actor.targetYaw =
              Math.atan2(travelDelta.x, travelDelta.z)
              + RESIDENT_PRESENTATION_CONTRACT.modelForwardYawOffsetRadians;

            const effectiveSpeed = Math.max(
              SIMULATION_TIME_CONTRACT.defaultSpeed,
              this.simulationSpeed,
            );
            const locomotionBudget =
              RESIDENT_VISUAL_SPEED_WORLD_UNITS_PER_SECOND_AT_1X
              * effectiveSpeed;
            const presentationSeconds =
              RESIDENT_PRESENTATION_CONTRACT.movementSampleSeconds
              + RESIDENT_PRESENTATION_CONTRACT.targetArrivalPaddingSeconds;

            actor.targetTravelSpeedWorldUnitsPerSecond = Math.min(
              locomotionBudget,
              travelDistance / Math.max(0.001, presentationSeconds),
            );
            actor.walkGraceRemainingSeconds =
              RESIDENT_PRESENTATION_CONTRACT.walkStopGraceSeconds;
          }
        }
      }

      actor.activityLabel = resident.activityLabel ?? 'Idle';
      actor.activityTargetId = resident.activityTargetId ?? '';
      actor.presentation = resident.presentation ?? null;
      this.updateActionCue(actor, resident, residents);
      actor.target.copy(next);

      if (!actor.initialized) {
        actor.current.copy(next);
        actor.root.position.copy(next);
        actor.targetYaw = actor.root.rotation.y;
        actor.targetTravelSpeedWorldUnitsPerSecond = 0;
        actor.smoothedTravelSpeedWorldUnitsPerSecond = 0;
        actor.walkGraceRemainingSeconds = 0;
        actor.initialized = true;
      }

      actor.root.visible = true;
    }
  }

  setSimulationSpeed(speed: number): void {
    this.simulationSpeed = normalizeSimulationSpeed(speed);
  }

  setSelectedResident(residentId: string | null): void {
    this.selectedResidentId = residentId;
    this.updateSelectionRing();
  }

  pickResident(raycaster: THREE.Raycaster): string | null {
    const roots = [...this.actors.values()]
      .filter((actor) => actor.root.visible)
      .map((actor) => actor.root);
    const intersections = raycaster.intersectObjects(roots, true);

    for (const intersection of intersections) {
      let current: THREE.Object3D | null = intersection.object;
      while (current && current !== this.group) {
        const residentId = current.userData.residentId;
        if (typeof residentId === 'string') return residentId;
        current = current.parent;
      }
    }

    return null;
  }

  update(deltaSeconds: number): void {
    const dt = Math.min(
      RESIDENT_PRESENTATION_CONTRACT.maxAnimationDeltaSeconds,
      Math.max(0, deltaSeconds),
    );
    for (const actor of this.actors.values()) {
      if (!actor.root.visible) continue;

      const delta = actor.target.clone().sub(actor.current);
      const distance = delta.length();
      const moving =
        distance > RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits;

      const desiredTravelSpeed = moving
        ? actor.targetTravelSpeedWorldUnitsPerSecond
        : 0;
      const speedBlend = 1 - Math.exp(
        -RESIDENT_PRESENTATION_CONTRACT.speedResponsivenessPerSecond * dt,
      );
      actor.smoothedTravelSpeedWorldUnitsPerSecond += (
        desiredTravelSpeed
        - actor.smoothedTravelSpeedWorldUnitsPerSecond
      ) * speedBlend;

      if (moving) {
        actor.walkGraceRemainingSeconds =
          RESIDENT_PRESENTATION_CONTRACT.walkStopGraceSeconds;

        const yawDelta = Math.atan2(
          Math.sin(actor.targetYaw - actor.root.rotation.y),
          Math.cos(actor.targetYaw - actor.root.rotation.y),
        );
        const turnBlend = 1 - Math.exp(
          -RESIDENT_PRESENTATION_CONTRACT.turnResponsivenessPerSecond * dt,
        );
        actor.root.rotation.y += yawDelta * turnBlend;

        const maxDistance =
          actor.smoothedTravelSpeedWorldUnitsPerSecond * dt;
        if (distance <= maxDistance) {
          actor.current.copy(actor.target);
        } else if (maxDistance > 0) {
          actor.current.addScaledVector(
            delta.multiplyScalar(1 / distance),
            maxDistance,
          );
        }
      } else {
        actor.targetTravelSpeedWorldUnitsPerSecond = 0;
        actor.walkGraceRemainingSeconds = Math.max(
          0,
          actor.walkGraceRemainingSeconds - dt,
        );

        const interactionYaw = this.interactionTargetYaw(actor);
        if (interactionYaw !== null) {
          const yawDelta = Math.atan2(
            Math.sin(interactionYaw - actor.root.rotation.y),
            Math.cos(interactionYaw - actor.root.rotation.y),
          );
          const turnBlend = 1 - Math.exp(
            -RESIDENT_PRESENTATION_CONTRACT.turnResponsivenessPerSecond * dt,
          );
          actor.root.rotation.y += yawDelta * turnBlend;
        }
      }

      if (
        this.simulationSpeed > 0
        && actor.presentation?.active
        && actor.presentation.phase === 'Moving'
      ) {
        actor.walkGraceRemainingSeconds = Math.max(
          actor.walkGraceRemainingSeconds,
          RESIDENT_PRESENTATION_CONTRACT.walkStopGraceSeconds,
        );
      }

      actor.root.position.copy(actor.current);
      if (actor.actionCue) {
        actor.actionCue.sprite.position.set(
          actor.current.x,
          actor.current.y + 2.0,
          actor.current.z,
        );
      }
      const presentationMoving =
        moving || actor.walkGraceRemainingSeconds > 0;
      this.syncWalkPlaybackRate(actor);
      this.setAction(actor, presentationMoving);
      actor.mixer.update(dt);
    }

    this.updateSelectionRing();
  }

  dispose(): void {
    for (const actor of this.actors.values()) {
      actor.root.traverse((object) => {
        if (!(object instanceof THREE.Mesh)) return;
        const materials = Array.isArray(object.material)
          ? object.material
          : [object.material];
        for (const material of materials) material.dispose();
        if (object.userData.residentOwnsGeometry) object.geometry.dispose();
      });
    }
    for (const actor of this.actors.values()) {
      if (!actor.actionCue) continue;
      this.group.remove(actor.actionCue.sprite);
      actor.actionCue.texture.dispose();
      actor.actionCue.sprite.material.dispose();
    }
    this.actors.clear();
    this.selectionRing.geometry.dispose();
    const selectionMaterial = this.selectionRing.material;
    if (!Array.isArray(selectionMaterial)) selectionMaterial.dispose();
  }

  private updateSelectionRing(): void {
    const actor = this.selectedResidentId
      ? this.actors.get(this.selectedResidentId)
      : undefined;

    if (!actor?.root.visible || !actor.initialized) {
      this.selectionRing.visible = false;
      return;
    }

    this.selectionRing.position.set(
      actor.current.x,
      actor.current.y + 0.035,
      actor.current.z,
    );
    this.selectionRing.visible = true;
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

    const appearance = createResidentAppearanceProfile(resident);
    const root = new THREE.Group();
    root.userData.residentId = resident.id;

    const model = cloneSkeleton(this.template) as THREE.Group;
    applyResidentMaterialVariant(model, appearance);

    const visual = new THREE.Group();
    visual.name = 'LifeLensResidentVisual';
    visual.add(model);
    root.add(visual);

    root.scale.set(
      appearance.heightWorldUnits * appearance.widthScale,
      appearance.heightWorldUnits,
      appearance.heightWorldUnits * appearance.depthScale,
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

    const idlePhase01 = (appearance.seed % 997) / 997;
    const walkPhase01 = ((appearance.seed >>> 8) % 991) / 991;

    idle?.play();
    if (idle) {
      idle.time = idlePhase01
        * Math.max(0.001, idle.getClip().duration);
    }
    if (walk) {
      walk.time = walkPhase01
        * Math.max(0.001, walk.getClip().duration);
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
      activityTargetId: resident.activityTargetId ?? '',
      presentation: resident.presentation ?? null,
      actionCue: null,
      current: new THREE.Vector3(),
      target: new THREE.Vector3(),
      targetYaw: 0,
      targetTravelSpeedWorldUnitsPerSecond: 0,
      smoothedTravelSpeedWorldUnitsPerSecond: 0,
      walkGraceRemainingSeconds: 0,
      gaitRateBias: appearance.gaitRateBias,
      initialized: false,
    };

    const actionCue = createActionCueSprite();
    actor.actionCue = actionCue;
    if (actionCue) this.group.add(actionCue.sprite);

    this.actors.set(resident.id, actor);
    this.group.add(root);
    return actor;
  }

  private updateActionCue(
    actor: ResidentActor,
    resident: Resident,
    residents: Resident[],
  ): void {
    const cue = residentActionCue(resident, residents);
    if (!actor.actionCue || !cue) {
      if (actor.actionCue) actor.actionCue.sprite.visible = false;
      return;
    }

    paintActionCue(actor.actionCue, cue.text, cue.phase);
    actor.actionCue.sprite.visible = true;
  }

  private interactionTargetYaw(actor: ResidentActor): number | null {
    const presentation = actor.presentation;
    if (
      !presentation?.active
      || presentation.phase !== 'Interacting'
    ) {
      return null;
    }

    let targetX: number | null = null;
    let targetZ: number | null = null;
    const targetResidentId = presentation.targetResidentId ?? '';
    const targetActor = targetResidentId && targetResidentId !== '0'
      ? this.actors.get(targetResidentId)
      : undefined;

    if (targetActor?.root.visible && targetActor.initialized) {
      targetX = targetActor.current.x;
      targetZ = targetActor.current.z;
    } else if (
      presentation.hasTargetGrid
      && typeof presentation.targetGridX === 'number'
      && typeof presentation.targetGridY === 'number'
    ) {
      const gridCellsPerChunk = WORLD_GRID_CONTRACT.gridCellsPerChunk;
      const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      const chunkX = Math.floor(
        presentation.targetGridX / gridCellsPerChunk,
      );
      const chunkY = Math.floor(
        presentation.targetGridY / gridCellsPerChunk,
      );
      const localX = (
        presentation.targetGridX - (chunkX * gridCellsPerChunk)
      ) / gridCellsPerChunk;
      const localY = (
        presentation.targetGridY - (chunkY * gridCellsPerChunk)
      ) / gridCellsPerChunk;
      targetX = (
        chunkX - this.pendingCenterX + localX - 0.5
      ) * chunkWorldSize;
      targetZ = (
        chunkY - this.pendingCenterY + localY - 0.5
      ) * chunkWorldSize;
    }

    if (targetX === null || targetZ === null) return null;
    const dx = targetX - actor.current.x;
    const dz = targetZ - actor.current.z;
    if (
      (dx * dx) + (dz * dz)
      <= RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits ** 2
    ) {
      return null;
    }

    return (
      Math.atan2(dx, dz)
      + RESIDENT_PRESENTATION_CONTRACT.modelForwardYawOffsetRadians
    );
  }

  private syncWalkPlaybackRate(actor: ResidentActor): void {
    if (!actor.walk) return;

    const referenceSpeed = Math.max(
      0.001,
      RESIDENT_PRESENTATION_CONTRACT.walkReferenceSpeedWorldUnitsPerSecond,
    );
    const normalizedSpeed =
      actor.smoothedTravelSpeedWorldUnitsPerSecond / referenceSpeed;
    const timeScale = THREE.MathUtils.clamp(
      normalizedSpeed * actor.gaitRateBias,
      RESIDENT_PRESENTATION_CONTRACT.walkMinTimeScale,
      RESIDENT_PRESENTATION_CONTRACT.walkMaxTimeScale,
    );
    actor.walk.setEffectiveTimeScale(timeScale);
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
    const presentation = actor.presentation;
    if (
      !presentation?.active
      || presentation.phase !== 'Interacting'
    ) {
      return 'idle';
    }

    const targetResidentId = presentation.targetResidentId ?? '';
    const targetActor = targetResidentId && targetResidentId !== '0'
      ? this.actors.get(targetResidentId)
      : undefined;
    const nearbyResident = Boolean(
      targetActor?.root.visible
      && targetActor.initialized
      && actor.current.distanceTo(targetActor.current) <= 3,
    );

    if (
      nearbyResident
      && actor.talk
      && (
        presentation.kind === 'KnowledgeTeaching'
        || presentation.kind === 'Social'
      )
    ) {
      return 'talk';
    }

    // Object-bound physical/civilization/parenting motions remain neutral in
    // this tranche. The directive makes their intent visible, but we do not
    // invent a chair, bed, toilet, tool alignment or hand interaction.
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

    const blendSeconds =
      RESIDENT_PRESENTATION_CONTRACT.animationCrossFadeSeconds;
    previous?.fadeOut(blendSeconds);
    next.enabled = true;
    next.play().fadeIn(blendSeconds);
    actor.active = desired;
  }
}
