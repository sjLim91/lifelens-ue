import * as THREE from 'three';
import type {
  CivilizationWorldPayload,
  CivilizationWorldResource,
  TerrainWindow,
  WorldObjectsPayload,
  WorldSanitationSite,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';

export const AUTHORITATIVE_NATURAL_RESOURCE_MATERIALS = [
  'Wood',
  'Stone',
  'Flint',
  'Fiber',
  'Clay',
  'PlantFood',
  'CopperOre',
  'TinOre',
] as const;

export type AuthoritativeNaturalResourceMaterial =
  (typeof AUTHORITATIVE_NATURAL_RESOURCE_MATERIALS)[number];

const MATERIAL_SET = new Set<string>(
  AUTHORITATIVE_NATURAL_RESOURCE_MATERIALS,
);

const MAX_VISIBLE_RESOURCE_SITES = 2048;
const MAX_VISIBLE_SANITATION_SITES = 128;
const EMPTY_RESOURCES: CivilizationWorldResource[] = [];
const EMPTY_SANITATION_SITES: WorldSanitationSite[] = [];

export interface TargetWorldPosition {
  x: number;
  y: number;
  z: number;
}

export interface AuthoritativeResourceSite {
  id: string;
  material: AuthoritativeNaturalResourceMaterial;
  quantity: number;
  maxQuantity: number;
  position: TargetWorldPosition;
}

export interface AuthoritativeSanitationSite {
  id: string;
  kind: string;
  position: TargetWorldPosition;
}

function isAuthoritativeNaturalResourceMaterial(
  material: string,
): material is AuthoritativeNaturalResourceMaterial {
  return MATERIAL_SET.has(material);
}

export interface AuthoritativeGridProjector {
  containsGrid(gridX: number, gridY: number): boolean;
  project(gridX: number, gridY: number): TargetWorldPosition | null;
}

function terrainChunkKey(chunkX: number, chunkY: number): string {
  return `${chunkX}:${chunkY}`;
}

export function createAuthoritativeGridProjector(
  terrain: TerrainWindow,
): AuthoritativeGridProjector {
  if (!terrain.available || terrain.chunks.length === 0) {
    return {
      containsGrid: () => false,
      project: () => null,
    };
  }

  const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const chunkKeys = new Set(
    terrain.chunks.map((chunk) => terrainChunkKey(chunk.x, chunk.y)),
  );
  // Build the elevation lookup once for the whole target batch. The old path
  // rebuilt this Map for every resource, which made initial projection scale as
  // resources × visible chunks.
  const sampleElevation = createTerrainElevationSampler(terrain);

  const containsGrid = (gridX: number, gridY: number): boolean => {
    const chunkX = Math.floor(gridX / span);
    const chunkY = Math.floor(gridY / span);
    return chunkKeys.has(terrainChunkKey(chunkX, chunkY));
  };

  const project = (
    gridX: number,
    gridY: number,
  ): TargetWorldPosition | null => {
    const chunkX = Math.floor(gridX / span);
    const chunkY = Math.floor(gridY / span);
    if (!chunkKeys.has(terrainChunkKey(chunkX, chunkY))) return null;

    const localX01 = gridX / span - chunkX;
    const localY01 = gridY / span - chunkY;
    return {
      x: (
        chunkX - terrain.centerChunkX + localX01 - 0.5
      ) * WORLD_GRID_CONTRACT.worldUnitsPerChunk,
      y: sampleElevation(
        chunkX,
        chunkY,
        localX01,
        localY01,
      ) * WORLD_GRID_CONTRACT.elevationScale,
      z: (
        chunkY - terrain.centerChunkY + localY01 - 0.5
      ) * WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    };
  };

  return { containsGrid, project };
}

export function authoritativeGridWorldPosition(
  gridX: number,
  gridY: number,
  terrain: TerrainWindow,
): TargetWorldPosition | null {
  return createAuthoritativeGridProjector(terrain).project(gridX, gridY);
}

export function visibleAuthoritativeResourceSites(
  civilization: CivilizationWorldPayload,
  terrain: TerrainWindow,
  projector = createAuthoritativeGridProjector(terrain),
): AuthoritativeResourceSite[] {
  if (civilization.available !== true) return [];

  const result: AuthoritativeResourceSite[] = [];
  for (const resource of civilization.resources ?? []) {
    if (
      result.length >= MAX_VISIBLE_RESOURCE_SITES
      || resource.quantity <= 0
      || !isAuthoritativeNaturalResourceMaterial(resource.material)
    ) {
      continue;
    }

    const targetGridX = resource.hasAccessGrid === true
      && Number.isFinite(resource.accessGridX)
      ? Number(resource.accessGridX)
      : resource.gridX;
    const targetGridY = resource.hasAccessGrid === true
      && Number.isFinite(resource.accessGridY)
      ? Number(resource.accessGridY)
      : resource.gridY;
    if (!projector.containsGrid(targetGridX, targetGridY)) continue;
    const position = projector.project(targetGridX, targetGridY);
    if (!position) continue;

    result.push({
      id: resource.id,
      material: resource.material,
      quantity: resource.quantity,
      maxQuantity: resource.maxQuantity,
      position,
    });
  }
  return result;
}

export function visibleAuthoritativeSanitationSites(
  worldObjects: WorldObjectsPayload,
  terrain: TerrainWindow,
  projector = createAuthoritativeGridProjector(terrain),
): AuthoritativeSanitationSite[] {
  if (worldObjects.available !== true) return [];

  const result: AuthoritativeSanitationSite[] = [];
  for (const site of worldObjects.sanitationSites ?? []) {
    if (
      result.length >= MAX_VISIBLE_SANITATION_SITES
      || site.active !== true
    ) {
      continue;
    }

    if (!projector.containsGrid(site.gridX, site.gridY)) continue;
    const position = projector.project(site.gridX, site.gridY);
    if (!position) continue;

    result.push({
      id: site.id,
      kind: site.kind,
      position,
    });
  }
  return result;
}

function mixHash(hash: number, token: string): number {
  let next = hash >>> 0;
  for (let index = 0; index < token.length; index += 1) {
    next ^= token.charCodeAt(index);
    next = Math.imul(next, 16777619) >>> 0;
  }
  return next >>> 0;
}

function resourceTargetGrid(
  resource: CivilizationWorldResource,
): { x: number; y: number } {
  return {
    x: resource.hasAccessGrid === true
      && Number.isFinite(resource.accessGridX)
      ? Number(resource.accessGridX)
      : resource.gridX,
    y: resource.hasAccessGrid === true
      && Number.isFinite(resource.accessGridY)
      ? Number(resource.accessGridY)
      : resource.gridY,
  };
}

function visibleTargetSignature(
  resources: CivilizationWorldResource[],
  sanitationSites: WorldSanitationSite[],
  projector: AuthoritativeGridProjector,
): string {
  // Core exports resources sorted by id, so a streaming hash is stable without
  // allocating/sorting thousands of signature strings on every refresh.
  let hash = 2166136261 >>> 0;
  let visibleResources = 0;
  for (const resource of resources) {
    if (
      resource.quantity <= 0
      || !isAuthoritativeNaturalResourceMaterial(resource.material)
    ) {
      continue;
    }
    const target = resourceTargetGrid(resource);
    if (!projector.containsGrid(target.x, target.y)) continue;
    hash = mixHash(hash, [
      resource.id,
      resource.material,
      target.x,
      target.y,
      resource.quantity,
      resource.maxQuantity,
    ].join(':'));
    visibleResources += 1;
    if (visibleResources >= MAX_VISIBLE_RESOURCE_SITES) break;
  }

  let visibleSanitation = 0;
  for (const site of sanitationSites) {
    if (
      site.active !== true
      || !projector.containsGrid(site.gridX, site.gridY)
    ) {
      continue;
    }
    hash = mixHash(hash, [
      site.id,
      site.kind,
      site.gridX,
      site.gridY,
      site.improvementWork,
    ].join(':'));
    visibleSanitation += 1;
    if (visibleSanitation >= MAX_VISIBLE_SANITATION_SITES) break;
  }

  return `${hash.toString(16)}:${visibleResources}:${visibleSanitation}`;
}

export class AuthoritativeSpatialTargetLayer {
  readonly group = new THREE.Group();

  private readonly sanitationAreaSites = new THREE.InstancedMesh(
    new THREE.RingGeometry(0.42, 0.62, 18),
    new THREE.MeshBasicMaterial({
      color: 0x8b7752,
      side: THREE.DoubleSide,
      transparent: true,
      opacity: 0.78,
      depthWrite: false,
    }),
    MAX_VISIBLE_SANITATION_SITES,
  );
  private readonly sanitationPitSites = new THREE.InstancedMesh(
    new THREE.RingGeometry(0.28, 0.58, 18),
    new THREE.MeshBasicMaterial({
      color: 0x503d2c,
      side: THREE.DoubleSide,
      transparent: true,
      opacity: 0.9,
      depthWrite: false,
    }),
    MAX_VISIBLE_SANITATION_SITES,
  );

  private readonly matrix = new THREE.Matrix4();
  private readonly position = new THREE.Vector3();
  private readonly scale = new THREE.Vector3();
  private signature = '';
  private lastResourcesRef: CivilizationWorldResource[] | null = null;
  private lastSanitationRef: WorldSanitationSite[] | null = null;
  private lastViewportKey = '';

  constructor() {
    this.group.name = 'authoritative-spatial-targets';
    // Natural resources belong to VegetationLayer/GroundDetailLayer. This layer
    // retains Core interaction projection helpers and sanitation visuals only.

    for (const mesh of [
      this.sanitationAreaSites,
      this.sanitationPitSites,
    ]) {
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.count = 0;
      mesh.renderOrder = 3;
      this.group.add(mesh);
    }
  }

  setTargets(
    civilization: CivilizationWorldPayload,
    worldObjects: WorldObjectsPayload,
    terrain: TerrainWindow,
  ): void {
    const resourcesRef = civilization.resources ?? EMPTY_RESOURCES;
    const sanitationRef =
      worldObjects.sanitationSites ?? EMPTY_SANITATION_SITES;
    const viewportKey = [
      terrain.worldSeed ?? '0',
      terrain.centerChunkX,
      terrain.centerChunkY,
      terrain.radiusChunks ?? '',
      terrain.chunks.length,
    ].join(':');

    // WorldSession deliberately refreshes civilization/world objects less often
    // than the 500 ms observer tick. When those payload references and viewport
    // are unchanged, no spatial-target work is needed at all.
    if (
      resourcesRef === this.lastResourcesRef
      && sanitationRef === this.lastSanitationRef
      && viewportKey === this.lastViewportKey
    ) {
      return;
    }
    this.lastResourcesRef = resourcesRef;
    this.lastSanitationRef = sanitationRef;
    this.lastViewportKey = viewportKey;

    const projector = createAuthoritativeGridProjector(terrain);
    const nextSignature = [
      viewportKey,
      visibleTargetSignature(
        EMPTY_RESOURCES,
        sanitationRef,
        projector,
      ),
    ].join('|');
    if (nextSignature === this.signature) return;
    this.signature = nextSignature;

    const sanitation = visibleAuthoritativeSanitationSites(
      worldObjects,
      terrain,
      projector,
    );

    let designatedCount = 0;
    let pitCount = 0;
    const groundRotation = new THREE.Quaternion().setFromEuler(
      new THREE.Euler(-Math.PI * 0.5, 0, 0),
    );
    for (const site of sanitation) {
      this.position.set(
        site.position.x,
        site.position.y + 0.035,
        site.position.z,
      );
      this.scale.set(1, 1, 1);
      this.matrix.compose(
        this.position,
        groundRotation,
        this.scale,
      );
      if (site.kind === 'DugPit') {
        this.sanitationPitSites.setMatrixAt(pitCount, this.matrix);
        pitCount += 1;
      } else {
        this.sanitationAreaSites.setMatrixAt(
          designatedCount,
          this.matrix,
        );
        designatedCount += 1;
      }
    }
    this.sanitationAreaSites.count = designatedCount;
    this.sanitationPitSites.count = pitCount;

    for (const mesh of [
      this.sanitationAreaSites,
      this.sanitationPitSites,
    ]) {
      mesh.instanceMatrix.needsUpdate = true;
      if (mesh.count > 0) mesh.computeBoundingSphere();
    }
  }

  dispose(): void {
    for (const mesh of [
      this.sanitationAreaSites,
      this.sanitationPitSites,
    ]) {
      mesh.geometry.dispose();
      const materials = Array.isArray(mesh.material)
        ? mesh.material
        : [mesh.material];
      for (const material of materials) material.dispose();
    }
  }

}
