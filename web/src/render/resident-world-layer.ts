import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { clone as cloneSkeleton } from 'three/addons/utils/SkeletonUtils.js';
import type {
  Resident,
  CivilizationWorldPayload,
  ResidentPresentationDirective,
  TerrainWindow,
} from '../runtime/core-types';
import {
  RESIDENT_PRESENTATION_CONTRACT,
  RESIDENT_VISUAL_SPEED_WORLD_UNITS_PER_SECOND_AT_1X,
  SIMULATION_TIME_CONTRACT,
  WORLD_GRID_CONTRACT,
  normalizeSimulationSpeed,
  residentPresentationMotionTimeScale,
} from '../runtime/lifelens-contract';
import type { SimulationSpeed } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';
import {
  applyResidentMaterialVariant,
  createResidentAppearanceProfile,
} from './resident-appearance';
import { ResidentInventoryProps } from './resident-props';
import { residentActionCue } from './resident-action-context';
import {
  residentSleepPostureActive,
  resolveResidentSemanticMotion,
  type ResidentSemanticMotion,
} from './resident-semantic-motion';
import {
  residentSocialCuePairs,
  type ResidentSocialCueKind,
} from './resident-social-cues';
import { createResidentMotionLibrary, residentGestureRate } from './resident-motion-library';
import { calibrateResidentSleep, residentSleepFallbackClip, residentStandingFallbackClip, ResidentSleepMotion, type ResidentSleepCalibration } from './resident-sleep-motion';
import { residentToWorldPosition } from './resident-world-coordinates';
import type { SocialEventAnchor } from './social-event-presentation';

const BASE_MODEL_COMMIT = 'ddd5fc34a445bcded3cf9836607aaeebc19a5c78';
const BASE_MODEL_URL =
  `https://raw.githubusercontent.com/programasweights/avatar/${BASE_MODEL_COMMIT}/public/assets/character.glb`;

const ANIMATION_COMMIT = 'aa02a4e6d8337a0604d2da131bcbbeb1f01badf0';
const ANIMATION_URL =
  `https://raw.githubusercontent.com/Seyamalam/blood-league-kickoff/${ANIMATION_COMMIT}/public/assets/vendor/quaternius/universal-animation-library.glb`;

// Quaternius Universal Animation Library 2 Standard, CC0 1.0.
// Official source: https://quaternius.com/packs/universalanimationlibrary2.html
// Pinned public mirror commit contains only the CC0 animation GLB we consume;
// unrelated avatar meshes in that repository are never requested.
const ANIMATION2_COMMIT = '84fd636910bf713099010efbab7f3c84550f4bcb';
const ANIMATION2_URL =
  `https://raw.githubusercontent.com/richardanaya/metaverse-avatar/${ANIMATION2_COMMIT}/anims/UAL2_Standard.glb`;

type MotionName = ResidentSemanticMotion;

interface ResidentActionCueSprite {
  sprite: THREE.Sprite;
  texture: THREE.CanvasTexture;
  canvas: HTMLCanvasElement;
  text: string;
}

interface ResidentSocialConnector {
  line: THREE.Line<THREE.BufferGeometry, THREE.LineBasicMaterial>;
  sourceId: string;
  targetId: string;
  kind: ResidentSocialCueKind;
}

interface ResidentActor {
  root: THREE.Group;
  visual: THREE.Group;
  model: THREE.Group;
  appearanceFacts: Resident;
  appearanceSignature: string;
  inventoryProps: ResidentInventoryProps;
  mixer: THREE.AnimationMixer;
  actions: Map<MotionName, THREE.AnimationAction>;
  uniqueActions: THREE.AnimationAction[];
  sleepMotion: ResidentSleepMotion;
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
  displayedTravelSpeedWorldUnitsPerSecond: number;
  walkGraceRemainingSeconds: number;
  gaitRateBias: number;
  sleepSupportHeightWorldUnits: number;
  initialized: boolean;
}

