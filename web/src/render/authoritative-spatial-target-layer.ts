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

interface TargetWorldPosition {
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

function stableUnit(value: string): number {
  let hash = 2166136261 >>> 0;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

function isAuthoritativeNaturalResourceMaterial(
  material: string,
): material is AuthoritativeNaturalResourceMaterial {
  return MATERIAL_SET.has(material);
}

export function authoritativeGridWorldPosition(
  gridX: number,
  gridY: number,
  terrain: TerrainWindow,
): TargetWorldPosition | null {
  if (!terrain.available || terrain.chunks.length === 0) return null;

  const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const chunkX = Math.floor(gridX / span);
  const chunkY = Math.floor(gridY / span);
  if (!terrain.chunks.some(
    (chunk) => chunk.x === chunkX && chunk.y === chunkY,
  )) {
    return null;
  }

  const localX01 = gridX / span - chunkX;
  const localY01 = gridY / span - chunkY;
  const sampleElevation = createTerrainElevationSampler(terrain);
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
}

export function visibleAuthoritativeResourceSites(
  civilization: CivilizationWorldPayload,
  terrain: TerrainWindow,
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
    const position = authoritativeGridWorldPosition(
      targetGridX,
      targetGridY,
      terrain,
    );
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

    const position = authoritativeGridWorldPosition(
      site.gridX,
      site.gridY,
      terrain,
    );
    if (!position) continue;

    result.push({
      id: site.id,
      kind: site.kind,
      position,
    });
  }
  return result;
}

function resourceSignature(
  resources: CivilizationWorldResource[],
): string {
  return resources
    .map((resource) => (
      `${resource.id}:${resource.material}:${resource.gridX}:`
      + `${resource.gridY}:${resource.hasAccessGrid ? 1 : 0}:`
      + `${resource.accessGridX ?? ''}:${resource.accessGridY ?? ''}:`
      + `${resource.quantity}:${resource.maxQuantity}`
    ))
    .sort()
    .join('|');
}

function sanitationSignature(
  sites: WorldSanitationSite[],
): string {
  return sites
    .map((site) => (
      `${site.id}:${site.kind}:${site.gridX}:${site.gridY}:`
      + `${site.active ? 1 : 0}:${site.improvementWork}`
    ))
    .sort()
    .join('|');
}

export class AuthoritativeSpatialTargetLayer {
  readonly group = new THREE.Group();

  private readonly woodTrunks = new THREE.InstancedMesh(
    new THREE.CylinderGeometry(1, 1.08, 1, 8),
    new THREE.MeshStandardMaterial({
      color: 0x68452d,
      roughness: 0.96,
      metalness: 0,
    }),
    MAX_VISIBLE_RESOURCE_SITES * 2,
  );
  private readonly woodCrowns = new THREE.InstancedMesh(
    new THREE.IcosahedronGeometry(1, 1),
    new THREE.MeshLambertMaterial({ color: 0x365f32 }),
    MAX_VISIBLE_RESOURCE_SITES * 2,
  );
  private readonly stoneSites = new THREE.InstancedMesh(
    new THREE.DodecahedronGeometry(1, 0),
    new THREE.MeshStandardMaterial({
      color: 0x77766d,
      roughness: 0.96,
      metalness: 0,
    }),
    MAX_VISIBLE_RESOURCE_SITES,
  );
  private readonly flintSites = new THREE.InstancedMesh(
    new THREE.DodecahedronGeometry(1, 0),
    new THREE.MeshStandardMaterial({
      color: 0x4d514f,
      roughness: 0.9,
      metalness: 0,
    }),
    MAX_VISIBLE_RESOURCE_SITES,
  );
  private readonly fiberSites = new THREE.InstancedMesh(
    new THREE.ConeGeometry(0.3, 1, 5),
    new THREE.MeshLambertMaterial({ color: 0x6d8244 }),
    MAX_VISIBLE_RESOURCE_SITES,
  );
  private readonly claySites = new THREE.InstancedMesh(
    new THREE.CylinderGeometry(1, 1.15, 0.22, 12),
    new THREE.MeshStandardMaterial({
      color: 0x8d5f48,
      roughness: 1,
      metalness: 0,
    }),
    MAX_VISIBLE_RESOURCE_SITES,
  );
  private readonly plantFoodSites = new THREE.InstancedMesh(
    new THREE.IcosahedronGeometry(1, 1),
    new THREE.MeshLambertMaterial({ color: 0x4f7337 }),
    MAX_VISIBLE_RESOURCE_SITES,
  );
  private readonly copperSites = new THREE.InstancedMesh(
    new THREE.DodecahedronGeometry(1, 0),
    new THREE.MeshStandardMaterial({
      color: 0x8e684b,
      roughness: 0.9,
      metalness: 0.08,
    }),
    MAX_VISIBLE_RESOURCE_SITES,
  );
  private readonly tinSites = new THREE.InstancedMesh(
    new THREE.DodecahedronGeometry(1, 0),
    new THREE.MeshStandardMaterial({
      color: 0x858a8c,
      roughness: 0.86,
      metalness: 0.1,
    }),
    MAX_VISIBLE_RESOURCE_SITES,
  );
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
  private readonly rotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();
  private signature = '';

