import { WORLD_PRESENTATION } from './world-presentation-config';
import * as THREE from 'three';
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

  private readonly boxGeometry = new THREE.BoxGeometry(1, 1, 1);
  private readonly cylinderGeometry = new THREE.CylinderGeometry(
    0.5,
    0.5,
    1,
    8,
  );
  private readonly taperedCylinderGeometry = new THREE.CylinderGeometry(
    0.38,
    0.56,
    1,
    10,
  );
  private readonly stoneGeometry = new THREE.DodecahedronGeometry(0.5, 0);
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
    this.activityGeometry.setAttribute('position', new THREE.BufferAttribute(this.activityPositions, 3));
    this.activityGeometry.setAttribute('color', new THREE.BufferAttribute(this.activityColors, 3));
    this.activityLines.frustumCulled = false;
    this.activityLines.renderOrder = 2;
    this.productionRocks.name='ObservedProcessingRocks';this.productionBars.name='ObservedProcessingBars';
    this.productionRocks.frustumCulled=false;this.productionBars.frustumCulled=false;
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
      structure.userData.storageId = trace.facilityKind === 'PrimitiveStorage' && trace.state === 'Operational'
        && trace.active && facility?.state === trace.state ? facility.linkedStorage : undefined;
      structure.userData.productionStocks = facility?.state === trace.state ? facilityProductionStocks(facility) : [];
      this.applyCondition(structure, trace);
      this.group.add(structure);
      nextStructures.set(trace.id, { signature: structureSignature, group: structure });
    }

    for (const [id, old] of this.structures) {
      if (nextStructures.get(id)?.group !== old.group) this.disposeStructureInstances(old.group);
    }
    this.structures.clear();
    for (const [id, entry] of nextStructures) this.structures.set(id, entry);
    this.updateProductionStocks();
    this.updateStorageCutaway();
    this.group.visible = true;
  }

  pickTrace(raycaster: THREE.Raycaster): string | null {
    if (!this.group.visible) return null;
    const hits = raycaster.intersectObjects(this.group.children, true);
    for (const hit of hits) {
      const owners=this.processingOwners.get(hit.object as THREE.InstancedMesh);
      if(owners && hit.instanceId !== undefined && owners[hit.instanceId])return owners[hit.instanceId];
      let current: THREE.Object3D | null = hit.object;
      while (current && current !== this.group) {
        if (typeof current.userData.traceId === 'string') {
          return current.userData.traceId;
        }
        current = current.parent;
      }
    }
    return null;
  }

  dispose(): void {
    if(this.disposed)return;this.disposed=true;
    this.productionRocks.dispose();this.productionBars.dispose();this.productionMaterial.dispose();
    this.activities.clear();this.residents=[];this.storageInteractions.clear();this.productionTargets.clear();
    this.processingOwners.clear();
    this.activityGeometry.dispose();
    this.activityMaterial.dispose();
    this.group.clear();
    for (const entry of this.structures.values()) this.disposeStructureInstances(entry.group);
    for (const material of this.wornMaterials.values()) material.dispose();
    this.wornMaterials.clear();
    this.surfaceMaterials.clear();
    this.structures.clear();
    this.facilitiesById.clear(); this.storagesById.clear();
    this.glowMaterial.dispose();
    this.glowTexture.dispose();
    this.boxGeometry.dispose();
    this.cylinderGeometry.dispose();
    this.taperedCylinderGeometry.dispose();
    this.stoneGeometry.dispose();
    this.flameGeometry.dispose();
    this.woodMaterial.dispose();
    this.darkWoodMaterial.dispose();
    this.stoneMaterial.dispose();
    this.earthMaterial.dispose();
    this.thatchMaterial.dispose();
    this.beddingMaterial.dispose();
    this.charMaterial.dispose();
    this.flameMaterial.dispose();
    this.cropMaterial.dispose();
    this.ripeCropMaterial.dispose();
  }

  private addStoredGoods(group: THREE.Group, trace: FacilityTrace): void {
    const facility = this.facility(trace);
    if (trace.facilityKind !== 'PrimitiveStorage' || trace.state !== 'Operational'
      || !facility?.linkedStorage) return;
    const storage = this.storagesById.get(facility.linkedStorage);
    const piles = storedGoodsPiles(storage);
    const config = WORLD_PRESENTATION.storage;
    const materials: Record<string, THREE.Material> = {
      Wood: this.woodMaterial, Stone: this.stoneMaterial, Fiber: this.thatchMaterial,
      Clay: this.earthMaterial, PlantFood: this.cropMaterial, Charcoal: this.charMaterial,
      CopperOre: this.stoneMaterial, TinOre: this.stoneMaterial, IronOre: this.stoneMaterial,
    };
    for (const [i, pile] of piles.entries()) {
      // Inside the existing store footprint: no extra buildings or free stock.
      const height = config.height * pile.fill;
      this.addBox(group, trace, materials[pile.material ?? ''] ?? this.beddingMaterial,
        [(i - (piles.length - 1) / 2) * config.width, 0.95 + height / 2, 0],
        [config.width * 0.85, height, config.depth], [0, 0, 0], 950 + i);
      group.children[group.children.length - 1].userData.storedGoods = pile;
    }
  }

  private updateStorageCutaway(): void {
    for (const { group } of this.structures.values()) {
      const cutaway = this.storageInteractions.has(group.userData.storageId);
      for (const part of group.children) if (part.userData.storageCutaway) part.visible = !cutaway;
    }
  }

  private updateProductionStocks(): void {
    this.productionRocks.count=0;this.productionBars.count=0;
    this.processingOwners.set(this.productionRocks,[]);this.processingOwners.set(this.productionBars,[]);
    let count=0;const C=WORLD_PRESENTATION.production;
    for(const {group} of this.structures.values()) {
      const stocks=group.userData.productionStocks ?? [];group.updateMatrix();
      for(const [i,stock] of stocks.entries()) {
        if(count>=C.maxProcessingStocks)break;
        const profile=PRODUCTION_MATERIALS[stock.material as keyof typeof PRODUCTION_MATERIALS];
        if(stock.material !== 'Fuel' && !profile)continue;
        const scale=C.processingPileSize*Math.cbrt(stock.fill);
        this.stockPosition.set((i-(stocks.length-1)/2)*C.processingPileSpacing,scale/2,-1.1);
        this.stockScale.setScalar(scale);this.stockMatrix.compose(this.stockPosition,this.stockRotation,this.stockScale).premultiply(group.matrix);
        const batch=stock.material.endsWith('Metal') || stock.material === 'Bronze' ? this.productionBars : this.productionRocks;
        batch.setMatrixAt(batch.count,this.stockMatrix);this.stockColor.setHex(profile?.color ?? PRODUCTION_MATERIALS.Wood.color);batch.setColorAt(batch.count,this.stockColor);
        this.processingOwners.get(batch)!.push(group.userData.traceId);batch.count++;count++;
      }
    }
    for(const batch of [this.productionRocks,this.productionBars]) {
      batch.visible=batch.count>0;batch.instanceMatrix.needsUpdate=true;if(batch.instanceColor)batch.instanceColor.needsUpdate=true;
      if(batch.count)this.group.add(batch);
    }
  }

  private applyCondition(group: THREE.Group, trace: FacilityTrace): void {
    const band = deriveFacilityVisualState(trace, this.facility(trace)).durabilityBand;
    const durability = band === 0 ? 1 : this.facility(trace)?.durability;
    // Missing detail and mere inactivity are not evidence of decay.
    if (trace.state !== 'Operational' || !Number.isFinite(durability)) return;
    const wear = CONSTRUCTION.wearAmounts[band];
    if (wear === 0) return;
    group.traverse(object => {
      if (!(object instanceof THREE.Mesh) || !(object.material instanceof THREE.MeshStandardMaterial)
        || object.material === this.flameMaterial || object.material === this.cropMaterial
        || object.material === this.ripeCropMaterial || object.userData.storedGoods || object.userData.productionStock) return;
      const original = object.material;
      const key = `${original.uuid}:${wear}`;
      let material = this.wornMaterials.get(key);
      if (!material) {
        material = original.clone();
        const baseline = this.surfaceMaterials.get(original);
        // Build condition from dry base, then apply weather. Avoid double darkening
        // when a durability refresh occurs while the base material is already wet.
        if (baseline) { material.color.copy(baseline.color); material.roughness = baseline.roughness; }
        material.color.lerp(new THREE.Color(WORLD_PRESENTATION.weatheredColor), wear * WORLD_PRESENTATION.wearTint);
        if (baseline) this.registerSurfaceMaterial(material, baseline.darkening);
        this.wornMaterials.set(key, material);
      }
      object.material = material;
      // Bounded weathering inside the current footprint; never move the facility.
      if (trace.facilityKind === 'SleepingPlace') return;
      if (band >= 2 && object.position.y > 0.7) {
        object.scale.y *= 1 - wear * CONSTRUCTION.wearShapeLoss;
        if (object.scale.x > 1 && object.scale.z > 1) object.scale.z *= 1 - wear * CONSTRUCTION.wearShapeLoss;
        object.position.y *= 1 - wear * 0.08;
      }
      object.rotation.z += wear * wear * CONSTRUCTION.wearTilt
        * (hash01(object.name) - 0.5);
    });
  }

  private addFireGlow(group: THREE.Group, y: number, z = 0): void {
    // Vertex-free soft silhouette, no real-time lights or shadow maps.
    const glow = new THREE.Sprite(this.glowMaterial);
    glow.position.set(0, y, z);
    glow.scale.setScalar(WORLD_PRESENTATION.fire.glowSize);
    group.add(glow);
  }

  private buildFacility(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    if (trace.state === 'Ruined') {
      this.buildRuinedFacility(group, trace);
      return;
    }

    if (trace.state !== 'Operational') {
      if (progress < CONSTRUCTION.earlyWorkEnd) this.addPlanStakes(group, trace);
      this.addMaterialPile(group, trace);
      if (progress === 0) return;
    }

    switch (trace.facilityKind) {
      case 'PrimitiveStorage':
        this.buildStorage(group, trace, progress);
        break;
      case 'FirePit':
        this.buildFirePit(group, trace, progress);
        break;
      case 'WorkSurface':
        this.buildWorkSurface(group, trace, progress);
        break;
      case 'SleepingPlace':
        this.buildSleepingPlace(group, trace, progress);
        break;
      case 'Shelter':
        this.buildShelter(group, trace, progress);
        break;
      case 'Furnace':
        this.buildFurnace(group, trace, progress);
        break;
      case 'CultivatedPlot':
        this.buildCultivatedPlot(group, trace, progress);
        break;
      default:
        this.buildUnknownFacility(group, trace, progress);
        break;
    }
  }

  private buildRuinedFacility(
    group: THREE.Group,
    trace: FacilityTrace,
  ): void {
    const stoneKind = ['FirePit', 'Furnace', 'CultivatedPlot'].includes(trace.facilityKind);
    const material = stoneKind ? this.stoneMaterial : this.darkWoodMaterial;
    const width = trace.facilityKind === 'Shelter' ? 2.7 : 1.7;
    this.addBox(group, trace, material, [-0.35, 0.14, 0], [width, 0.22, 0.24],
      [0.08, 0.3, 0.12], 900);
    this.addBox(group, trace, material, [0.35, 0.12, 0.35], [1.1, 0.18, 0.3],
      [0.12, -0.6, -0.18], 901);
    if (trace.facilityKind === 'Shelter') {
      this.addBox(group, trace, this.thatchMaterial, [0, 0.35, 0], [2.1, 0.16, 1.6],
        [0.12, 0.18, -0.25], 904);
      this.addCylinder(group, trace, this.woodMaterial, [-1.1, 0.65, -0.8],
        [0.12, 1.2, 0.12], [0, 0, 0.35], 905);
    } else if (trace.facilityKind === 'SleepingPlace') {
      this.addBox(group, trace, this.beddingMaterial, [0, 0.1, 0], [1.7, 0.08, 0.6],
        [0.08, -0.15, 0.12], 903);
    } else if (trace.facilityKind === 'WorkSurface' || trace.facilityKind === 'PrimitiveStorage') {
      this.addBox(group, trace, this.woodMaterial, [0, 0.35, 0], [1.8, 0.16, 1.2],
        [0.3, 0.1, -0.25], 906);
    } else if (trace.facilityKind === 'Furnace') {
      this.addCylinder(group, trace, this.earthMaterial, [0, 0.4, 0], [0.7, 0.8, 0.7],
        [0, 0, 0.25], 907);
    } else if (trace.facilityKind === 'FirePit') {
      for (let i = 0; i < 6; i++) this.addBox(group, trace, this.stoneMaterial,
        [Math.cos(i) * 0.75, 0.08, Math.sin(i) * 0.75], [0.3, 0.16, 0.24], [0, i, 0], 910 + i);
    } else if (trace.facilityKind === 'CultivatedPlot') {
      this.addBox(group, trace, this.earthMaterial, [0, 0.025, 0], [2.3, 0.05, 1.9], [0, 0, 0], 920);
    }
  }

  private addPlanStakes(
    group: THREE.Group,
    trace: FacilityTrace,
  ): void {
    const corners = [
      [-1.35, -1.1],
      [1.35, -1.1],
      [1.35, 1.1],
      [-1.35, 1.1],
    ] as const;
    corners.forEach(([x, z], index) => {
      this.addCylinder(
        group,
        trace,
        this.woodMaterial,
        [x, CONSTRUCTION.stakeHeight / 2, z],
        [0.08, CONSTRUCTION.stakeHeight, 0.08],
        [0, 0, 0],
        index,
      );
    });
  }

  private addMaterialPile(
    group: THREE.Group,
    trace: FacilityTrace,
  ): void {
    const materials: Record<string, THREE.Material> = {
      Wood: this.woodMaterial, Stone: this.stoneMaterial, Fiber: this.thatchMaterial,
      Clay: this.earthMaterial,
    };
    // Uses only trace.deliveredMaterialUnits / trace.requiredMaterialUnits;
    // no material is inferred from resident arrival or construction time.
    const piles = constructionMaterialPiles(trace, this.facility(trace));
    for (const [index, pile] of piles.entries()) {
      const size = CONSTRUCTION.pileSize;
      const height = size * pile.fill;
      this.addBox(group, trace, materials[pile.material] ?? this.earthMaterial,
        [-1.3 + index * CONSTRUCTION.pileSpacing, height / 2, 1.25],
        [size * 0.85, height, size * 0.65], [0, 0.12 * index, 0], 90 + index);
      group.children[group.children.length - 1].userData.constructionMaterial = pile;
    }
  }

  private buildStorage(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.06,
      this.darkWoodMaterial,
      [0, 0.12, 0],
      [2.4, 0.24, 1.65],
      [0, 0, 0],
      10,
    );

    const posts = [
      [-1.0, -0.62],
      [1.0, -0.62],
      [-1.0, 0.62],
      [1.0, 0.62],
    ] as const;
    posts.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.28 + index * 0.035,
        this.woodMaterial,
        [x, 0.62, z],
        [0.11, 1.24, 0.11],
        [0, 0, 0],
        20 + index,
      );
    });

    const cutawayStart = group.children.length;
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.48,
      this.woodMaterial,
      [0, 0.55, 0],
      [1.85, 0.72, 1.14],
      [0, 0, 0],
      30,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.72,
      this.thatchMaterial,
      [0, 1.23, 0],
      [2.55, 0.18, 1.9],
      [0, 0, -0.05],
      31,
    );
    for (const part of group.children.slice(cutawayStart)) part.userData.storageCutaway = true;
  }

  private buildFirePit(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const stoneCount = 10;
    for (let index = 0; index < stoneCount; index += 1) {
      const angle = index / stoneCount * Math.PI * 2;
      this.addStoneAtProgress(
        group,
        trace,
        progress,
        0.04 + index * 0.025,
        [
          Math.cos(angle) * 0.82,
          0.16,
          Math.sin(angle) * 0.82,
        ],
        [0.42, 0.28, 0.36],
        [0, angle, 0],
        40 + index,
      );
    }

    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.38,
      this.charMaterial,
      [0, 0.22, 0],
      [0.13, 1.3, 0.13],
      [0, 0, Math.PI * 0.5],
      52,
    );
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.46,
      this.charMaterial,
      [0, 0.24, 0],
      [0.13, 1.3, 0.13],
      [Math.PI * 0.5, 0, Math.PI * 0.5],
      53,
    );

    if (trace.lit && progress >= 0.78 && trace.state !== 'Ruined') {
      const flame = new THREE.Mesh(
        this.flameGeometry,
        this.flameMaterial,
      );
      flame.position.set(0, 0.74, 0);
      flame.scale.set(0.65, 1.0, 0.65);
      this.prepareMesh(flame, trace, 54);
      group.add(flame);
      this.addFireGlow(group, flame.position.y, flame.position.z);
    }
  }

  private buildWorkSurface(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const firstPart = group.children.length;
    const legs = [
      [-0.84, -0.42],
      [0.84, -0.42],
      [-0.84, 0.42],
      [0.84, 0.42],
    ] as const;
    legs.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.06 + index * 0.04,
        this.woodMaterial,
        [x, 0.48, z],
        [0.09, 0.96, 0.09],
        [0, 0, 0],
        60 + index,
      );
    });
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.52,
      this.darkWoodMaterial,
      [0, 1.0, 0],
      [2.1, 0.2, 1.15],
      [0, 0, 0],
      65,
    );
    this.addStoneAtProgress(
      group,
      trace,
      progress,
      0.72,
      [0.48, 1.2, 0.08],
      [0.34, 0.18, 0.25],
      [0, 0.4, 0],
      66,
    );
    // Existing composition's working hands need a low primitive workbench.
    // Keep the same footprint/build order; this is geometry, not work authority.
    for (const part of group.children.slice(firstPart)) {
      part.position.y *= WORLD_PRESENTATION.production.workSurfaceHeightRatio;
      part.scale.y *= WORLD_PRESENTATION.production.workSurfaceHeightRatio;
    }
  }

  private buildSleepingPlace(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const firstPart = group.children.length;
    // Ground-level woven grass/fiber, with ragged edges and three branches.
    // Facility groups sit 0.025 above terrain; the occupied center therefore
    // meets the same support plane as the native LayToIdle calibration.
    const surface = RESIDENT_PRESENTATION_CONTRACT.sleepPoseSleepingPlaceSurfaceHeightWorldUnits;
    this.addBoxAtProgress(group, trace, progress, 0.36, this.beddingMaterial,
      [0, surface - 0.025 - 0.045, 0], [2.24, 0.09, 0.66], [0, 0, 0], 70);
    this.addBoxAtProgress(group, trace, progress, 0.52, this.thatchMaterial,
      [-0.09, 0.06, -0.39], [2.08, 0.06, 0.24], [0, 0.025, 0], 71);
    this.addBoxAtProgress(group, trace, progress, 0.58, this.beddingMaterial,
      [0.07, 0.06, 0.40], [2.32, 0.06, 0.23], [0, -0.035, 0], 72);
    // A loose fiber bundle merges with the mat edge; no raised pillow.
    this.addBoxAtProgress(group, trace, progress, CONSTRUCTION.finishingStart, this.thatchMaterial,
      [-0.77, 0.075, -0.31], [0.65, 0.075, 0.23], [0, 0.16, 0], 73);
    for (const [i, [x, z, length, yaw]] of [
      [0, -0.51, 2.37, 0.03], [0.11, 0.52, 2.12, -0.06], [-1.12, 0.04, 0.82, Math.PI / 2],
    ].entries()) {
      this.addCylinderAtProgress(group, trace, progress, 0.06 + i * 0.04,
        this.woodMaterial, [x, 0.05, z], [0.055, length, 0.055],
        [0, yaw, Math.PI / 2], 74 + i);
    }
    // Seven primitive parts, three material/geometry batches: preserve the
    // original bed's draw-call budget. Allocate only when this cache rebuilds.
    const parts = group.children.slice(firstPart) as THREE.Mesh[];
    const band = group.userData.visualState?.durabilityBand ?? 0;
    if (band >= 2) for (const part of parts) {
      if (part.name.endsWith('-70')) continue;
      part.scale.x *= 1 - CONSTRUCTION.wearAmounts[band] * 0.25;
      part.rotation.y += CONSTRUCTION.wearAmounts[band] * 0.18;
    }
    for (const material of [this.beddingMaterial, this.thatchMaterial, this.woodMaterial]) {
      const matching = parts.filter(part => part.material === material);
      if (!matching.length) continue;
      const batch = new THREE.InstancedMesh(matching[0].geometry, material, matching.length);
      this.prepareMesh(batch, trace, 70 + group.children.length);
      matching.forEach((part, index) => {
        part.updateMatrix(); batch.setMatrixAt(index, part.matrix); group.remove(part);
      });
      batch.instanceMatrix.needsUpdate = true;
      batch.computeBoundingBox(); batch.computeBoundingSphere();
      group.add(batch);
    }
  }

  private disposeStructureInstances(group: THREE.Group): void {
    group.traverse(object => {
      if (object instanceof THREE.InstancedMesh) object.dispose();
    });
  }

  private buildShelter(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const posts = [
      [-1.45, -1.15],
      [1.45, -1.15],
      [-1.45, 1.15],
      [1.45, 1.15],
    ] as const;
    posts.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.06 + index * 0.04,
        this.woodMaterial,
        [x, 1.35, z],
        [0.12, 2.7, 0.12],
        [0, 0, 0],
        80 + index,
      );
    });

    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.38,
      this.darkWoodMaterial,
      [0, 2.55, -1.15],
      [3.15, 0.15, 0.15],
      [0, 0, 0],
      85,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.46,
      this.darkWoodMaterial,
      [0, 2.55, 1.15],
      [3.15, 0.15, 0.15],
      [0, 0, 0],
      86,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.58,
      this.thatchMaterial,
      [-0.76, 2.88, 0],
      [1.85, 0.17, 2.75],
      [0, 0, -0.27],
      87,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.68,
      this.thatchMaterial,
      [0.76, 2.88, 0],
      [1.85, 0.17, 2.75],
      [0, 0, 0.27],
      88,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.82,
      this.woodMaterial,
      [0, 1.3, 1.12],
      [2.7, 2.35, 0.1],
      [0, 0, 0],
      89,
    );
    // Finish the existing shelter envelope as construction reaches completion.
    // Side panels remain inside its original clearance; no new site is invented.
    const wall = WORLD_PRESENTATION.shelter;
    for (const direction of [-1, 1]) {
      this.addBoxAtProgress(group, trace, progress, wall.wallProgress,
        this.thatchMaterial, [direction * wall.sideX, wall.wallY, 0],
        [wall.wallThickness, wall.wallHeight, wall.wallDepth], [0, 0, 0], 910 + direction);
    }
  }

  private buildFurnace(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.18,
      this.stoneMaterial,
      [0, 0.56, 0],
      [1.1, 1.12, 1.1],
      [0, 0, 0],
      100,
      true,
    );
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.48,
      this.earthMaterial,
      [0, 1.32, 0],
      [0.84, 0.7, 0.84],
      [0, 0, 0],
      101,
      true,
    );
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.72,
      this.stoneMaterial,
      [0, 1.98, 0],
      [0.48, 0.78, 0.48],
      [0, 0, 0],
      102,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.58,
      this.charMaterial,
      [0, 0.62, -0.56],
      [0.66, 0.48, 0.16],
      [0, 0, 0],
      103,
    );

    if (trace.lit && progress >= 0.75 && trace.state !== 'Ruined') {
      const flame = new THREE.Mesh(
        this.flameGeometry,
        this.flameMaterial,
      );
      flame.position.set(0, 0.72, -0.66);
      flame.scale.set(0.36, 0.55, 0.36);
      flame.rotation.x = Math.PI * 0.08;
      this.prepareMesh(flame, trace, 104);
      group.add(flame);
      this.addFireGlow(group, flame.position.y, flame.position.z);
    }
  }

  private buildCultivatedPlot(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    // Prepared soil is visible as construction advances. Crop geometry appears
    // only from authoritative planted/growth state carried by HumanTrace.
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.06,
      this.earthMaterial,
      [0, 0.07, 0],
      [3.5, 0.14, 2.45],
      [0, 0, 0],
      120,
    );

    const edgePosts = [
      [-1.65, -1.08],
      [1.65, -1.08],
      [-1.65, 1.08],
      [1.65, 1.08],
    ] as const;
    edgePosts.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.42 + index * 0.035,
        this.woodMaterial,
        [x, 0.34, z],
        [0.07, 0.68, 0.07],
        [0, 0, 0],
        121 + index,
      );
    });

    const cultivation = WORLD_PRESENTATION.cultivation;
    for (const [index, row] of cultivation.rows.entries()) {
      this.addBoxAtProgress(group, trace, progress, 0.42, this.darkWoodMaterial,
        [0, 0.16, row], [cultivation.length, cultivation.ridgeHeight, cultivation.ridgeWidth],
        [0, 0, 0], 920 + index);
    }
    if (trace.state !== 'Operational' || !trace.cropPlanted) return;

    const growth = clamp01(trace.cropGrowth01);
    const harvestReady = Number(trace.cropHarvestUnits) > 0;
    const cropHeight = 0.18 + 0.92 * growth;
    const cropWidth = 0.045 + 0.055 * growth;
    const material = harvestReady
      ? this.ripeCropMaterial
      : this.cropMaterial;

    const rows = [-0.72, 0, 0.72];
    const columns = [-1.18, -0.4, 0.4, 1.18];
    let slot = 130;
    for (const z of rows) {
      for (const x of columns) {
        const wobble = (hash01(`${trace.id}:${slot}:crop`) - 0.5) * 0.12;
        this.addCylinder(
          group,
          trace,
          material,
          [x + wobble, 0.12 + cropHeight * 0.5, z],
          [cropWidth, cropHeight, cropWidth],
          [0, wobble, 0],
          slot,
        );
        if (growth >= 0.35) {
          this.addBox(
            group,
            trace,
            material,
            [x + wobble + 0.08, 0.22 + cropHeight * 0.55, z],
            [0.22 + 0.12 * growth, 0.045, 0.08 + 0.06 * growth],
            [0, wobble, 0.32],
            slot + 40,
          );
        }
        slot += 1;
      }
    }
  }

  private buildUnknownFacility(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const corners = [
      [-0.9, -0.7],
      [0.9, -0.7],
      [-0.9, 0.7],
      [0.9, 0.7],
    ] as const;
    corners.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.06 + index * 0.06,
        this.woodMaterial,
        [x, 0.7, z],
        [0.1, 1.4, 0.1],
        [0, 0, 0],
        110 + index,
      );
    });
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.62,
      this.darkWoodMaterial,
      [0, 1.25, 0],
      [2.1, 0.18, 1.7],
      [0, 0, 0],
      115,
    );
  }

  private addBoxAtProgress(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    if (!this.shouldShowPart(trace, progress, threshold, slot)) return;
    const reveal = trace.state === 'Ruined' ? 1 : partReveal(progress, threshold,
      Math.min(1, threshold + CONSTRUCTION.revealSpan));
    if (reveal <= 0) return;
    // Upright pieces rise from their existing bottom, rather than floating
    // around their finished center while work is still partial.
    if (Math.abs(rotation[0]) + Math.abs(rotation[2]) < 0.1) {
      position = [position[0], position[1] - scale[1] * (1 - reveal) / 2, position[2]];
    }
    scale = [scale[0] * reveal, scale[1] * reveal, scale[2] * reveal];
    this.addBox(
      group,
      trace,
      material,
      position,
      scale,
      this.ruinedRotation(trace, rotation, slot),
      slot,
    );
  }

  private addCylinderAtProgress(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
    tapered = false,
  ): void {
    if (!this.shouldShowPart(trace, progress, threshold, slot)) return;
    const reveal = trace.state === 'Ruined' ? 1 : partReveal(progress, threshold,
      Math.min(1, threshold + CONSTRUCTION.revealSpan));
    if (reveal <= 0) return;
    // Upright pieces rise from their existing bottom, rather than floating
    // around their finished center while work is still partial.
    if (Math.abs(rotation[0]) + Math.abs(rotation[2]) < 0.1) {
      position = [position[0], position[1] - scale[1] * (1 - reveal) / 2, position[2]];
    }
    scale = [scale[0] * reveal, scale[1] * reveal, scale[2] * reveal];
    const mesh = new THREE.Mesh(
      tapered
        ? this.taperedCylinderGeometry
        : this.cylinderGeometry,
      material,
    );
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...this.ruinedRotation(trace, rotation, slot));
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private addStoneAtProgress(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    if (!this.shouldShowPart(trace, progress, threshold, slot)) return;
    const reveal = trace.state === 'Ruined' ? 1 : partReveal(progress, threshold,
      Math.min(1, threshold + CONSTRUCTION.revealSpan));
    if (reveal <= 0) return;
    // Upright pieces rise from their existing bottom, rather than floating
    // around their finished center while work is still partial.
    if (Math.abs(rotation[0]) + Math.abs(rotation[2]) < 0.1) {
      position = [position[0], position[1] - scale[1] * (1 - reveal) / 2, position[2]];
    }
    scale = [scale[0] * reveal, scale[1] * reveal, scale[2] * reveal];
    const mesh = new THREE.Mesh(
      this.stoneGeometry,
      this.stoneMaterial,
    );
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...this.ruinedRotation(trace, rotation, slot));
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private addBox(
    group: THREE.Group,
    trace: FacilityTrace,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    const mesh = new THREE.Mesh(this.boxGeometry, material);
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...rotation);
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private addCylinder(
    group: THREE.Group,
    trace: FacilityTrace,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    const mesh = new THREE.Mesh(this.cylinderGeometry, material);
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...rotation);
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private shouldShowPart(
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    slot: number,
  ): boolean {
    if (trace.state !== 'Ruined') return progress > threshold;
    return slot % 4 !== 0;
  }

  private ruinedRotation(
    trace: FacilityTrace,
    rotation: [number, number, number],
    slot: number,
  ): [number, number, number] {
    if (trace.state !== 'Ruined') return rotation;
    const direction = slot % 2 === 0 ? 1 : -1;
    return [
      rotation[0] + 0.08 * direction,
      rotation[1],
      rotation[2] + 0.2 * direction,
    ];
  }

  private prepareMesh(
    mesh: THREE.Mesh,
    trace: FacilityTrace,
    slot: number,
  ): void {
    mesh.name = `${trace.facilityKind}-${slot}`;
    mesh.userData.traceId = trace.id;
    mesh.castShadow = false;
    mesh.receiveShadow = true;
  }
}




