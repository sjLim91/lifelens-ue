import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';
import { buildWaterGeometry } from './water-geometry';

const WATER_KINDS = new Set(['Spring', 'Stream', 'River', 'Lake', 'Coast', 'Ocean']);

export class WaterLayer {
  readonly group = new THREE.Group();

  private readonly entries = new Map<string, THREE.Mesh>();

  setTerrain(window: TerrainWindow): void {
    const active = new Set<string>();

    for (const chunk of window.chunks) {
      if (!WATER_KINDS.has(chunk.waterKind)) continue;
      const key = `${chunk.x}:${chunk.y}`;
      active.add(key);

      let mesh = this.entries.get(key);
      if (!mesh) {
        mesh = this.createMesh(chunk, window);
        this.entries.set(key, mesh);
        this.group.add(mesh);
      } else {
        const previous = mesh.geometry;
        mesh.geometry = buildWaterGeometry(chunk, window, 8);
        previous.dispose();
      }

      const size = 8;
      mesh.position.set(
        (chunk.x - window.centerChunkX) * size,
        (chunk.elevation01 * 48) + 0.08,
        (chunk.y - window.centerChunkY) * size,
      );
      mesh.scale.set(1, 1, 1);
    }

    for (const [key, mesh] of this.entries) {
      if (active.has(key)) continue;
      this.group.remove(mesh);
      mesh.geometry.dispose();
      const material = mesh.material;
      if (!Array.isArray(material)) material.dispose();
      this.entries.delete(key);
    }
  }

  dispose(): void {
    for (const mesh of this.entries.values()) {
      mesh.geometry.dispose();
      const material = mesh.material;
      if (!Array.isArray(material)) material.dispose();
    }
    this.entries.clear();
  }

  private createMesh(
    chunk: TerrainChunk,
    window: TerrainWindow,
  ): THREE.Mesh {
    const geometry = buildWaterGeometry(chunk, window, 8);
    const material = new THREE.MeshStandardMaterial({
      color: this.colorFor(chunk),
      transparent: true,
      opacity: chunk.waterKind === 'Ocean' ? 0.86 : 0.76,
      roughness: 0.28,
      metalness: 0.04,
      depthWrite: true,
    });
    return new THREE.Mesh(geometry, material);
  }

  private colorFor(chunk: TerrainChunk): number {
    switch (chunk.waterKind) {
      case 'Ocean': return 0x174b67;
      case 'Coast': return 0x2f7487;
      case 'Lake': return 0x2e7188;
      case 'River': return 0x3f8daa;
      case 'Stream': return 0x55a0ba;
      default: return 0x63acc2;
    }
  }
}
