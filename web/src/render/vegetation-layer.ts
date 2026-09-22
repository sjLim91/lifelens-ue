import * as THREE from 'three';
import type { TerrainWindow } from '../runtime/core-types';

const MAX_TREES = 4096;

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

  private readonly treeGeometry = new THREE.ConeGeometry(0.42, 2.1, 6);
  private readonly treeMaterial = new THREE.MeshStandardMaterial({
    color: 0x214b27,
    roughness: 0.94,
    metalness: 0,
  });
  private readonly trees = new THREE.InstancedMesh(
    this.treeGeometry,
    this.treeMaterial,
    MAX_TREES,
  );
  private readonly matrix = new THREE.Matrix4();

  constructor() {
    this.trees.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
    this.trees.castShadow = false;
    this.trees.receiveShadow = false;
    this.group.add(this.trees);
  }

  setTerrain(window: TerrainWindow): void {
    const seed = window.worldSeed ?? '0';
    let count = 0;

    for (const chunk of window.chunks) {
      const forest = Math.max(
        0,
        Math.min(1, Number(chunk.forestCoverage01) || 0),
      );
      const treeCount = forest < 0.12 ? 0 : Math.min(5, 1 + Math.floor(forest * 5));

      for (let index = 0; index < treeCount && count < MAX_TREES; index += 1) {
        const offsetX = (hash01(seed, chunk.x, chunk.y, index * 2) - 0.5) * 6.2;
        const offsetZ = (hash01(seed, chunk.x, chunk.y, index * 2 + 1) - 0.5) * 6.2;
        const scale = 0.72 + hash01(seed, chunk.x, chunk.y, index + 19) * 0.72;
        const worldX = (chunk.x - window.centerChunkX) * 8 + offsetX;
        const worldZ = (chunk.y - window.centerChunkY) * 8 + offsetZ;
        const worldY = chunk.elevation01 * 48 + scale;

        this.matrix.compose(
          new THREE.Vector3(worldX, worldY, worldZ),
          new THREE.Quaternion(),
          new THREE.Vector3(scale, scale, scale),
        );
        this.trees.setMatrixAt(count, this.matrix);
        count += 1;
      }
    }

    this.trees.count = count;
    this.trees.instanceMatrix.needsUpdate = true;
  }

  dispose(): void {
    this.treeGeometry.dispose();
    this.treeMaterial.dispose();
  }
}
