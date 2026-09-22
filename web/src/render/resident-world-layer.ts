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
  activityTargetId: string;
  presentation: ResidentPresentationDirective | null;
  current: THREE.Vector3;
  target: THREE.Vector3;
  walkStateGraceSeconds: number;
  statusSprite: THREE.Sprite;
  statusText: string;
  groundShadow: THREE.Mesh;
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

function residentTargetName(
  resident: Resident,
  residentsById: Map<string, Resident>,
): string {
  const targetId = resident.contextAction?.targetResidentId
    ?? resident.presentation?.targetResidentId
    ?? resident.activityTargetId
    ?? '';
  return targetId
    ? (residentsById.get(targetId)?.name ?? resident.activityTargetName ?? '상대')
    : (resident.activityTargetName ?? '상대');
}

function facilityKindKorean(kind: string | undefined): string {
  const labels: Record<string, string> = {
    PrimitiveStorage: '원시 저장소',
    FirePit: '화덕',
    WorkSurface: '작업대',
    SleepingPlace: '잠자리',
    Shelter: '쉼터',
    Furnace: '용광로',
  };
  return kind ? (labels[kind] ?? '시설') : '시설';
}

function materialKorean(material: string | undefined): string {
  const labels: Record<string, string> = {
    Stone: '돌',
    Flint: '부싯돌',
    Wood: '나무',
    Fiber: '섬유',
    Clay: '점토',
    Water: '물',
    PlantFood: '먹을거리',
    Bone: '뼈',
    Hide: '가죽',
    CopperOre: '구리 광석',
    TinOre: '주석 광석',
    IronOre: '철 광석',
    Charcoal: '숯',
    CopperMetal: '구리',
  };
  return material ? (labels[material] ?? '재료') : '재료';
}

function techniqueKorean(technique: string | undefined): string {
  const labels: Record<string, string> = {
    SharpFlake: '날카로운 박편',
    ChippedStoneTool: '뗀석기',
    FireMaking: '불 피우기',
    FiberCordage: '섬유 끈',
    SimpleContainer: '간이 용기',
    DesignatedSanitationArea: '지정 위생구역',
    DugSanitationPit: '구덩이식 위생시설',
    PrimitiveStorage: '원시 저장소',
    DiggingStick: '굴착봉',
    StoneHammer: '돌망치',
    CopperSmelting: '구리 제련',
  };
  return technique ? (labels[technique] ?? '기술') : '기술';
}

