import * as THREE from 'three';
import type { TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';

const MAX_TREES = 4096;
const MAX_SHRUBS = 6144;
const MAX_ROCKS = 3072;

function hash01(seed: string, x: number, y: number, index: number): number {
  const input = `${seed}:${x}:${y}:${index}`;
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < input.length; i += 1) {
    hash ^= input.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

export class VegetationLayer {
  readonly group = new THREE.Group();

  private readonly trunkGeometry = new THREE.CylinderGeometry(
    0.16,
    0.25,
    3.05,
    7,
  );
  private readonly lowerCrownGeometry = new THREE.IcosahedronGeometry(1, 1);
  private readonly middleCrownGeometry = new THREE.IcosahedronGeometry(1, 1);
  private readonly upperCrownGeometry = new THREE.IcosahedronGeometry(1, 1);
  private readonly shrubGeometry = new THREE.IcosahedronGeometry(0.52, 1);
  private readonly rockGeometry = new THREE.DodecahedronGeometry(0.48, 0);

  private readonly trunkMaterial = new THREE.MeshStandardMaterial({
    color: 0x4b3827,
    roughness: 0.98,
    metalness: 0,
  });
  private readonly lowerCrownMaterial = new THREE.MeshStandardMaterial({
    color: 0x264b2a,
    roughness: 0.96,
    metalness: 0,
  });
  private readonly middleCrownMaterial = new THREE.MeshStandardMaterial({
    color: 0x2d5930,
    roughness: 0.95,
    metalness: 0,
  });
  private readonly upperCrownMaterial = new THREE.MeshStandardMaterial({
    color: 0x37683a,
    roughness: 0.94,
    metalness: 0,
  });
  private readonly shrubMaterial = new THREE.MeshStandardMaterial({
    color: 0x415f34,
    roughness: 0.98,
    metalness: 0,
  });
  private readonly rockMaterial = new THREE.MeshStandardMaterial({
    color: 0x6b6b63,
    roughness: 1,
    metalness: 0,
  });

  private readonly trunks = new THREE.InstancedMesh(
    this.trunkGeometry,
    this.trunkMaterial,
    MAX_TREES,
  );
  private readonly lowerCrowns = new THREE.InstancedMesh(
    this.lowerCrownGeometry,
    this.lowerCrownMaterial,
    MAX_TREES,
  );
  private readonly middleCrowns = new THREE.InstancedMesh(
    this.middleCrownGeometry,
    this.middleCrownMaterial,
    MAX_TREES,
  );
  private readonly upperCrowns = new THREE.InstancedMesh(
    this.upperCrownGeometry,
    this.upperCrownMaterial,
    MAX_TREES,
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
  private readonly up = new THREE.Vector3(0, 1, 0);

  constructor() {
    for (const mesh of [
      this.trunks,
      this.lowerCrowns,
      this.middleCrowns,
      this.upperCrowns,
      this.shrubs,
      this.rocks,
    ]) {
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.castShadow = false;
      mesh.receiveShadow = false;
      mesh.frustumCulled = true;
      this.group.add(mesh);
    }
  }

  setTerrain(window: TerrainWindow): void {
    const seed = window.worldSeed ?? '0';
    const sampleElevation = createTerrainElevationSampler(window);
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const halfChunk = chunkWorldSize * 0.5;
    let treeCountTotal = 0;
    let shrubCountTotal = 0;
    let rockCountTotal = 0;

    const groundAt = (
      chunkX: number,
      chunkY: number,
      offsetX: number,
      offsetZ: number,
    ): number => {
      const localX01 = Math.max(
        0,
        Math.min(1, (offsetX + halfChunk) / chunkWorldSize),
      );
      const localY01 = Math.max(
        0,
        Math.min(1, (offsetZ + halfChunk) / chunkWorldSize),
      );
      return sampleElevation(
        chunkX,
        chunkY,
        localX01,
        localY01,
      ) * WORLD_GRID_CONTRACT.elevationScale;
    };

    for (const chunk of window.chunks) {
      const forest = Math.max(
        0,
        Math.min(1, Number(chunk.forestCoverage01) || 0),
      );
      const shrub = Math.max(
        0,
        Math.min(1, Number(chunk.shrubCoverage01) || 0),
      );
      const grass = Math.max(
        0,
        Math.min(1, Number(chunk.grassCoverage01) || 0),
      );
      const rock = Math.max(
        0,
        Math.min(1, Number(chunk.rockCoverage01) || 0),
      );

      const treeCount = forest < 0.1
        ? 0
        : Math.min(12, 1 + Math.floor(forest * 11));
      const shrubCount = Math.min(
        18,
        Math.floor(shrub * 10 + grass * 5 + forest * 4),
      );
      const rockCount = Math.min(8, Math.floor(rock * 8));

      for (
        let index = 0;
        index < treeCount && treeCountTotal < MAX_TREES;
        index += 1
      ) {
        const offsetX = (
          hash01(seed, chunk.x, chunk.y, 1000 + index * 7) - 0.5
        ) * chunkWorldSize * 0.88;
        const offsetZ = (
          hash01(seed, chunk.x, chunk.y, 1001 + index * 7) - 0.5
        ) * chunkWorldSize * 0.88;
        const treeScale = 0.78
          + hash01(seed, chunk.x, chunk.y, 1002 + index * 7) * 0.74;
        const widthScale = 0.78
          + hash01(seed, chunk.x, chunk.y, 1003 + index * 7) * 0.48;
        const asymmetry = (
          hash01(seed, chunk.x, chunk.y, 1004 + index * 7) - 0.5
        ) * 0.45;
        const yaw = hash01(
          seed,
          chunk.x,
          chunk.y,
          1005 + index * 7,
        ) * Math.PI * 2;
        const worldX = (
          chunk.x - window.centerChunkX
        ) * chunkWorldSize + offsetX;
        const worldZ = (
          chunk.y - window.centerChunkY
        ) * chunkWorldSize + offsetZ;
        const groundY = groundAt(
          chunk.x,
          chunk.y,
          offsetX,
          offsetZ,
        );

        this.rotation.setFromAxisAngle(this.up, yaw);

        const trunkHeight = 3.05 * treeScale;
        this.position.set(
          worldX,
          groundY + trunkHeight * 0.5,
          worldZ,
        );
        this.scale.set(
          treeScale * (0.9 + widthScale * 0.1),
          treeScale,
          treeScale * (0.9 + widthScale * 0.1),
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.trunks.setMatrixAt(treeCountTotal, this.matrix);

        const crownBaseY = groundY + trunkHeight - 0.2 * treeScale;
        const crownLayers: Array<{
          mesh: THREE.InstancedMesh;
          y: number;
          x: number;
          z: number;
          sx: number;
          sy: number;
          sz: number;
        }> = [
          {
            mesh: this.lowerCrowns,
            y: crownBaseY + 1.15 * treeScale,
            x: asymmetry * treeScale,
            z: -asymmetry * 0.45 * treeScale,
            sx: 1.7 * treeScale * widthScale,
            sy: 1.28 * treeScale,
            sz: 1.55 * treeScale * widthScale,
          },
          {
            mesh: this.middleCrowns,
            y: crownBaseY + 2.35 * treeScale,
            x: -asymmetry * 0.5 * treeScale,
            z: asymmetry * treeScale,
            sx: 1.45 * treeScale * widthScale,
            sy: 1.22 * treeScale,
            sz: 1.38 * treeScale * widthScale,
          },
          {
            mesh: this.upperCrowns,
            y: crownBaseY + 3.45 * treeScale,
            x: asymmetry * 0.3 * treeScale,
            z: asymmetry * 0.22 * treeScale,
            sx: 1.05 * treeScale * widthScale,
            sy: 1.05 * treeScale,
            sz: 1.02 * treeScale * widthScale,
          },
        ];
        for (const layer of crownLayers) {
          this.position.set(
            worldX + layer.x,
            layer.y,
            worldZ + layer.z,
          );
          this.scale.set(layer.sx, layer.sy, layer.sz);
          this.matrix.compose(this.position, this.rotation, this.scale);
          layer.mesh.setMatrixAt(treeCountTotal, this.matrix);
        }

        treeCountTotal += 1;
      }

      for (
        let index = 0;
        index < shrubCount && shrubCountTotal < MAX_SHRUBS;
        index += 1
      ) {
        const offsetX = (
          hash01(seed, chunk.x, chunk.y, 3000 + index * 5) - 0.5
        ) * chunkWorldSize * 0.92;
        const offsetZ = (
          hash01(seed, chunk.x, chunk.y, 3001 + index * 5) - 0.5
        ) * chunkWorldSize * 0.92;
        const scaleBase = 0.38
          + hash01(seed, chunk.x, chunk.y, 3002 + index * 5) * 0.75;
        const worldX = (
          chunk.x - window.centerChunkX
        ) * chunkWorldSize + offsetX;
        const worldZ = (
          chunk.y - window.centerChunkY
        ) * chunkWorldSize + offsetZ;
        const groundY = groundAt(
          chunk.x,
          chunk.y,
          offsetX,
          offsetZ,
        );
        const yaw = hash01(
          seed,
          chunk.x,
          chunk.y,
          3003 + index * 5,
        ) * Math.PI * 2;
        this.rotation.setFromAxisAngle(this.up, yaw);
        this.position.set(
          worldX,
          groundY + scaleBase * 0.42,
          worldZ,
        );
        this.scale.set(
          scaleBase * 1.35,
          scaleBase * 0.85,
          scaleBase,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.shrubs.setMatrixAt(shrubCountTotal, this.matrix);
        shrubCountTotal += 1;
      }

      for (
        let index = 0;
        index < rockCount && rockCountTotal < MAX_ROCKS;
        index += 1
      ) {
        const offsetX = (
          hash01(seed, chunk.x, chunk.y, 5000 + index * 5) - 0.5
        ) * chunkWorldSize * 0.9;
        const offsetZ = (
          hash01(seed, chunk.x, chunk.y, 5001 + index * 5) - 0.5
        ) * chunkWorldSize * 0.9;
        const scaleBase = 0.34
          + hash01(seed, chunk.x, chunk.y, 5002 + index * 5) * 0.78;
        const worldX = (
          chunk.x - window.centerChunkX
        ) * chunkWorldSize + offsetX;
        const worldZ = (
          chunk.y - window.centerChunkY
        ) * chunkWorldSize + offsetZ;
        const groundY = groundAt(
          chunk.x,
          chunk.y,
          offsetX,
          offsetZ,
        );
        const yaw = hash01(
          seed,
          chunk.x,
          chunk.y,
          5003 + index * 5,
        ) * Math.PI * 2;
        this.rotation.setFromAxisAngle(this.up, yaw);
        this.position.set(
          worldX,
          groundY + scaleBase * 0.32,
          worldZ,
        );
        this.scale.set(
          scaleBase * 1.25,
          scaleBase * 0.65,
          scaleBase,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.rocks.setMatrixAt(rockCountTotal, this.matrix);
        rockCountTotal += 1;
      }
    }

    this.trunks.count = treeCountTotal;
    this.lowerCrowns.count = treeCountTotal;
    this.middleCrowns.count = treeCountTotal;
    this.upperCrowns.count = treeCountTotal;
    this.shrubs.count = shrubCountTotal;
    this.rocks.count = rockCountTotal;

    for (const mesh of [
      this.trunks,
      this.lowerCrowns,
      this.middleCrowns,
      this.upperCrowns,
      this.shrubs,
      this.rocks,
    ]) {
      mesh.instanceMatrix.needsUpdate = true;
    }
  }

  dispose(): void {
    this.trunkGeometry.dispose();
    this.lowerCrownGeometry.dispose();
    this.middleCrownGeometry.dispose();
    this.upperCrownGeometry.dispose();
    this.shrubGeometry.dispose();
    this.rockGeometry.dispose();
    this.trunkMaterial.dispose();
    this.lowerCrownMaterial.dispose();
    this.middleCrownMaterial.dispose();
    this.upperCrownMaterial.dispose();
    this.shrubMaterial.dispose();
    this.rockMaterial.dispose();
  }
}
