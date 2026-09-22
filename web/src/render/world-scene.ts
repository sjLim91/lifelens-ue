import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';

export interface WorldSceneCameraState {
  centerChunkX: number;
  centerChunkY: number;
  zoom: number;
  angle: number;
}

interface TerrainMeshEntry {
  mesh: THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial>;
  key: string;
}

export class WorldScene {
  readonly scene = new THREE.Scene();
  readonly camera = new THREE.PerspectiveCamera(42, 1, 0.1, 5000);

  private readonly terrainGroup = new THREE.Group();
  private readonly terrainMeshes = new Map<string, TerrainMeshEntry>();

  constructor() {
    this.scene.background = new THREE.Color(0x08100b);
    this.scene.add(this.terrainGroup);

    const hemi = new THREE.HemisphereLight(0xf5f6ef, 0x29352e, 1.6);
    this.scene.add(hemi);

    const sun = new THREE.DirectionalLight(0xffffff, 1.8);
    sun.position.set(-80, 160, 110);
    this.scene.add(sun);

    this.camera.position.set(0, 160, 180);
    this.camera.lookAt(0, 0, 0);
  }

  resize(width: number, height: number): void {
    this.camera.aspect = Math.max(1, width) / Math.max(1, height);
    this.camera.updateProjectionMatrix();
  }

  setCamera(state: WorldSceneCameraState): void {
    const distance = 180 / Math.max(0.35, state.zoom);
    const horizontal = Math.cos(state.angle) * distance;
    const depth = Math.sin(state.angle) * distance;

    this.camera.position.set(horizontal, distance * 0.8, depth);
    this.camera.lookAt(0, 0, 0);
  }

  setTerrain(window: TerrainWindow): void {
    const active = new Set<string>();

    for (const chunk of window.chunks) {
      const key = this.chunkKey(chunk);
      active.add(key);
      const existing = this.terrainMeshes.get(key);
      if (existing) {
        this.updateTerrainMesh(existing.mesh, chunk, window);
        continue;
      }

      const mesh = this.createTerrainMesh(chunk, window);
      this.terrainMeshes.set(key, { mesh, key });
      this.terrainGroup.add(mesh);
    }

    for (const [key, entry] of this.terrainMeshes) {
      if (active.has(key)) continue;
      this.terrainGroup.remove(entry.mesh);
      entry.mesh.geometry.dispose();
      entry.mesh.material.dispose();
      this.terrainMeshes.delete(key);
    }
  }

  dispose(): void {
    for (const entry of this.terrainMeshes.values()) {
      entry.mesh.geometry.dispose();
      entry.mesh.material.dispose();
    }
    this.terrainMeshes.clear();
  }

  private chunkKey(chunk: TerrainChunk): string {
    return `${chunk.x}:${chunk.y}`;
  }

  private createTerrainMesh(
    chunk: TerrainChunk,
    window: TerrainWindow,
  ): THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial> {
    const geometry = new THREE.PlaneGeometry(1, 1, 1, 1);
    geometry.rotateX(-Math.PI / 2);

    const material = new THREE.MeshStandardMaterial({
      color: this.terrainColor(chunk),
      roughness: 0.92,
      metalness: 0,
    });

    const mesh = new THREE.Mesh(geometry, material);
    this.updateTerrainMesh(mesh, chunk, window);
    return mesh;
  }

  private updateTerrainMesh(
    mesh: THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial>,
    chunk: TerrainChunk,
    window: TerrainWindow,
  ): void {
    const chunkWorldSize = 8;
    const elevationScale = 48;

    mesh.position.set(
      (chunk.x - window.centerChunkX) * chunkWorldSize,
      chunk.elevation01 * elevationScale,
      (chunk.y - window.centerChunkY) * chunkWorldSize,
    );
    mesh.scale.set(chunkWorldSize, chunkWorldSize, chunkWorldSize);
    mesh.material.color.set(this.terrainColor(chunk));
  }

  private terrainColor(chunk: TerrainChunk): number {
    switch (chunk.waterKind) {
      case 'Ocean': return 0x1b4b63;
      case 'Coast': return 0x276878;
      case 'Wetland': return 0x496e58;
      default:
        if (chunk.elevation01 < 0.34) return 0x294b31;
        if (chunk.elevation01 < 0.48) return 0x3b6439;
        if (chunk.elevation01 < 0.62) return 0x64794a;
        if (chunk.elevation01 < 0.76) return 0x807d5c;
        return 0xaaa78f;
    }
  }
}