function residentActionText(
  resident: Resident,
  residentsById: Map<string, Resident>,
): string {
  const presentation = resident.presentation;
  const action = resident.contextAction;
  const phase = presentation?.phase ?? 'Idle';
  const moving = phase === 'Moving';
  const targetName = residentTargetName(resident, residentsById);

  if (action?.active) {
    switch (action.kind) {
      case 'Social':
        switch (action.socialIntent) {
          case 'Approach': return moving ? `${targetName}에게 가는 중` : `${targetName}와 대화 중`;
          case 'Repair': return moving ? `${targetName}에게 사과하러 가는 중` : `${targetName}에게 사과 중`;
          case 'Comfort': return moving ? `${targetName}을 위로하러 가는 중` : `${targetName}을 위로 중`;
          case 'Avoid': return `${targetName}을 피하는 중`;
          default: return moving ? `${targetName}에게 이동 중` : `${targetName}와 상호작용 중`;
        }
      case 'KnowledgeTeaching':
        return moving
          ? `${targetName}에게 기술을 가르치러 가는 중`
          : `${targetName}에게 ${techniqueKorean(action.technique)} 가르치는 중`;
      case 'Parenting': {
        const labels: Record<string, string> = {
          Feed: '먹이는 중',
          PutToSleep: '재우는 중',
          Bathe: '씻기는 중',
          ToiletAssist: '화장실을 도와주는 중',
          Hold: '안아주는 중',
          Play: '놀아주는 중',
          Educate: '가르치는 중',
          Discipline: '훈육하는 중',
          Comfort: '달래주는 중',
          HealthCare: '돌보는 중',
        };
        return moving
          ? `${targetName}을 돌보러 가는 중`
          : `${targetName}을 ${labels[action.parentingAction ?? ''] ?? '돌보는 중'}`;
      }
      case 'Civilization': {
        const facility = facilityKindKorean(action.facilityKind);
        const buildLabels: Record<string, string> = {
          Plan: `${facility} 자리 정하는 중`,
          DeliverMaterial: `${facility} 재료 운반 중`,
          Work: `${facility} 건설 중`,
          Repair: `${facility} 수리 중`,
          Fuel: `${facility}에 연료 넣는 중`,
          Ignite: `${facility}에 불 붙이는 중`,
          CollectCharcoal: '숯 거두는 중',
          LoadSmeltCharge: '용광로에 광석과 숯 넣는 중',
          CollectMetal: '제련한 구리 거두는 중',
        };
        if (action.facilityAction && action.facilityAction !== 'None') {
          return buildLabels[action.facilityAction] ?? `${facility} 작업 중`;
        }
        switch (action.civilizationIntent) {
          case 'Gather': return `${materialKorean(action.material)} 채집 중`;
          case 'Store': return `${materialKorean(action.material)} 저장 중`;
          case 'Retrieve': return `${materialKorean(action.material)} 꺼내는 중`;
          case 'Experiment': return `${techniqueKorean(action.technique)} 실험 중`;
          case 'Craft': return `${techniqueKorean(action.technique)} 제작 중`;
          default: return '생활 기반 작업 중';
        }
      }
      default:
        break;
    }
  }

  if (presentation?.active && presentation.kind === 'Physical') {
    const movingSuffix = moving ? '하러 가는 중' : '중';
    switch (presentation.physicalGoal) {
      case 'Eat': return moving ? '먹을거리 찾으러 가는 중' : '먹는 중';
      case 'Drink': return moving ? '물을 마시러 가는 중' : '물 마시는 중';
      case 'Sleep': return moving ? '잠자리로 가는 중' : '자는 중';
      case 'Wash': return moving ? '씻으러 가는 중' : '씻는 중';
      case 'UseToilet':
        if (presentation.designatedSanitationSite) {
          return moving ? '위생시설로 가는 중' : '위생시설 이용 중';
        }
        return moving ? '야외 배변 장소로 가는 중' : '야외에서 용변 보는 중';
      default:
        return movingSuffix === '중' ? '생활 행동 중' : '생활 행동하러 가는 중';
    }
  }

  if (resident.civilizationActivity?.active) {
    switch (resident.civilizationActivity.kind) {
      case 'Gather': return `${materialKorean(resident.civilizationActivity.material)} 채집 완료`;
      case 'Store': return `${materialKorean(resident.civilizationActivity.material)} 저장 완료`;
      case 'Retrieve': return `${materialKorean(resident.civilizationActivity.material)} 꺼냄`;
      case 'Experiment': return `${techniqueKorean(resident.civilizationActivity.technique)} 실험`;
      case 'Craft': return `${techniqueKorean(resident.civilizationActivity.technique)} 제작`;
      default: break;
    }
  }

  return '';
}

function makeStatusSprite(): THREE.Sprite {
  const material = new THREE.SpriteMaterial({
    transparent: true,
    depthTest: false,
    depthWrite: false,
  });
  const sprite = new THREE.Sprite(material);
  sprite.scale.set(5.25, 1.08, 1);
  sprite.renderOrder = 20;
  sprite.visible = false;
  return sprite;
}

function updateStatusSprite(sprite: THREE.Sprite, text: string): void {
  if (!text) {
    sprite.visible = false;
    return;
  }

  const canvas = document.createElement('canvas');
  canvas.width = 640;
  canvas.height = 128;
  const context = canvas.getContext('2d');
  if (!context) {
    sprite.visible = false;
    return;
  }

  const background = context.createLinearGradient(0, 0, 640, 128);
  background.addColorStop(0, 'rgba(9, 18, 12, 0.78)');
  background.addColorStop(1, 'rgba(17, 31, 21, 0.70)');
  context.fillStyle = background;
  context.roundRect(12, 14, 616, 100, 24);
  context.fill();
  context.strokeStyle = 'rgba(177, 204, 181, 0.38)';
  context.lineWidth = 2;
  context.stroke();
  context.fillStyle = '#edf5ed';
  context.font = '600 30px sans-serif';
  context.textAlign = 'center';
  context.textBaseline = 'middle';
  context.fillText(text, 320, 64, 574);

  const nextTexture = new THREE.CanvasTexture(canvas);
  nextTexture.colorSpace = THREE.SRGBColorSpace;
  nextTexture.minFilter = THREE.LinearFilter;

  const material = sprite.material;
  material.map?.dispose();
  material.map = nextTexture;
  material.needsUpdate = true;
  sprite.visible = true;
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
    object.castShadow = true;
    object.receiveShadow = false;
  });
}

export class ResidentWorldLayer {
  readonly group = new THREE.Group();