function createActionCueSprite(): ResidentActionCueSprite | null {
  if (typeof document === 'undefined') return null;

  const canvas = document.createElement('canvas');
  canvas.width = 768;
  canvas.height = 144;
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
  sprite.scale.set(4.8, 0.9, 1);
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
  let fontSize = 42;
  context.font = `600 ${fontSize}px system-ui, sans-serif`;
  while (
    fontSize > 29
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
  private readonly eventResidentIds = new Set<string>();
  private readonly socialConnectors =
    new Map<string, ResidentSocialConnector>();
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
  private motionClips = new Map<MotionName, THREE.AnimationClip>();
  private sleepClip: THREE.AnimationClip | null = null;
  private sleepCalibration: ResidentSleepCalibration | null = null;
  private readonly movementDelta = new THREE.Vector3();
  private disposed = false;
  private readonly interactionSites = new Map<string, { gridX: number; gridY: number }>();
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
    this.eventResidentIds.clear();
    for (const resident of residents) {
      if (resident.alive !== false && resident.hasPosition
        && Number.isFinite(resident.gridX) && Number.isFinite(resident.gridY)) {
        this.eventResidentIds.add(resident.id);
      }
    }
    this.pendingTerrain = terrain;
    this.pendingCenterX = centerX;
    this.pendingCenterY = centerY;

    if (!this.ready || !this.template) return;

    const sampleElevation = createTerrainElevationSampler(terrain);
    for (const actor of this.actors.values()) {
      actor.root.visible = false;
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
              travelDistance / Math.max(0.001, presentationSeconds) * effectiveSpeed,
            );
            actor.targetTravelSpeedWorldUnitsPerSecond /= effectiveSpeed;
            actor.walkGraceRemainingSeconds =
              RESIDENT_PRESENTATION_CONTRACT.walkStopGraceSeconds;
          }
        }
      }

      actor.activityLabel = resident.activityLabel ?? 'Idle';
      actor.activityTargetId = resident.activityTargetId ?? '';
      actor.presentation = resident.presentation ?? null;
      actor.sleepSupportHeightWorldUnits =
        this.sleepSupportHeightWorldUnits(
          actor,
          resident,
          terrain,
          sampleElevation,
          next,
          centerX,
          centerY,
        );
      const visualDx = next.x - actor.current.x;
      const visualDz = next.z - actor.current.z;
      const visuallyMovingToAuthoritativePosition =
        actor.initialized
        && (
          (visualDx * visualDx) + (visualDz * visualDz)
          > RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits ** 2
        );
      this.updateActionCue(
        actor,
        resident,
        residents,
        visuallyMovingToAuthoritativePosition,
      );
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

    // Keep a small re-entry cache, not an unbounded skeleton for every resident
    // ever seen while panning or across generations.
    const inactive = [...this.actors.entries()].filter(([, actor]) => !actor.root.visible);
    for (const [id, actor] of inactive.slice(0, Math.max(0, inactive.length - 32))) this.releaseActor(id, actor);
    this.syncSocialConnectors(residents);
  }

  setCivilization(civilization: CivilizationWorldPayload): void {
    this.interactionSites.clear();
    if (!civilization.available) return;
    for (const resource of civilization.resources ?? []) this.interactionSites.set(`resource:${resource.id}`, resource);
    for (const facility of civilization.facilities ?? []) this.interactionSites.set(`facility:${facility.id}`, facility);
    for (const storage of civilization.storages ?? []) this.interactionSites.set(`storage:${storage.id}`, storage);
  }

  // Read the current interpolated actor position, never its future Core target.
  // Return a copy so presentation snapshots cannot mutate resident movement.
  socialEventAnchor(id: string): SocialEventAnchor | null {
    const actor = this.actors.get(id);
    if (!this.eventResidentIds.has(id) || !actor?.initialized || !actor.root.visible) return null;
    return { x: actor.current.x, y: actor.current.y, z: actor.current.z };
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
    const motionTimeScale =
      residentPresentationMotionTimeScale(this.simulationSpeed);
    const motionDt = dt * motionTimeScale;

    for (const actor of this.actors.values()) {
      if (!actor.root.visible) continue;

      const delta = this.movementDelta.subVectors(actor.target, actor.current);
      const distance = delta.length();
      const moving =
        distance > RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits;

      actor.displayedTravelSpeedWorldUnitsPerSecond = 0;
      const desiredTravelSpeed = moving
        ? actor.targetTravelSpeedWorldUnitsPerSecond * this.simulationSpeed
        : 0;
      const speedBlend = 1 - Math.exp(
        -RESIDENT_PRESENTATION_CONTRACT.speedResponsivenessPerSecond
          * motionDt,
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
          -RESIDENT_PRESENTATION_CONTRACT.turnResponsivenessPerSecond
            * motionDt,
        );
        if (!actor.sleepMotion.active) actor.root.rotation.y += yawDelta * turnBlend;

        // Do not translate backward while smoothing a large heading reversal.
        // This is interpolation only; the Core target and access cell are intact.
        const headingError = actor.targetYaw - actor.root.rotation.y;
        const forwardFraction = actor.sleepMotion.active ? 1 : Math.max(0, Math.cos(headingError));
        const maxDistance =
          actor.smoothedTravelSpeedWorldUnitsPerSecond * dt
          * (this.simulationSpeed > 0 ? forwardFraction : 0);
        actor.displayedTravelSpeedWorldUnitsPerSecond = Math.min(distance, maxDistance) / Math.max(.001, dt);
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
          actor.walkGraceRemainingSeconds - motionDt,
        );

        const interactionYaw = this.interactionTargetYaw(actor);
        if (interactionYaw !== null) {
          const yawDelta = Math.atan2(
            Math.sin(interactionYaw - actor.root.rotation.y),
            Math.cos(interactionYaw - actor.root.rotation.y),
          );
          const turnBlend = 1 - Math.exp(
            -RESIDENT_PRESENTATION_CONTRACT.turnResponsivenessPerSecond
              * motionDt,
          );
          if (!actor.sleepMotion.active) actor.root.rotation.y += yawDelta * turnBlend;
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
      actor.inventoryProps.setVisuallyMoving(presentationMoving);
      const sleeping = residentSleepPostureActive(
        actor.presentation,
        presentationMoving,
      );

      const sleepWasActive = actor.sleepMotion.active;
      const sleepHandled = actor.sleepMotion.update(sleeping, motionDt,
        actor.sleepSupportHeightWorldUnits, actor.root.scale.y,
        actor.mixer, actor.uniqueActions);
      if (sleepHandled) {
        actor.active = 'sleep';
      } else {
        if (sleepWasActive || actor.active === 'sleep') actor.active = '';
        this.syncWalkPlaybackRate(actor, motionTimeScale);
        this.setAction(actor, presentationMoving);
        actor.mixer.update(motionDt);
      }
      actor.inventoryProps.update();
    }

    this.updateSocialConnectors();
    this.updateSelectionRing();
  }

  dispose(): void {
    this.disposed = true;
    this.eventResidentIds.clear();
    for (const connector of this.socialConnectors.values()) {
      this.group.remove(connector.line);
      connector.line.geometry.dispose();
      connector.line.material.dispose();
    }
    this.socialConnectors.clear();

    for (const [id, actor] of this.actors) this.releaseActor(id, actor);
    this.actors.clear();
    this.selectionRing.geometry.dispose();
    const selectionMaterial = this.selectionRing.material;
    if (!Array.isArray(selectionMaterial)) selectionMaterial.dispose();
  }

  private releaseActor(id: string, actor: ResidentActor): void {
    actor.mixer.stopAllAction(); actor.mixer.uncacheRoot(actor.root);
    actor.inventoryProps.dispose();
    actor.root.traverse(object => {
      if (!(object instanceof THREE.Mesh)) return;
      for (const material of Array.isArray(object.material) ? object.material : [object.material]) material.dispose();
      if (object.userData.residentOwnsGeometry) object.geometry.dispose();
    });
    if (actor.actionCue) {
      this.group.remove(actor.actionCue.sprite); actor.actionCue.texture.dispose(); actor.actionCue.sprite.material.dispose();
    }
    this.group.remove(actor.root); this.actors.delete(id);
  }

  private socialCueColor(kind: ResidentSocialCueKind): number {
    switch (kind) {
      case 'Approach':
        return RESIDENT_PRESENTATION_CONTRACT.socialConnectorApproachColorHex;
      case 'Comfort':
        return RESIDENT_PRESENTATION_CONTRACT.socialConnectorComfortColorHex;
      case 'Repair':
        return RESIDENT_PRESENTATION_CONTRACT.socialConnectorRepairColorHex;
      case 'KnowledgeTeaching':
        return RESIDENT_PRESENTATION_CONTRACT.socialConnectorTeachingColorHex;
      case 'Parenting':
        return RESIDENT_PRESENTATION_CONTRACT.socialConnectorParentingColorHex;
      case 'Social':
      default:
        return RESIDENT_PRESENTATION_CONTRACT.socialConnectorDefaultColorHex;
    }
  }

  private createSocialConnector(
    sourceId: string,
    targetId: string,
    kind: ResidentSocialCueKind,
  ): ResidentSocialConnector {
    const geometry = new THREE.BufferGeometry();
    const positions = new THREE.Float32BufferAttribute(
      new Float32Array(9),
      3,
    );
    positions.setUsage(THREE.DynamicDrawUsage);
    geometry.setAttribute('position', positions);

    const material = new THREE.LineBasicMaterial({
      color: this.socialCueColor(kind),
      transparent: true,
      opacity: 0.46,
      depthWrite: false,
      depthTest: true,
      toneMapped: false,
    });
    const line = new THREE.Line(geometry, material);
    line.renderOrder = 6;
    line.frustumCulled = false;
    line.visible = false;
    this.group.add(line);

    return {
      line,
      sourceId,
      targetId,
      kind,
    };
  }

  private syncSocialConnectors(residents: Resident[]): void {
    const pairs = residentSocialCuePairs(residents);
    const activeKeys = new Set(pairs.map((pair) => pair.key));

    for (const pair of pairs) {
      const existing = this.socialConnectors.get(pair.key);
      if (existing) {
        existing.sourceId = pair.sourceId;
        existing.targetId = pair.targetId;
        if (existing.kind !== pair.kind) {
          existing.kind = pair.kind;
          existing.line.material.color.setHex(
            this.socialCueColor(pair.kind),
          );
        }
        continue;
      }

      this.socialConnectors.set(
        pair.key,
        this.createSocialConnector(
          pair.sourceId,
          pair.targetId,
          pair.kind,
        ),
      );
    }

    for (const [key, connector] of this.socialConnectors) {
      if (activeKeys.has(key)) continue;
      this.group.remove(connector.line);
      connector.line.geometry.dispose();
      connector.line.material.dispose();
      this.socialConnectors.delete(key);
    }

    this.updateSocialConnectors();
  }

  private updateSocialConnectors(): void {
    for (const connector of this.socialConnectors.values()) {
      const source = this.actors.get(connector.sourceId);
      const target = this.actors.get(connector.targetId);

      if (
        !source?.root.visible
        || !target?.root.visible
        || !source.initialized
        || !target.initialized
      ) {
        connector.line.visible = false;
        continue;
      }

      if (source.current.distanceToSquared(source.target) > RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits ** 2
        || source.sleepMotion.active) {
        connector.line.visible = false;
        continue;
      }
      const horizontalDistance = Math.hypot(
        target.current.x - source.current.x,
        target.current.z - source.current.z,
      );
      if (horizontalDistance > 4.5 || horizontalDistance < 0.08) {
        connector.line.visible = false;
        continue;
      }

      const sourceY = source.current.y + 1.18;
      const targetY = target.current.y + 1.18;
      const midpointLift = Math.min(
        0.42,
        0.18 + horizontalDistance * 0.08,
      );

      const positions = connector.line.geometry.getAttribute(
        'position',
      ) as THREE.BufferAttribute;
      positions.setXYZ(
        0,
        source.current.x,
        sourceY,
        source.current.z,
      );
      positions.setXYZ(
        1,
        (source.current.x + target.current.x) * 0.5,
        ((sourceY + targetY) * 0.5) + midpointLift,
        (source.current.z + target.current.z) * 0.5,
      );
      positions.setXYZ(
        2,
        target.current.x,
        targetY,
        target.current.z,
      );
      positions.needsUpdate = true;
      connector.line.visible = true;
    }
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

  private async loadCharacterAsset(file: string, fallback: string) {
    try {
      return await this.loader.loadAsync(`${import.meta.env.BASE_URL}vendor/characters/${file}`);
    } catch {
      return this.loader.loadAsync(fallback);
    }
  }

  private async loadAssets(): Promise<void> {
    try {
      const base = await this.loadCharacterAsset('character.glb', BASE_MODEL_URL);

      let animationClips: THREE.AnimationClip[] = [];
      try {
        const animationAsset = await this.loadCharacterAsset('ual1.glb', ANIMATION_URL);
        animationClips = animationAsset.animations;
      } catch (error) {
        console.warn(
          'LifeLens Three World animation asset unavailable',
          error,
        );
      }

      try {
        const animation2Asset = await this.loadCharacterAsset('ual2.glb', ANIMATION2_URL);
        animationClips = [
          ...animationClips,
          ...animation2Asset.animations,
        ];
      } catch (error) {
        // UAL2 is a presentation enhancement. Failure must never take down the
        // resident layer or remove the proven UAL1 locomotion baseline.
        console.warn(
          'LifeLens Three World UAL2 animation asset unavailable',
          error,
        );
      }

      if (this.disposed) return;
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
      this.motionClips = createResidentMotionLibrary([...base.animations, ...animationClips]);
      if (!this.motionClips.has('idle')) this.motionClips.set('idle', residentStandingFallbackClip());
      this.sleepClip = animationClips.find(clip => clip.name === 'LayToIdle') ?? residentSleepFallbackClip();
      // A single shared calibration. The temporary skeleton is not retained per resident.
      const calibrationModel = cloneSkeleton(normalized) as THREE.Group;
      this.sleepCalibration = calibrateResidentSleep(calibrationModel, this.sleepClip);
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
    if (existing) {
      this.actors.delete(resident.id); this.actors.set(resident.id, existing);
      // Full observations can arrive after the lightweight runtime summary.
      // Preserve known phenotype fields when a partial summary omits them.
      const facts = { ...existing.appearanceFacts };
      if (resident.sex !== undefined) facts.sex = resident.sex;
      if (resident.ageYears !== undefined) facts.ageYears = resident.ageYears;
      if (resident.genetics !== undefined) facts.genetics = { ...facts.genetics, ...resident.genetics };
      const appearance = createResidentAppearanceProfile(facts);
      const signature = JSON.stringify(appearance);
      if (signature !== existing.appearanceSignature) {
        applyResidentMaterialVariant(existing.model, appearance);
        existing.root.scale.set(appearance.heightWorldUnits * appearance.widthScale,
          appearance.heightWorldUnits, appearance.heightWorldUnits * appearance.depthScale);
        existing.gaitRateBias = appearance.gaitRateBias;
        existing.appearanceSignature = signature;
      }
      existing.appearanceFacts = facts;
      // Missing inventory is unknown: do not leave an already-consumed tool visible.
      existing.inventoryProps.setInventory(resident.civilization?.inventory, resident.presentation);
      return existing;
    }
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
    const actions = new Map<MotionName, THREE.AnimationAction>();
    for (const [motion, clip] of this.motionClips) {
      actions.set(motion, mixer.clipAction(clip, root).setLoop(THREE.LoopRepeat, Infinity));
    }
    if (!this.sleepClip || !this.sleepCalibration) throw new Error('Resident sleep calibration unavailable');
    const sleepAction = mixer.clipAction(this.sleepClip, root);
    actions.set('sleep', sleepAction);
    const uniqueActions = [...new Set(actions.values())];
    const sleepMotion = new ResidentSleepMotion(sleepAction, this.sleepCalibration, visual);
    const idle = actions.get('idle'), walk = actions.get('walk');
    idle?.play();
    if (idle) idle.time = (appearance.seed % 997) / 997 * Math.max(.001, idle.getClip().duration);
    if (walk) walk.time = ((appearance.seed >>> 8) % 991) / 991 * Math.max(.001, walk.getClip().duration);

    const inventoryProps = new ResidentInventoryProps(visual, model);
    inventoryProps.setInventory(resident.civilization?.inventory, resident.presentation);
    const actor: ResidentActor = {
      root,
      visual,
      model,
      appearanceFacts: resident,
      appearanceSignature: JSON.stringify(appearance),
      inventoryProps,
      mixer,
      actions,
      uniqueActions,
      sleepMotion,
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
      displayedTravelSpeedWorldUnitsPerSecond: 0,
      walkGraceRemainingSeconds: 0,
      gaitRateBias: appearance.gaitRateBias,
      sleepSupportHeightWorldUnits: 0,
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
    visuallyMoving: boolean,
  ): void {
    const cue = residentActionCue(
      resident,
      residents,
      visuallyMoving,
    );
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
      // A Gather destination can be an access cell outside the visible patch.
      // Face the exact Core node center, keeping the resident on its accessGrid.
      const site = presentation.kind === 'Civilization' ? (
        this.interactionSites.get(`resource:${presentation.civilizationResourceNode}`)
        ?? this.interactionSites.get(`facility:${presentation.facilityId}`)
        ?? this.interactionSites.get(`storage:${presentation.civilizationStorage}`)
      ) : undefined;
      const facingX = site?.gridX ?? presentation.targetGridX;
      const facingY = site?.gridY ?? presentation.targetGridY;
      const gridCellsPerChunk = WORLD_GRID_CONTRACT.gridCellsPerChunk;
      const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      const chunkX = Math.floor(
        facingX / gridCellsPerChunk,
      );
      const chunkY = Math.floor(
        facingY / gridCellsPerChunk,
      );
      const localX = (
        facingX - (chunkX * gridCellsPerChunk)
      ) / gridCellsPerChunk;
      const localY = (
        facingY - (chunkY * gridCellsPerChunk)
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

  private sleepSupportHeightWorldUnits(
    actor: ResidentActor,
    resident: Resident,
    terrain: TerrainWindow,
    sampleElevation: ReturnType<typeof createTerrainElevationSampler>,
    position: THREE.Vector3,
    centerX: number,
    centerY: number,
  ): number {
    if (
      !residentSleepPostureActive(actor.presentation, false)
      || resident.gridX === undefined
      || resident.gridY === undefined
    ) {
      return 0;
    }

    const sleepingPlace = (terrain.humanTraces?.entries ?? []).find(
      (trace) => (
        trace.kind === 'Facility'
        && trace.facilityKind === 'SleepingPlace'
        && trace.state === 'Operational'
        && trace.gridX === resident.gridX
        && trace.gridY === resident.gridY
      ),
    );
    const beddingSupport = sleepingPlace
      ? RESIDENT_PRESENTATION_CONTRACT.sleepPoseSleepingPlaceSurfaceHeightWorldUnits
      : 0;

    // Outdoor/shelter sleep needs slope-aware support. The authoritative root
    // remains on Core's exact grid cell; only the visual body is lifted enough
    // to clear the highest terrain point beneath its horizontal footprint.
    const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const elevationScale = WORLD_GRID_CONTRACT.elevationScale;
    const halfLength = Math.max(
      0.1,
      actor.root.scale.y
        * RESIDENT_PRESENTATION_CONTRACT.sleepPoseBodyHalfLengthHeightRatio,
    );
    const sampleWorldHeight = (
      localWorldX: number,
      localWorldZ: number,
    ): number => {
      const gridX = (
        localWorldX / chunkWorldSize
        + centerX
        + 0.5
      ) * span;
      const gridY = (
        localWorldZ / chunkWorldSize
        + centerY
        + 0.5
      ) * span;
      const chunkX = Math.floor(gridX / span);
      const chunkY = Math.floor(gridY / span);
      const localX01 = gridX / span - chunkX;
      const localY01 = gridY / span - chunkY;
      return sampleElevation(
        chunkX,
        chunkY,
        localX01,
        localY01,
      ) * elevationScale;
    };

    const baseHeight = position.y;
    // Cover the whole lying footprint, independent of heading changes during
    // arrival. Snapshot-time terrain sampling only, not per-frame mesh queries.
    let highest = baseHeight;
    for (const x of [-halfLength, 0, halfLength]) {
      for (const z of [-halfLength, 0, halfLength]) {
        highest = Math.max(highest, sampleWorldHeight(position.x + x, position.z + z));
      }
    }
    return Math.max(beddingSupport, highest - baseHeight);
  }

  private syncWalkPlaybackRate(
    actor: ResidentActor,
    motionTimeScale: number,
  ): void {
    const walk = actor.actions.get('walk'), carry = actor.actions.get('carry');
    if (!walk && !carry) return;

    const referenceSpeed = Math.max(
      0.001,
      RESIDENT_PRESENTATION_CONTRACT.walkReferenceSpeedWorldUnitsPerSecond,
    );
    const normalizedSpeed =
      actor.displayedTravelSpeedWorldUnitsPerSecond / referenceSpeed
      / Math.max(1, this.simulationSpeed);
    const maximumLocalTimeScale = motionTimeScale > 0
      ? Math.min(
        RESIDENT_PRESENTATION_CONTRACT.walkMaxTimeScale,
        RESIDENT_PRESENTATION_CONTRACT.maxMotionTimeScale
          / motionTimeScale,
      )
      : RESIDENT_PRESENTATION_CONTRACT.walkMaxTimeScale;
    const timeScale = THREE.MathUtils.clamp(
      normalizedSpeed * actor.gaitRateBias,
      Math.min(
        RESIDENT_PRESENTATION_CONTRACT.walkMinTimeScale,
        maximumLocalTimeScale,
      ),
      maximumLocalTimeScale,
    );
    walk?.setEffectiveTimeScale(timeScale);
    carry?.setEffectiveTimeScale(timeScale);
  }

  private actionFor(
    actor: ResidentActor,
    motion: MotionName,
  ): THREE.AnimationAction | undefined {
    return actor.actions.get(motion);
  }

  private restMotion(actor: ResidentActor): MotionName {
    const presentation = actor.presentation;
    const targetResidentId = presentation?.targetResidentId ?? '';
    const targetActor = targetResidentId && targetResidentId !== '0'
      ? this.actors.get(targetResidentId)
      : undefined;
    const nearbyResident = Boolean(
      targetActor?.root.visible
      && targetActor.initialized
      && actor.current.distanceTo(targetActor.current) <= 3,
    );

    const resolved = resolveResidentSemanticMotion(
      presentation,
      {
        moving: false,
        nearbyResident,
        hasWaterContainer: actor.inventoryProps.hasWaterContainer,
      },
    );

    // Missing vendor clips fail closed to neutral Idle. Core intent remains
    // visible through the action cue, but Presentation never substitutes a
    // semantically unrelated pose just to avoid standing still.
    return this.actionFor(actor, resolved)
      ? resolved
      : 'idle';
  }

  private setAction(actor: ResidentActor, moving: boolean): void {
    let desired: MotionName = (moving || actor.presentation?.phase === 'Moving') && actor.actions.has('walk')
      ? resolveResidentSemanticMotion(
        actor.presentation,
        {
          moving: true,
          nearbyResident: false,
          hasCarriedLoad: actor.inventoryProps.hasCarriedLoad,
        },
      )
      : this.restMotion(actor);

    if (!this.actionFor(actor, desired)) {
      desired = moving && actor.actions.has('walk') ? 'walk' : 'idle';
    }
    if (actor.active === desired) return;

    const previous = actor.active
      ? this.actionFor(actor, actor.active)
      : undefined;
    const next = this.actionFor(actor, desired) ?? actor.actions.get('idle');
    if (!next) return;

    const blendSeconds =
      RESIDENT_PRESENTATION_CONTRACT.animationCrossFadeSeconds;
    if (previous !== next) previous?.fadeOut(blendSeconds);
    next.enabled = true;
    if (desired !== 'walk' && desired !== 'carry') next.setEffectiveTimeScale(residentGestureRate(desired));
    // Keep locomotion phase across brief stops; restart discrete interactions.
    if (desired !== 'walk' && desired !== 'carry' && desired !== 'idle') next.reset();
    if (previous === next) next.play();
    else next.play().fadeIn(blendSeconds);
    actor.active = desired;
  }
}


