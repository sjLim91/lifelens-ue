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

  private readonly crownGeometry = new THREE.ConeGeometry(0.72, 3.8, 7);
  private readonly trunkGeometry = new THREE.CylinderGeometry(0.12, 0.17, 1.7, 6);
  private readonly crownMaterial = new THREE.MeshStandardMaterial({
    color: 0x214b27,
    roughness: 0.94,
    metalness: 0,
  });
  private readonly trunkMaterial = new THREE.MeshStandardMaterial({
    color: 0x4b3926,
    roughness: 0.96,
    metalness: 0,
  });
  private readonly crowns = new THREE.InstancedMesh(
    this.crownGeometry,
    this.crownMaterial,
    MAX_TREES,
  );
  private readonly trunks = new THREE.InstancedMesh(
    this.trunkGeometry,
    this.trunkMaterial,
    MAX_TREES,
  );
  private readonly matrix = new THREE.Matrix4();
  private readonly rotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();
  private readonly position = new THREE.Vector3();

  constructor() {
    this.crowns.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
    this.trunks.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
    this.crowns.castShadow = false;
    this.crowns.receiveShadow = false;
    this.trunks.castShadow = false;
    this.trunks.receiveShadow = false;
    this.group.add(this.trunks);
    this.group.add(this.crowns);
  }

  setTerrain(window: TerrainWindow): void {
    const seed = window.worldSeed ?? '0';
    let count = 0;

    for (const chunk of window.chunks) {
      const forest = Math.max(
        0,
        Math.min(1, Number(chunk.forestCoverage01) || 0),
      );
      const treeCount = forest < 0.12
        ? 0
        : Math.min(9, 1 + Math.floor(forest * 8));

      for (let index = 0; index < treeCount && count < MAX_TREES; index += 1) {
        const offsetX = (hash01(seed, chunk.x, chunk.y, index * 2) - 0.5) * 6.5;
        const offsetZ = (hash01(seed, chunk.x, chunk.y, index * 2 + 1) - 0.5) * 6.5;
        const treeScale = 0.82 + hash01(seed, chunk.x, chunk.y, index + 19) * 0.68;
        const widthScale = 0.82 + hash01(seed, chunk.x, chunk.y, index + 31) * 0.36;
        const yaw = hash01(seed, chunk.x, chunk.y, index + 47) * Math.PI * 2;
        const worldX = (chunk.x - window.centerChunkX) * 8 + offsetX;
        const worldZ = (chunk.y - window.centerChunkY) * 8 + offsetZ;
        const groundY = chunk.elevation01 * 48;
        const trunkHeight = 1.7 * treeScale;
        const crownHeight = 3.8 * treeScale;

        this.rotation.setFromAxisAngle(
          new THREE.Vector3(0, 1, 0),
          yaw,
        );

        this.position.set(
          worldX,
          groundY + (trunkHeight * 0.5),
          worldZ,
        );
        this.scale.set(
          treeScale * widthScale,
          treeScale,
          treeScale * widthScale,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.trunks.setMatrixAt(count, this.matrix);

        this.position.set(
          worldX,
          groundY + trunkHeight + (crownHeight * 0.5) - (0.35 * treeScale),
          worldZ,
        );
        this.scale.set(
          treeScale * widthScale,
          treeScale,
          treeScale * widthScale,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.crowns.setMatrixAt(count, this.matrix);

        count += 1;
      }
    }

    this.crowns.count = count;
    this.trunks.count = count;
    this.crowns.instanceMatrix.needsUpdate = true;
    this.trunks.instanceMatrix.needsUpdate = true;
  }

  dispose(): void {
    this.crownGeometry.dispose();
    this.trunkGeometry.dispose();
    this.crownMaterial.dispose();
    this.trunkMaterial.dispose();
  }
}