  constructor() {
    this.group.name = 'authoritative-spatial-targets';

    for (const mesh of this.resourceMeshes()) {
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.castShadow = false;
      mesh.receiveShadow = true;
      mesh.count = 0;
      this.group.add(mesh);
    }

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
    const nextSignature = [
      terrain.worldSeed ?? '0',
      terrain.centerChunkX,
      terrain.centerChunkY,
      resourceSignature(civilization.resources ?? []),
      sanitationSignature(worldObjects.sanitationSites ?? []),
    ].join('|');
    if (nextSignature === this.signature) return;
    this.signature = nextSignature;

    const resources = visibleAuthoritativeResourceSites(
      civilization,
      terrain,
    );
    const sanitation = visibleAuthoritativeSanitationSites(
      worldObjects,
      terrain,
    );

    const counts: Record<AuthoritativeNaturalResourceMaterial, number> = {
      Wood: 0,
      Stone: 0,
      Flint: 0,
      Fiber: 0,
      Clay: 0,
      PlantFood: 0,
      CopperOre: 0,
      TinOre: 0,
    };

    let woodInstanceCount = 0;
    for (const resource of resources) {
      const count = counts[resource.material];
      const ratio = resource.maxQuantity > 0
        ? Math.max(0, Math.min(1, resource.quantity / resource.maxQuantity))
        : 1;
      const fullness = 0.84 + Math.sqrt(ratio) * 0.2;
      const yaw = stableUnit(`${resource.id}:${resource.material}`)
        * Math.PI * 2;
      this.rotation.setFromAxisAngle(
        new THREE.Vector3(0, 1, 0),
        yaw,
      );

      switch (resource.material) {
        case 'Wood': {
          // ResourceNode is the interaction center of a wood patch. Keep that
          // center clear for the resident and place visible trees around it so
          // gathering reads as happening beside real trees, not inside a trunk.
          for (let treeOrdinal = 0; treeOrdinal < 2; treeOrdinal += 1) {
            const angle = yaw + treeOrdinal * Math.PI;
            const offset = 0.58 + treeOrdinal * 0.08;
            const treeX = resource.position.x + Math.cos(angle) * offset;
            const treeZ = resource.position.z + Math.sin(angle) * offset;
            const treeFullness = fullness * (treeOrdinal === 0 ? 1 : 0.88);
            const height = 3.0 * treeFullness;
            const radius = 0.22 * treeFullness;

            this.position.set(
              treeX,
              resource.position.y + height * 0.5,
              treeZ,
            );
            this.scale.set(radius, height, radius);
            this.matrix.compose(
              this.position,
              this.rotation,
              this.scale,
            );
            this.woodTrunks.setMatrixAt(
              woodInstanceCount,
              this.matrix,
            );

            this.position.set(
              treeX,
              resource.position.y + height * 0.9,
              treeZ,
            );
            this.scale.set(
              1.08 * treeFullness,
              1.34 * treeFullness,
              1.08 * treeFullness,
            );
            this.matrix.compose(
              this.position,
              this.rotation,
              this.scale,
            );
            this.woodCrowns.setMatrixAt(
              woodInstanceCount,
              this.matrix,
            );
            woodInstanceCount += 1;
          }
          break;
        }
        case 'Stone':
          this.setGroundResource(
            this.stoneSites,
            count,
            resource.position,
            0.48 * fullness,
            0.34 * fullness,
          );
          break;
        case 'Flint':
          this.setGroundResource(
            this.flintSites,
            count,
            resource.position,
            0.38 * fullness,
            0.24 * fullness,
          );
          break;
        case 'Fiber':
          this.setGroundResource(
            this.fiberSites,
            count,
            resource.position,
            0.52 * fullness,
            0.78 * fullness,
          );
          break;
        case 'Clay':
          this.setGroundResource(
            this.claySites,
            count,
            resource.position,
            0.62 * fullness,
            0.16,
          );
          break;
        case 'PlantFood':
          this.setGroundResource(
            this.plantFoodSites,
            count,
            resource.position,
            0.62 * fullness,
            0.58 * fullness,
          );
          break;
        case 'CopperOre':
          this.setGroundResource(
            this.copperSites,
            count,
            resource.position,
            0.46 * fullness,
            0.3 * fullness,
          );
          break;
        case 'TinOre':
          this.setGroundResource(
            this.tinSites,
            count,
            resource.position,
            0.43 * fullness,
            0.28 * fullness,
          );
          break;
      }
      counts[resource.material] += 1;
    }

    this.woodTrunks.count = woodInstanceCount;
    this.woodCrowns.count = woodInstanceCount;
    this.stoneSites.count = counts.Stone;
    this.flintSites.count = counts.Flint;
    this.fiberSites.count = counts.Fiber;
    this.claySites.count = counts.Clay;
    this.plantFoodSites.count = counts.PlantFood;
    this.copperSites.count = counts.CopperOre;
    this.tinSites.count = counts.TinOre;

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
      ...this.resourceMeshes(),
      this.sanitationAreaSites,
      this.sanitationPitSites,
    ]) {
      mesh.instanceMatrix.needsUpdate = true;
      if (mesh.count > 0) mesh.computeBoundingSphere();
    }
  }

  dispose(): void {
    for (const mesh of [
      ...this.resourceMeshes(),
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

  private setGroundResource(
    mesh: THREE.InstancedMesh,
    index: number,
    position: TargetWorldPosition,
    horizontalScale: number,
    verticalScale: number,
  ): void {
    this.position.set(
      position.x,
      position.y + verticalScale * 0.5,
      position.z,
    );
    this.scale.set(
      horizontalScale,
      verticalScale,
      horizontalScale,
    );
    this.matrix.compose(
      this.position,
      this.rotation,
      this.scale,
    );
    mesh.setMatrixAt(index, this.matrix);
  }

  private resourceMeshes(): THREE.InstancedMesh[] {
    return [
      this.woodTrunks,
      this.woodCrowns,
      this.stoneSites,
      this.flintSites,
      this.fiberSites,
      this.claySites,
      this.plantFoodSites,
      this.copperSites,
      this.tinSites,
    ];
  }
}
