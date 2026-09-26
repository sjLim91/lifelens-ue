import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';
import { useMobileVegetationProfile } from './vegetation-profile';

const MAX_GRASS_TUFTS = 9000;
const MAX_SHRUBS = 2600;
const MAX_ROCKS = 3200;
const UP = new THREE.Vector3(0, 1, 0);

const GRASS_PALETTE = [
  0x496b36,
  0x58783b,
  0x3f6233,
  0x6f7d43,
] as const;

const SHRUB_PALETTE = [
  0x315b32,
  0x3c6938,
  0x294e2d,
  0x4b7440,
] as const;

const ROCK_PALETTE = [
  0x68685e,
  0x79776a,
  0x595b54,
  0x827b6b,
] as const;

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function hash01(
  seed: string,
  x: number,
  y: number,
  index: number,
  salt: number,
): number {
  const input = `${seed}:${x}:${y}:${index}:${salt}`;
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < input.length; i += 1) {
    hash ^= input.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

function createGrassGeometry(): THREE.BufferGeometry {
  const halfWidth = 0.12;
  const height = 0.68;
  const positions = new Float32Array([
    -halfWidth, 0, 0,
    halfWidth, 0, 0,
    halfWidth * 0.62, height, 0,
    -halfWidth * 0.62, height, 0,

    0, 0, -halfWidth,
    0, 0, halfWidth,
    0, height, halfWidth * 0.62,
    0, height, -halfWidth * 0.62,
  ]);
  const indices = [
    0, 1, 2, 0, 2, 3,
    4, 5, 6, 4, 6, 7,
  ];

  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute(
    'position',
    new THREE.BufferAttribute(positions, 3),
  );
  geometry.setIndex(indices);
  geometry.computeVertexNormals();
  geometry.computeBoundingSphere();
  return geometry;
}

function isOpenWater(chunk: TerrainChunk): boolean {
  return chunk.waterKind === 'Ocean'
    || chunk.waterKind === 'Lake'
    || chunk.waterKind === 'River'
    || chunk.waterKind === 'Stream';
}

export class GroundDetailLayer {
  readonly group = new THREE.Group();

  private readonly mobileProfile = useMobileVegetationProfile();
  private readonly grassGeometry = createGrassGeometry();
  private readonly shrubGeometry = new THREE.IcosahedronGeometry(1, 0);
  private readonly rockGeometry = new THREE.DodecahedronGeometry(1, 0);

  private readonly grassMaterial = new THREE.MeshLambertMaterial({
    color: 0xffffff,
    side: THREE.DoubleSide,
  });
  private readonly shrubMaterial = new THREE.MeshLambertMaterial({
    color: 0xffffff,
  });
  private readonly rockMaterial = new THREE.MeshStandardMaterial({
    color: 0xffffff,
    roughness: 0.94,
    metalness: 0,
  });

  private readonly grass = new THREE.InstancedMesh(
    this.grassGeometry,
    this.grassMaterial,
    MAX_GRASS_TUFTS,
  );
  private readonly shrubs = new THREE.InstancedMesh(
    this.shrubGeometry,
    this.shrubMaterial,
    MAX_SHRUBS,
  );
  private readonly rocks = new THREE.InstancedMesh(
    this.rockGeometry,
    this.rockMaterial,
    MAX_ROCKS,
  );

  private readonly matrix = new THREE.Matrix4();
  private readonly rotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();
  private readonly position = new THREE.Vector3();
  private readonly color = new THREE.Color();

  constructor() {
    for (const mesh of [this.grass, this.shrubs, this.rocks]) {
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.castShadow = false;
      mesh.receiveShadow = true;
      mesh.frustumCulled = true;
      this.group.add(mesh);
    }
  }

  setTerrain(window: TerrainWindow): void {
    const seed = window.worldSeed ?? '0';
    const sampleElevation = createTerrainElevationSampler(window);
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const halfChunk = chunkWorldSize * 0.5;

    const budgets = this.mobileProfile
      ? { grass: 14, shrubs: 4, rocks: 5 }
      : { grass: 24, shrubs: 7, rocks: 8 };

    let grassIndex = 0;
    let shrubIndex = 0;
    let rockIndex = 0;

    for (const chunk of window.chunks) {
      const grassCoverage = clamp01(chunk.grassCoverage01);
      const shrubCoverage = clamp01(chunk.shrubCoverage01);
      const rockCoverage = clamp01(chunk.rockCoverage01);
      const wetlandCoverage = clamp01(chunk.wetlandCoverage01);
      const forestCoverage = clamp01(chunk.forestCoverage01);

      const grassCount = isOpenWater(chunk)
        ? 0
        : Math.round(
          budgets.grass
          * Math.max(grassCoverage, wetlandCoverage * 0.55)
          * (1 - rockCoverage * 0.35),
        );
      const shrubCount = isOpenWater(chunk)
        ? 0
        : Math.round(
          budgets.shrubs
          * Math.max(shrubCoverage, forestCoverage * 0.24),
        );
      const rockCount = chunk.waterKind === 'Ocean'
        ? 0
        : Math.round(
          budgets.rocks
          * Math.max(
            rockCoverage,
            chunk.waterKind === 'Coast' ? 0.34 : 0,
          ),
        );

      for (
        let index = 0;
        index < grassCount && grassIndex < MAX_GRASS_TUFTS;
        index += 1
      ) {
        const placement = this.clusteredPlacement(
          seed,
          chunk,
          index,
          101,
          halfChunk,
        );
        const groundY = sampleElevation(
          chunk.x,
          chunk.y,
          placement.localX01,
          placement.localY01,
        ) * WORLD_GRID_CONTRACT.elevationScale;
        const size = 0.52 + hash01(
          seed,
          chunk.x,
          chunk.y,
          index,
          109,
        ) * 0.7;

        this.rotation.setFromAxisAngle(
          UP,
          hash01(seed, chunk.x, chunk.y, index, 113) * Math.PI * 2,
        );
        this.position.set(
          (chunk.x - window.centerChunkX) * chunkWorldSize + placement.x,
          groundY + 0.02,
          (chunk.y - window.centerChunkY) * chunkWorldSize + placement.z,
        );
        this.scale.set(size, size, size);
        this.matrix.compose(
          this.position,
          this.rotation,
          this.scale,
        );
        this.grass.setMatrixAt(grassIndex, this.matrix);

        const paletteIndex = Math.min(
          GRASS_PALETTE.length - 1,
          Math.floor(
            hash01(seed, chunk.x, chunk.y, index, 127)
            * GRASS_PALETTE.length,
          ),
        );
        this.color.setHex(GRASS_PALETTE[paletteIndex]);
        this.grass.setColorAt(grassIndex, this.color);
        grassIndex += 1;
      }

      for (
        let index = 0;
        index < shrubCount && shrubIndex < MAX_SHRUBS;
        index += 1
      ) {
        const placement = this.clusteredPlacement(
          seed,
          chunk,
          index,
          211,
          halfChunk,
        );
        const groundY = sampleElevation(
          chunk.x,
          chunk.y,
          placement.localX01,
          placement.localY01,
        ) * WORLD_GRID_CONTRACT.elevationScale;
        const width = 0.34 + hash01(
          seed,
          chunk.x,
          chunk.y,
          index,
          223,
        ) * 0.44;
        const height = 0.28 + hash01(
          seed,
          chunk.x,
          chunk.y,
          index,
          227,
        ) * 0.52;

        this.rotation.setFromAxisAngle(
          UP,
          hash01(seed, chunk.x, chunk.y, index, 229) * Math.PI * 2,
        );
        this.position.set(
          (chunk.x - window.centerChunkX) * chunkWorldSize + placement.x,
          groundY + height * 0.68,
          (chunk.y - window.centerChunkY) * chunkWorldSize + placement.z,
        );
        this.scale.set(width, height, width * 0.92);
        this.matrix.compose(
          this.position,
          this.rotation,
          this.scale,
        );
        this.shrubs.setMatrixAt(shrubIndex, this.matrix);

        const paletteIndex = Math.min(
          SHRUB_PALETTE.length - 1,
          Math.floor(
            hash01(seed, chunk.x, chunk.y, index, 233)
            * SHRUB_PALETTE.length,
          ),
        );
        this.color.setHex(SHRUB_PALETTE[paletteIndex]);
        this.shrubs.setColorAt(shrubIndex, this.color);
        shrubIndex += 1;
      }

      for (
        let index = 0;
        index < rockCount && rockIndex < MAX_ROCKS;
        index += 1
      ) {
        const placement = this.clusteredPlacement(
          seed,
          chunk,
          index,
          307,
          halfChunk,
        );
        const groundY = sampleElevation(
          chunk.x,
          chunk.y,
          placement.localX01,
          placement.localY01,
        ) * WORLD_GRID_CONTRACT.elevationScale;
        const width = 0.16 + hash01(
          seed,
          chunk.x,
          chunk.y,
          index,
          311,
        ) * 0.42;
        const height = width * (
          0.48 + hash01(seed, chunk.x, chunk.y, index, 313) * 0.54
        );

        this.rotation.setFromAxisAngle(
          UP,
          hash01(seed, chunk.x, chunk.y, index, 317) * Math.PI * 2,
        );
        this.position.set(
          (chunk.x - window.centerChunkX) * chunkWorldSize + placement.x,
          groundY + height * 0.52,
          (chunk.y - window.centerChunkY) * chunkWorldSize + placement.z,
        );
        this.scale.set(
          width,
          height,
          width * (
            0.72 + hash01(seed, chunk.x, chunk.y, index, 331) * 0.38
          ),
        );
        this.matrix.compose(
          this.position,
          this.rotation,
          this.scale,
        );
        this.rocks.setMatrixAt(rockIndex, this.matrix);

        const paletteIndex = Math.min(
          ROCK_PALETTE.length - 1,
          Math.floor(
            hash01(seed, chunk.x, chunk.y, index, 337)
            * ROCK_PALETTE.length,
          ),
        );
        this.color.setHex(ROCK_PALETTE[paletteIndex]);
        this.rocks.setColorAt(rockIndex, this.color);
        rockIndex += 1;
      }
    }

    this.finalize(this.grass, grassIndex);
    this.finalize(this.shrubs, shrubIndex);
    this.finalize(this.rocks, rockIndex);
  }

  setWetness(wetness01: number): void {
    const wetness = clamp01(wetness01);
    this.grassMaterial.color.setScalar(1 - wetness * 0.12);
    this.shrubMaterial.color.setScalar(1 - wetness * 0.1);
    this.rockMaterial.color.setScalar(1 - wetness * 0.22);
    this.rockMaterial.roughness = Math.max(0.46, 0.94 - wetness * 0.42);
  }

  dispose(): void {
    this.grassGeometry.dispose();
    this.shrubGeometry.dispose();
    this.rockGeometry.dispose();
    this.grassMaterial.dispose();
    this.shrubMaterial.dispose();
    this.rockMaterial.dispose();
  }

  private clusteredPlacement(
    seed: string,
    chunk: TerrainChunk,
    index: number,
    salt: number,
    halfChunk: number,
  ): {
    x: number;
    z: number;
    localX01: number;
    localY01: number;
  } {
    const clusterIndex = Math.floor(index / 3);
    const clusterX = (
      hash01(seed, chunk.x, chunk.y, clusterIndex, salt) - 0.5
    ) * halfChunk * 1.55;
    const clusterZ = (
      hash01(seed, chunk.x, chunk.y, clusterIndex, salt + 1) - 0.5
    ) * halfChunk * 1.55;
    const jitterRadius = halfChunk * 0.24;
    const x = THREE.MathUtils.clamp(
      clusterX
        + (
          hash01(seed, chunk.x, chunk.y, index, salt + 2) - 0.5
        ) * jitterRadius,
      -halfChunk * 0.88,
      halfChunk * 0.88,
    );
    const z = THREE.MathUtils.clamp(
      clusterZ
        + (
          hash01(seed, chunk.x, chunk.y, index, salt + 3) - 0.5
        ) * jitterRadius,
      -halfChunk * 0.88,
      halfChunk * 0.88,
    );
    const chunkSize = halfChunk * 2;

    return {
      x,
      z,
      localX01: THREE.MathUtils.clamp(
        (x + halfChunk) / chunkSize,
        0,
        1,
      ),
      localY01: THREE.MathUtils.clamp(
        (z + halfChunk) / chunkSize,
        0,
        1,
      ),
    };
  }

  private finalize(mesh: THREE.InstancedMesh, count: number): void {
    mesh.count = count;
    mesh.instanceMatrix.needsUpdate = true;
    if (mesh.instanceColor) {
      mesh.instanceColor.needsUpdate = true;
    }
    mesh.computeBoundingSphere();
  }
}
