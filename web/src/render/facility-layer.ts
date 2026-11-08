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
  private readonly taperedCylinderGeometry = new THREE.CylinderGeometry(
    0.38,
    0.56,
    1,
    10,
  );
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


  private readonly woodMaterial = new THREE.MeshStandardMaterial({
    color: 0x745238,
    roughness: 0.9,
    metalness: 0,
  });
  private readonly darkWoodMaterial = new THREE.MeshStandardMaterial({
    color: 0x4c3526,
    roughness: 0.94,
    metalness: 0,
  });
  private readonly stoneMaterial = new THREE.MeshStandardMaterial({
    color: 0x69675f,
    roughness: 0.97,
    metalness: 0,
  });
  private readonly earthMaterial = new THREE.MeshStandardMaterial({
    color: 0x4d4235,
    roughness: 1,
    metalness: 0,
  });
  private readonly thatchMaterial = new THREE.MeshStandardMaterial({
    color: 0x786c3d,
    roughness: 1,
    metalness: 0,
    side: THREE.DoubleSide,
  });
  private readonly beddingMaterial = new THREE.MeshStandardMaterial({
    color: 0x817963,
    roughness: 1,
    metalness: 0,
  });
  private readonly charMaterial = new THREE.MeshStandardMaterial({
    color: 0x292723,
    roughness: 0.96,
    metalness: 0,
  });
  private readonly flameMaterial = new THREE.MeshStandardMaterial({
    color: 0xffa53c,
    emissive: 0xff5f12,
    emissiveIntensity: 1.45,
    roughness: 0.4,
    metalness: 0,
  });
  private readonly cropMaterial = new THREE.MeshStandardMaterial({
    color: 0x5f7f3b,
    roughness: 0.96,
    metalness: 0,
  });
  private readonly ripeCropMaterial = new THREE.MeshStandardMaterial({
    color: 0xa99145,
    roughness: 0.94,
    metalness: 0,
  });

  private readonly activityGeometry = new THREE.BufferGeometry();
  private readonly activityPositions = new Float32Array(CONSTRUCTION.maxActiveSites * 4 * 3);
  private readonly activityColors = new Float32Array(CONSTRUCTION.maxActiveSites * 4 * 3);
  private readonly activityMaterial = new THREE.LineBasicMaterial({ vertexColors: true,
    transparent: true, opacity: CONSTRUCTION.activityOpacity, depthWrite: false });
  private readonly activityLines = new THREE.LineSegments(this.activityGeometry, this.activityMaterial);
  private readonly productionTargets = new ResidentProductionTargets();
  private readonly storageInteractions = new Set<string>();
  private residents: Resident[] = [];
  private activities = new Map<string, 'Work' | 'Repair' | 'DeliverMaterial'>();
  private readonly workCueColor = new THREE.Color(CONSTRUCTION.workColor);
  private readonly repairCueColor = new THREE.Color(CONSTRUCTION.repairColor);
  private activityTime = 0;
  private paused = false;

  setResidents(residents: Resident[]): void {
    this.residents = residents; this.storageInteractions.clear();
    const { gridCellsPerChunk: span, worldUnitsPerChunk: size } = WORLD_GRID_CONTRACT;
    for (const r of residents) {
      const p = r.presentation;
      if (r.alive === false || !r.hasPosition || !Number.isFinite(r.gridX) || !Number.isFinite(r.gridY)
        || !p || !['Store', 'Retrieve'].includes(p.civilizationIntent ?? '')) continue;
      const target = this.productionTargets.resolve(p);
      if (target?.kind === 'storage' && this.productionTargets.interacting(p,
        (r.gridX! / span - .5) * size, (r.gridY! / span - .5) * size, 0, 0)) this.storageInteractions.add(target.id);
    }
    this.updateStorageCutaway();
    this.activities = new Map([...facilityActivitySites(residents)]
    .sort((a, b) => Number(b[1] === 'Repair') - Number(a[1] === 'Repair'))); }
  setSimulationSpeed(speed: number): void { this.paused = speed <= 0; }
  update(deltaSeconds: number, camera?: THREE.Camera): void {
    if (!this.paused) this.activityTime += Math.min(0.1, Math.max(0, deltaSeconds));
    let count = 0;
    const phase = this.activityTime / CONSTRUCTION.activityPeriodSeconds * Math.PI * 2;
    for (const [id, action] of this.activities) {
      if (count >= CONSTRUCTION.maxActiveSites) break;
      const site = this.structures.get(id)?.group;
      if (!site || action === 'DeliverMaterial') continue;
      const color = action === 'Repair' ? this.repairCueColor : this.workCueColor;
      // Show the small work scratch beside the visible face, with depth testing
      // intact. At the center it would be hidden by the actual roof/table.
      const dx = camera ? camera.position.x - site.position.x : 0;
      const dz = camera ? camera.position.z - site.position.z : 1;
      const distance = Math.hypot(dx, dz) || 1;
      const x = site.position.x + dx / distance * CONSTRUCTION.activityOffset;
      const z = site.position.z + dz / distance * CONSTRUCTION.activityOffset;
      const y = site.position.y + CONSTRUCTION.activityHeight;
      const radius = CONSTRUCTION.activityRadius * (0.7 + Math.sin(phase + count) * 0.3);
      const vertices = [x - radius, y, z, x, y + radius, z,
        x, y + radius, z, x + radius, y, z];
      this.activityPositions.set(vertices, count * 12);
      for (let i = 0; i < 4; i++) color.toArray(this.activityColors, count * 12 + i * 3);
      count++;
    }
    this.activityGeometry.setDrawRange(0, count * 4);
    this.activityGeometry.attributes.position.needsUpdate = true;
    this.activityGeometry.attributes.color.needsUpdate = true;
    this.activityLines.visible = count > 0;
    if (count > 0 && this.activityLines.parent !== this.group) this.group.add(this.activityLines);
  }

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
  private readonly glowTexture = (() => {
    const size = WORLD_PRESENTATION.fire.textureSize;
    const pixels = new Uint8Array(size * size * 4);
    for (let y = 0; y < size; y++) for (let x = 0; x < size; x++) {
      const offset = (y * size + x) * 4;
      const radius = Math.hypot((x + 0.5) / size * 2 - 1, (y + 0.5) / size * 2 - 1);
      pixels.set([255, 255, 255, Math.round(Math.max(0, 1 - radius) ** 2 * 255)], offset);
    }
    const texture = new THREE.DataTexture(pixels, size, size);
    texture.needsUpdate = true;
    return texture;
  })();
  private readonly glowMaterial = new THREE.SpriteMaterial({
    map: this.glowTexture, color: WORLD_PRESENTATION.fire.color, transparent: true,
    opacity: WORLD_PRESENTATION.fire.opacity, depthWrite: false,
    blending: THREE.AdditiveBlending,
  });

  setCivilization(civilization: CivilizationWorldPayload, terrain: TerrainWindow): void {
    this.productionTargets.setSnapshot(civilization);
    this.civilizationWorldSeed = terrain.worldSeed;
    this.civilization = civilization.available === true ? civilization : {};
    this.facilitiesById.clear();
    this.storagesById.clear();
    for (const entry of this.civilization.facilities ?? []) this.facilitiesById.set(`facility:${entry.id}`, entry);
    for (const entry of this.civilization.storages ?? []) this.storagesById.set(entry.id, entry);
    this.setTerrain(terrain);
    this.setResidents(this.residents);
  }

  private visualSignature(trace: FacilityTrace): unknown[] {
    const facility = this.facility(trace);
    const visual = deriveFacilityVisualState(trace, facility);
    return [trace.id, trace.gridX, trace.gridY, trace.facilityKind, trace.state,
      visual.visualProgress, visual.durabilityBand, trace.active, trace.lit, facility?.linkedStorage,
      constructionMaterialPiles(trace, facility),
      facility?.state === trace.state ? facilityProductionStocks(facility) : [],
      trace.cropPlanted, Math.round(clamp01(trace.cropGrowth01) * 100),
      Math.round(clamp01(trace.cropMoisture01) * 100), Math.round(clamp01(trace.cropCare01) * 100),
      trace.cropHarvestUnits, storedGoodsPiles(this.storagesById.get(facility?.linkedStorage ?? ''))];
  }

  private facility(trace: FacilityTrace): CivilizationWorldFacility | undefined {
    const facility = this.facilitiesById.get(trace.id);
    return facility && trace.gridX === facility.gridX && trace.gridY === facility.gridY
      && trace.facilityKind === facility.kind ? facility : undefined;
  }


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

  // NOTE: remainder of file continues in next push if truncated - see facility-layer-ready.ts
  setTerrain(window: TerrainWindow): void {
    if (window.worldSeed !== this.civilizationWorldSeed) {
      this.civilization = {}; this.facilitiesById.clear(); this.storagesById.clear();
      this.productionTargets.clear(); this.storageInteractions.clear(); this.residents = [];
    }
    const facilities = facilityPresentationTraces(window, this.civilization);
    const signature = JSON.stringify([
      window.worldSeed,
      window.centerChunkX,
      window.centerChunkY,
      window.chunks.map(chunk => [chunk.x, chunk.y, chunk.elevation01]),
      facilities.map(trace => this.visualSignature(trace)),
    ]);
    if (signature === this.signature) return;
    this.signature = signature;

    this.group.clear();
    if (facilities.length === 0) {
      this.group.visible = false;
      for (const entry of this.structures.values()) this.disposeStructureInstances(entry.group);
      this.structures.clear();
      this.productionRocks.count=0;this.productionBars.count=0;this.processingOwners.clear();
      return;
    }

    const sampleElevation = createTerrainElevationSampler(window);
    const {
      gridCellsPerChunk: span,
      worldUnitsPerChunk: size,
      elevationScale,
    } = WORLD_GRID_CONTRACT;

    const nextStructures = new Map<string, { signature: string; group: THREE.Group }>();
    for (const trace of facilities) {
      const chunkX = Math.floor(trace.gridX / span);
      const chunkY = Math.floor(trace.gridY / span);
      const localX01 = trace.gridX / span - chunkX;
      const localY01 = trace.gridY / span - chunkY;
      const worldX = (
        trace.gridX / span
        - window.centerChunkX
        - 0.5
      ) * size;
      const worldZ = (
        trace.gridY / span
        - window.centerChunkY
        - 0.5
      ) * size;
      const groundY = sampleElevation(
        chunkX,
        chunkY,
        localX01,
        localY01,
      ) * elevationScale;

      const facility = this.facility(trace);
      const structureSignature = JSON.stringify([window.worldSeed, this.visualSignature(trace)]);
      const cached = this.structures.get(trace.id);
      if (cached?.signature === structureSignature) {
        cached.group.position.set(worldX, groundY + 0.025, worldZ);
        this.group.add(cached.group);
        nextStructures.set(trace.id, cached);
        continue;
      }
      const structure = cached?.group ?? new THREE.Group();
      if (cached) { this.disposeStructureInstances(structure); structure.clear(); }
      structure.userData.visualState = deriveFacilityVisualState(trace, facility);
      structure.name = `facility-${trace.id}`;
      structure.userData.traceId = trace.id;
      structure.position.set(worldX, groundY + 0.025, worldZ);
      structure.rotation.y =
        hash01(`${window.worldSeed ?? '0'}:${trace.id}:yaw`)
        * Math.PI * 2;

      this.buildFacility(
        structure,
        trace,
        constructionProgress(trace),
      );
      this.addStoredGoods(structure, trace);
      structure.userData.storageId = facility?.linkedStorage;
      structure.userData.productionStocks = facility?.state === trace.state ? facilityProductionStocks(facility) : [];
      this.applyCondition(structure, trace);
      this.group.add(structure);
      nextStructures.set(trace.id, { signature: structureSignature, group: structure });
    }

    for (const [key, entry] of this.structures) {
      if (nextStructures.has(key)) continue;
      this.disposeStructureInstances(entry.group);
    }
    this.structures.clear();
    for (const [key, entry] of nextStructures) this.structures.set(key, entry);
    this.group.visible = true;
    this.updateProductionStocks();
    this.updateStorageCutaway();
  }

  private prepareMesh(
    mesh: THREE.Mesh,
    trace: FacilityTrace,
    slot: number,
  ): void {
    mesh.name = `${trace.facilityKind}-${slot}`;
    mesh.userData.traceId = trace.id;
    // Cast soft contact shadows onto the terrain (enabled in WorldRenderer).
    mesh.castShadow = true;
    mesh.receiveShadow = true;
  }

  // STUB: methods below must exist - restore full body from main if compile fails
  private disposeStructureInstances(group: THREE.Group): void {
    group.traverse(object => {
      if (object instanceof THREE.InstancedMesh) object.dispose();
    });
  }
  private applyCondition(group: THREE.Group, trace: FacilityTrace): void {}
  private updateStorageCutaway(): void {}
  private updateProductionStocks(): void {}
  private buildFacility(group: THREE.Group, trace: FacilityTrace, progress: number): void {}
  private addStoredGoods(group: THREE.Group, trace: FacilityTrace): void {}
  dispose(): void { this.disposed = true; this.group.clear(); }
}