  private readonly loader = new GLTFLoader();
  private readonly socialLinkGeometry = new THREE.BufferGeometry();
  private readonly socialLinkMaterial = new THREE.LineBasicMaterial({
    vertexColors: true,
    transparent: true,
    opacity: 0.38,
    depthWrite: false,
  });
  private readonly socialLinks = new THREE.LineSegments(
    this.socialLinkGeometry,
    this.socialLinkMaterial,
  );
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
  private selectionPulseSeconds = 0;

  constructor() {
    this.selectionRing.rotation.x = -Math.PI * 0.5;
    this.selectionRing.visible = false;
    this.selectionRing.renderOrder = 4;
    this.socialLinks.renderOrder = 8;
    this.socialLinks.frustumCulled = false;
    this.group.add(this.socialLinks);
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
    const residentsById = new Map(
      residents.map((resident) => [resident.id, resident]),
    );
    const activeIds = new Set(residents.map((resident) => resident.id));

    for (const [id, actor] of this.actors) {
      const visible = activeIds.has(id);
      actor.root.visible = visible;
      if (!visible) {
        actor.statusSprite.visible = false;
      }
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
        const direction = next.clone().sub(actor.target);
        if (direction.lengthSq() > 0.0004) {
          actor.root.rotation.y = this.visualFacingYaw(
            direction.x,
            direction.z,
          );
        }
      }

      actor.activityLabel = resident.activityLabel ?? 'Idle';
      actor.activityTargetId = resident.activityTargetId ?? '';
      actor.presentation = resident.presentation ?? null;
      const nextStatusText = residentActionText(resident, residentsById);
      if (actor.statusText !== nextStatusText) {
        actor.statusText = nextStatusText;
        updateStatusSprite(actor.statusSprite, nextStatusText);
      } else {
        actor.statusSprite.visible = Boolean(nextStatusText);
      }
      actor.target.copy(next);

      if (!actor.initialized) {
        actor.current.copy(next);
        actor.root.position.copy(next);
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
    const effectiveSpeed = Math.max(
      SIMULATION_TIME_CONTRACT.defaultSpeed,
      this.simulationSpeed,
    );
    const maxDistance =
      RESIDENT_VISUAL_SPEED_WORLD_UNITS_PER_SECOND_AT_1X
      * effectiveSpeed
      * dt;

    for (const actor of this.actors.values()) {
      if (!actor.root.visible) continue;

      const delta = actor.target.clone().sub(actor.current);
      const distance = delta.length();
      const moving =
        distance > RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits;

      if (moving) {
        actor.root.rotation.y = this.visualFacingYaw(
          delta.x,
          delta.z,
        );
        if (distance <= maxDistance) {
          actor.current.copy(actor.target);
        } else if (maxDistance > 0) {
          actor.current.addScaledVector(
            delta.multiplyScalar(1 / distance),
            maxDistance,
          );
        }
      } else {
        this.faceInteractionTarget(actor);
      }

      if (
        this.simulationSpeed > 0
        && actor.presentation?.active
        && actor.presentation.phase === 'Moving'
      ) {
        actor.walkStateGraceSeconds =
          RESIDENT_PRESENTATION_CONTRACT.walkStateGraceSeconds;
      } else {
        actor.walkStateGraceSeconds = Math.max(
          0,
          actor.walkStateGraceSeconds - dt,
        );
      }

      actor.root.position.copy(actor.current);
      actor.groundShadow.visible = actor.root.visible;
      actor.statusSprite.position.set(
        actor.current.x,
        actor.current.y
          + 2.48
          + (
            stableHash(String(actor.root.userData.residentId ?? actor.statusText))
            % 3
          ) * 0.12,
        actor.current.z,
      );
      this.setAction(actor, moving);
      actor.mixer.update(dt);
    }

    this.updateSocialLinks();
    this.selectionPulseSeconds += dt;
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
      });
    }
    for (const actor of this.actors.values()) {
      const statusMaterial = actor.statusSprite.material;
      statusMaterial.map?.dispose();
      statusMaterial.dispose();
      this.group.remove(actor.statusSprite);
    }
    this.actors.clear();
    this.socialLinkGeometry.dispose();
    this.socialLinkMaterial.dispose();
    this.selectionRing.geometry.dispose();
    const selectionMaterial = this.selectionRing.material;
    if (!Array.isArray(selectionMaterial)) selectionMaterial.dispose();
  }

  private updateSocialLinks(): void {
    const positions: number[] = [];
    const colors: number[] = [];
    const seen = new Set<string>();

    const pushColor = (color: THREE.Color): void => {
      colors.push(
        color.r,
        color.g,
        color.b,
        color.r,
        color.g,
        color.b,
      );
    };

    for (const resident of this.pendingResidents) {
      const action = resident.contextAction;
      if (
        !action?.active
        || (
          action.kind !== 'Social'
          && action.kind !== 'KnowledgeTeaching'
          && action.kind !== 'Parenting'
        )
      ) {
        continue;
      }

      const targetId = action.targetResidentId ?? '';
      if (!targetId) continue;
      const sourceActor = this.actors.get(resident.id);
      const targetActor = this.actors.get(targetId);
      if (
        !sourceActor?.initialized
        || !targetActor?.initialized
        || !sourceActor.root.visible
        || !targetActor.root.visible
      ) {
        continue;
      }

      const pairKey = [resident.id, targetId].sort().join(':');
      if (seen.has(pairKey)) continue;
      seen.add(pairKey);

      positions.push(
        sourceActor.current.x,
        sourceActor.current.y + 1.0,
        sourceActor.current.z,
        targetActor.current.x,
        targetActor.current.y + 1.0,
        targetActor.current.z,
      );

      let color = new THREE.Color(0xa9c9af);
      if (action.kind === 'KnowledgeTeaching') {
        color = new THREE.Color(0xb9b878);
      } else if (action.kind === 'Parenting') {
        color = new THREE.Color(0xc9a894);
      } else {
        switch (action.socialIntent) {
          case 'Comfort':
            color = new THREE.Color(0x9bb9cf);
            break;
          case 'Repair':
            color = new THREE.Color(0xb8a276);
            break;
          case 'Avoid':
            color = new THREE.Color(0x8d8f92);
            break;
          default:
            color = new THREE.Color(0xa9c9af);
            break;
        }
      }
      pushColor(color);
    }

    this.socialLinkGeometry.setAttribute(
      'position',
      new THREE.Float32BufferAttribute(positions, 3),
    );
    this.socialLinkGeometry.setAttribute(
      'color',
      new THREE.Float32BufferAttribute(colors, 3),
    );
    this.socialLinkGeometry.computeBoundingSphere();
    this.socialLinks.visible = positions.length > 0;
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
    const pulse = 1 + Math.sin(this.selectionPulseSeconds * 4.2) * 0.07;
    this.selectionRing.scale.setScalar(pulse);
    const material = this.selectionRing.material;
    if (!Array.isArray(material)) {
      material.opacity = 0.56 + Math.sin(this.selectionPulseSeconds * 4.2) * 0.12;
    }
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

    const variantSeed = stableHash(resident.id);
    const root = new THREE.Group();
    root.userData.residentId = resident.id;
    const model = cloneSkeleton(this.template) as THREE.Group;
    cloneActorMaterials(model, variantSeed);
    root.add(model);

    const baseHeight = 1.68;
    const heightJitter = (((variantSeed >>> 8) % 13) - 6) * 0.018;
    const widthJitter = (((variantSeed >>> 16) % 9) - 4) * 0.012;
    const bodyWidth = (
      resident.sex === 'Female' ? 0.94 : 1
    ) + widthJitter;
    root.scale.set(
      bodyWidth,
      baseHeight + heightJitter,
      bodyWidth * (0.985 + ((variantSeed >>> 20) % 5) * 0.008),
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
    const motionTempo = 0.94 + ((variantSeed >>> 24) % 9) * 0.015;
    idle?.setEffectiveTimeScale(0.96 + (motionTempo - 1) * 0.4);
    walk?.setEffectiveTimeScale(motionTempo);
    talk?.setEffectiveTimeScale(0.95 + (motionTempo - 1) * 0.65);
    interact?.setEffectiveTimeScale(motionTempo);

    idle?.play();
    if (idle) {
      idle.time = ((variantSeed % 997) / 997)
        * Math.max(0.001, idle.getClip().duration);
    }

    const statusSprite = makeStatusSprite();
    this.group.add(statusSprite);

    const groundShadow = new THREE.Mesh(
      new THREE.CircleGeometry(0.48, 24),
      new THREE.MeshBasicMaterial({
        color: 0x000000,
        transparent: true,
        opacity: 0.2,
        depthWrite: false,
      }),
    );
    groundShadow.rotation.x = -Math.PI * 0.5;
    groundShadow.position.y = 0.015;
    groundShadow.renderOrder = 2;
    root.add(groundShadow);

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
      current: new THREE.Vector3(),
      target: new THREE.Vector3(),
      walkStateGraceSeconds: 0,
      statusSprite,
      statusText: '',
      groundShadow,
      initialized: false,
    };

    this.actors.set(resident.id, actor);
    this.group.add(root);
    return actor;
  }

  private faceInteractionTarget(actor: ResidentActor): void {
    const presentation = actor.presentation;
    if (
      !presentation?.active
      || presentation.phase !== 'Interacting'
    ) {
      return;
    }

    let targetX: number | null = null;
    let targetZ: number | null = null;
    const targetResidentId = presentation.targetResidentId ?? '';
    const targetActor = targetResidentId
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

    if (targetX === null || targetZ === null) return;

    const dx = targetX - actor.current.x;
    const dz = targetZ - actor.current.z;
    if (
      (dx * dx) + (dz * dz)
      <= RESIDENT_PRESENTATION_CONTRACT.movementEpsilonWorldUnits ** 2
    ) {
      return;
    }

    actor.root.rotation.y = this.visualFacingYaw(dx, dz);
  }

  private visualFacingYaw(dx: number, dz: number): number {
    // The Quaternius resident source faces +Z. Keep that source orientation
    // intact and align +Z directly with the authoritative travel vector.
    return Math.atan2(dx, dz);
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
    const targetActor = targetResidentId
      ? this.actors.get(targetResidentId)
      : undefined;
    const hasNearbyResidentTarget = Boolean(
      targetActor?.root.visible
      && targetActor.initialized
      && actor.current.distanceTo(targetActor.current) <= 3,
    );

    const supportsResidentConversation =
      presentation.kind === 'KnowledgeTeaching'
      || (
        presentation.kind === 'Social'
        && (
          presentation.socialIntent === 'Approach'
          || presentation.socialIntent === 'Repair'
          || presentation.socialIntent === 'Comfort'
        )
      );

    if (
      supportsResidentConversation
      && hasNearbyResidentTarget
      && actor.talk
    ) {
      return 'talk';
    }

    if (
      presentation.kind === 'Parenting'
      && hasNearbyResidentTarget
    ) {
      switch (presentation.parentingAction) {
        case 'Comfort':
        case 'Educate':
        case 'Discipline':
        case 'Play':
          return actor.talk ? 'talk' : 'idle';
        case 'Feed':
        case 'Hold':
        case 'Bathe':
        case 'HealthCare':
          return actor.interact ? 'interact' : 'idle';
        case 'PutToSleep':
        case 'ToiletAssist':
        default:
          // Lie-down / dependent sanitation need dedicated validated
          // presentation sequences. Never substitute a generic sitting pose.
          return 'idle';
      }
    }

    if (
      presentation.kind === 'Civilization'
      && presentation.hasTargetGrid
      && actor.interact
    ) {
      return 'interact';
    }

    if (presentation.kind === 'Physical') {
      switch (presentation.physicalGoal) {
        case 'Eat':
        case 'Drink':
          // Emergency variants still consume real carried provisions in Core,
          // so a generic self-interaction fallback is truthful.
          return actor.interact ? 'interact' : 'idle';
        case 'Wash':
          return (
            presentation.hasObjectTarget
            || presentation.emergencyFallback
          ) && actor.interact
            ? 'interact'
            : 'idle';
        case 'Sleep':
        case 'UseToilet':
        default:
          // Sleep needs a validated lie sequence; toilet needs the canonical
          // privacy/alignment sequence. Do not revive the old fake sit mapping.
          return 'idle';
      }
    }

    return 'idle';
  }

  private setAction(actor: ResidentActor, moving: boolean): void {
    const authoritativeWalking = Boolean(
      this.simulationSpeed > 0
      && (
        (
          actor.presentation?.active
          && actor.presentation.phase === 'Moving'
        )
        || actor.walkStateGraceSeconds > 0
      )
    );
    const desired: MotionName = (moving || authoritativeWalking) && actor.walk
      ? 'walk'
      : this.restMotion(actor);
    if (actor.active === desired) return;

    const previous = actor.active
      ? this.actionFor(actor, actor.active)
      : undefined;
    const next = this.actionFor(actor, desired) ?? actor.idle;
    if (!next) return;

    previous?.fadeOut(0.18);
    if (desired === 'walk') {
      // Keep the locomotion clip's phase across brief authoritative snapshot
      // gaps instead of restarting the gait cycle on every walk re-entry.
      next.enabled = true;
      next.fadeIn(0.18).play();
    } else {
      next.reset().fadeIn(0.18).play();
    }
    actor.active = desired;
  }
}
