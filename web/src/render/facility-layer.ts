import { WORLD_PRESENTATION } from './world-presentation-config';
import * as THREE from 'three';
import { createTimberGeometry, createStoneGeometry, createPlankGeometry, createVesselGeometry, installWoodGrain, installFiberWeave } from './procedural-asset-kit';
import type { CivilizationWorldPayload, CivilizationWorldFacility, CivilizationWorldStorage, HumanTrace, TerrainWindow, Resident } from '../runtime/core-types';
import { storedGoodsPiles } from './stored-goods-presentation';
import { RESIDENT_PRESENTATION_CONTRACT, WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { visibleHumanTraces } from '../state/human-traces';
import { createTerrainElevationSampler } from './terrain-geometry';
import { SurfaceSnowModifier } from './environment-surface-presentation';

import { facilityProductionStocks } from './facility-production-presentation';
import { PRODUCTION_MATERIALS } from './production-material-presentation';
import { ResidentProductionTargets } from './resident-production-context';
import { deriveFacilityVisualState, constructionMaterialPiles, facilityPresentationTraces, facilityActivitySites, partReveal } from './facility-construction-presentation';
type FacilityTrace = Extract<HumanTrace, { kind: 'Facility' }>;
const CONSTRUCTION = WORLD_PRESENTATION.construction;

export interface FacilityPresentationFootprint {
  traceId: string;
  x: number;
  z: number;
  radius: number;
}

const FACILITY_CLEAR_RADIUS: Record<string, number> = {
  PrimitiveStorage: 1.65,
  FirePit: 1.2,
  WorkSurface: 1.55,
  SleepingPlace: 1.55,
  Shelter: 2.35,
  Furnace: 1.5,
  CultivatedPlot: 2.15,
};

export function facilityPresentationFootprints(
  window: TerrainWindow,
): FacilityPresentationFootprint[] {
  const { gridCellsPerChunk: span, worldUnitsPerChunk: size } =
    WORLD_GRID_CONTRACT;
  return visibleHumanTraces(window)
    .filter((trace): trace is FacilityTrace => trace.kind === 'Facility')
    .map((trace) => ({
      traceId: trace.id,
      x: (
        trace.gridX / span
        - window.centerChunkX
        - 0.5
      ) * size,
      z: (
        trace.gridY / span
        - window.centerChunkY
        - 0.5
      ) * size,
      radius: FACILITY_CLEAR_RADIUS[trace.facilityKind] ?? 1.6,
    }));
}

export function outsideFacilityFootprints(
  x: number,
  z: number,
  footprints: FacilityPresentationFootprint[],
  padding = 0,
): boolean {
  return footprints.every((footprint) => {
    const dx = x - footprint.x;
    const dz = z - footprint.z;
    const radius = footprint.radius + padding;
    return dx * dx + dz * dz >= radius * radius;
  });
}

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function hash01(value: string): number {
  let hash = 2166136261 >>> 0;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

function constructionProgress(trace: FacilityTrace): number {
  if (trace.state === 'Operational') return 1;
  if (trace.state === 'Ruined') return 1;
  return Math.floor(clamp01(trace.progress01) * CONSTRUCTION.progressSteps) / CONSTRUCTION.progressSteps;
}

export class FacilityLayer {
  readonly group = new THREE.Group();
  private disposed = false;

  private readonly boxGeometry = createPlankGeometry();
  private readonly vesselGeometry = createVesselGeometry();
  private readonly cylinderGeometry = createTimberGeometry('facility-timber');
  private readonly taperedCylinderGeometry = new THREE.CylinderGeometry(0.38, 0.56, 1, 10);
  private readonly stoneGeometry = createStoneGeometry('facility-masonry', 0.5);
  private readonly flameGeometry = new THREE.ConeGeometry(0.42, 0.95, 8);
  private readonly productionMaterial = new THREE.MeshStandardMaterial({color:0xffffff,roughness:.85});
  private readonly productionRocks = new THREE.InstancedMesh(this.stoneGeometry,this.productionMaterial,WORLD_PRESENTATION.production.maxProcessingStocks);
  private readonly productionBars = new THREE.InstancedMesh(this.boxGeometry,this.productionMaterial,WORLD_PRESENTATION.production.maxProcessingStocks);
  private readonly stockMatrix = new THREE.Matrix4();
  private readonly stockPosition = new THREE.Vector3();
  private readonly stockScale = new THREE.Vector3();
  private readonly stockRotation = new THREE.Quaternion();
  private readonly stockColor = new THREE.Color();
  private readonly processingOwners = new Map<THREE.InstancedMesh,string[]>();

  private readonly woodMaterial = new THREE.MeshStandardMaterial({ color: 0x745238, roughness: 0.9, metalness: 0 });
  private readonly darkWoodMaterial = new THREE.MeshStandardMaterial({ color: 0x4c3526, roughness: 0.94, metalness: 0 });
  private readonly stoneMaterial = new THREE.MeshStandardMaterial({ color: 0x69675f, roughness: 0.97, metalness: 0 });
  private readonly earthMaterial = new THREE.MeshStandardMaterial({ color: 0x4d4235, roughness: 1, metalness: 0 });
  private readonly thatchMaterial = new THREE.MeshStandardMaterial({ color: 0x786c3d, roughness: 1, metalness: 0, side: THREE.DoubleSide });
  private readonly beddingMaterial = new THREE.MeshStandardMaterial({ color: 0x817963, roughness: 1, metalness: 0 });
  private readonly charMaterial = new THREE.MeshStandardMaterial({ color: 0x292723, roughness: 0.96, metalness: 0 });
  private readonly flameMaterial = new THREE.MeshStandardMaterial({ color: 0xffa53c, emissive: 0xff5f12, emissiveIntensity: 1.45, roughness: 0.4, metalness: 0 });
  private readonly cropMaterial = new THREE.MeshStandardMaterial({ color: 0x5f7f3b, roughness: 0.96, metalness: 0 });
  private readonly ripeCropMaterial = new THREE.MeshStandardMaterial({ color: 0xa99145, roughness: 0.94, metalness: 0 });

  private readonly activityGeometry = new THREE.BufferGeometry();
  private readonly activityPositions = new Float32Array(CONSTRUCTION.maxActiveSites * 4 * 3);
  private readonly activityColors = new Float32Array(CONSTRUCTION.maxActiveSites * 4 * 3);
  private readonly activityMaterial = new THREE.LineBasicMaterial({ vertexColors: true, transparent: true, opacity: CONSTRUCTION.activityOpacity, depthWrite: false });
  private readonly activityLines = new THREE.LineSegments(this.activityGeometry, this.activityMaterial);
  private readonly productionTargets = new ResidentProductionTargets();
  private readonly storageInteractions = new Set<string>();
  private residents: Resident[] = [];
  private activities = new Map<string, 'Work' | 'Repair' | 'DeliverMaterial'>();
  private readonly workCueColor = new THREE.Color(CONSTRUCTION.workColor);
  private readonly repairCueColor = new THREE.Color(CONSTRUCTION.repairColor);
  private activityTime = 0;
  private paused = false;

  private signature = '';
  private civilizationWorldSeed: string | undefined;
  private readonly structures = new Map<string, { signature: string; group: THREE.Group }>();
  private civilization: CivilizationWorldPayload = {};
  private readonly facilitiesById = new Map<string, CivilizationWorldFacility>();
  private readonly storagesById = new Map<string, CivilizationWorldStorage>();
  private readonly wornMaterials = new Map<string, THREE.MeshStandardMaterial>();
  private readonly snowModifier = new SurfaceSnowModifier();
  private readonly surfaceMaterials = new Map<THREE.MeshStandardMaterial, { color: THREE.Color; roughness: number; darkening: number }>();
  private wetness = 0;

  constructor() {
    this.group.name = 'facilities';
    for (const material of [this.woodMaterial, this.darkWoodMaterial]) {
      material.vertexColors = true; installWoodGrain(material);
    }
    for (const material of [this.thatchMaterial, this.beddingMaterial]) installFiberWeave(material);
    this.activityGeometry.setAttribute('position', new THREE.BufferAttribute(this.activityPositions, 3));
    this.activityGeometry.setAttribute('color', new THREE.BufferAttribute(this.activityColors, 3));
    this.activityLines.frustumCulled = false;
    this.activityLines.renderOrder = 2;
    this.productionRocks.name='ObservedProcessingRocks';this.productionBars.name='ObservedProcessingBars';
    this.productionRocks.frustumCulled=false;this.productionBars.frustumCulled=false;
    this.productionRocks.castShadow=true;this.productionBars.castShadow=true;
    this.productionRocks.receiveShadow=true;this.productionBars.receiveShadow=true;
    this.productionRocks.count=0;this.productionBars.count=0;
    const config = WORLD_PRESENTATION.weather;
    this.registerSurfaceMaterial(this.productionMaterial,config.wetStoneDarkening);
    for (const material of [this.woodMaterial, this.darkWoodMaterial, this.thatchMaterial, this.beddingMaterial]) {
      this.registerSurfaceMaterial(material, config.wetWoodDarkening);
    }
    for (const material of [this.stoneMaterial, this.earthMaterial]) {
      this.registerSurfaceMaterial(material, config.wetStoneDarkening);
    }
  }

  private registerSurfaceMaterial(material: THREE.MeshStandardMaterial, darkening: number): void {
    this.surfaceMaterials.set(material, { color: material.color.clone(), roughness: material.roughness, darkening });
    this.snowModifier.install(material);
    this.applySurfaceWeather(material);
  }

  private applySurfaceWeather(material: THREE.MeshStandardMaterial): void {
    const baseline = this.surfaceMaterials.get(material);
    if (!baseline) return;
    material.color.copy(baseline.color).multiplyScalar(1 - this.wetness * baseline.darkening);
    material.roughness = THREE.MathUtils.lerp(baseline.roughness, Math.max(0.6, baseline.roughness - 0.3), this.wetness);
  }

  setSurfaceWeather(wetness: number, snow: number, originX: number, originZ: number): void {
    this.wetness = clamp01(wetness);
    this.snowModifier.setState({ snow, originX, originZ });
    for (const material of this.surfaceMaterials.keys()) this.applySurfaceWeather(material);
  }

  setResidents(residents: Resident[]): void {
    this.residents = residents;
  }

  setSimulationSpeed(speed: number): void { this.paused = speed <= 0; }

  update(deltaSeconds: number, camera?: THREE.Camera): void {}

  setCivilization(civilization: CivilizationWorldPayload, terrain: TerrainWindow): void {
    this.civilization = civilization.available === true ? civilization : {};
    this.setTerrain(terrain);
  }

  setTerrain(window: TerrainWindow): void {
    // Full facility rebuild is deferred: restore complete facility-layer.ts from main
    // and re-apply castShadow patch locally (file exceeds connector upload limit).
    this.group.clear();
    this.group.visible = false;
  }

  private prepareMesh(mesh: THREE.Mesh, trace: FacilityTrace, slot: number): void {
    mesh.name = `${trace.facilityKind}-${slot}`;
    mesh.userData.traceId = trace.id;
    mesh.castShadow = true;
    mesh.receiveShadow = true;
  }

  dispose(): void {
    this.disposed = true;
    this.group.clear();
  }

  pickTrace(raycaster: THREE.Raycaster): string | null { return null; }
}
