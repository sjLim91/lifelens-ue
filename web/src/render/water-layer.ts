import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createWaterGeometryBuilder } from './water-geometry';

const WATER_KINDS = new Set(['Spring', 'Stream', 'River', 'Lake', 'Coast', 'Ocean']);

interface WaterEntry {
  mesh: THREE.Mesh;
  signature: string;
}

export class WaterLayer {
  readonly group = new THREE.Group();

  private readonly entries = new Map<string, WaterEntry>();

  setTerrain(window: TerrainWindow): void {
    const active = new Set<string>();
    const buildGeometry = createWaterGeometryBuilder(
      window,
      WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    );
    const chunkMap = new Map(
      window.chunks.map((chunk) => [`${chunk.x}:${chunk.y}`, chunk]),
    );

    const topologySignature = (chunk: TerrainChunk): string => {
      const neighbors = (
        [[1, 0], [-1, 0], [0, 1], [0, -1]] as Array<[number, number]>
      ).map(([dx, dy]) => (
        chunkMap.get(`${chunk.x + dx}:${chunk.y + dy}`)?.waterKind ?? 'None'
      ));
      return `${chunk.waterKind}|${neighbors.join(",")}`;
    };

    for (const chunk of window.chunks) {
      if (!WATER_KINDS.has(chunk.waterKind)) continue;
      const key = `${chunk.x}:${chunk.y}`;
      const signature = topologySignature(chunk);
      active.add(key);

      let entry = this.entries.get(key);
      if (!entry) {
        const mesh = this.createMesh(chunk, buildGeometry);
        entry = { mesh, signature };
        this.entries.set(key, entry);
        this.group.add(mesh);
      } else if (entry.signature !== signature) {
        const previous = entry.mesh.geometry;
        entry.mesh.geometry = buildGeometry(chunk);
        previous.dispose();
        const material = entry.mesh.material;
        if (material instanceof THREE.MeshPhysicalMaterial) {
          material.color.set(this.colorFor(chunk));
          material.opacity = chunk.waterKind === 'Ocean' ? 0.82 : 0.7;
          material.roughness = chunk.waterKind === 'Ocean' ? 0.16 : 0.12;
        }
        entry.signature = signature;
      }

      const size = 8;
      entry.mesh.position.set(
        (chunk.x - window.centerChunkX) * size,
        (chunk.elevation01 * WORLD_GRID_CONTRACT.elevationScale) + 0.08,
        (chunk.y - window.centerChunkY) * size,
      );
      entry.mesh.scale.set(1, 1, 1);
    }

    for (const [key, entry] of this.entries) {
      if (active.has(key)) continue;
      this.group.remove(entry.mesh);
      entry.mesh.geometry.dispose();
      const material = entry.mesh.material;
      if (!Array.isArray(material)) material.dispose();
      this.entries.delete(key);
    }
  }

  dispose(): void {
    for (const entry of this.entries.values()) {
      entry.mesh.geometry.dispose();
      const material = entry.mesh.material;
      if (!Array.isArray(material)) material.dispose();
    }
    this.entries.clear();
  }

  private createMesh(
    chunk: TerrainChunk,
    buildGeometry: (chunk: TerrainChunk) => THREE.BufferGeometry,
  ): THREE.Mesh {
    const geometry = buildGeometry(chunk);
    const material = new THREE.MeshPhysicalMaterial({
      color: this.colorFor(chunk),
      transparent: true,
      opacity: chunk.waterKind === 'Ocean' ? 0.82 : 0.7,
      roughness: chunk.waterKind === 'Ocean' ? 0.16 : 0.12,
      metalness: 0.02,
      clearcoat: 0.42,
      clearcoatRoughness: 0.18,
      reflectivity: 0.62,
      depthWrite: false,
      side: THREE.DoubleSide,
    });
    return new THREE.Mesh(geometry, material);
  }

  private colorFor(chunk: TerrainChunk): number {
    switch (chunk.waterKind) {
      case 'Ocean': return 0x1d536c;
      case 'Coast': return 0x337889;
      case 'Lake': return 0x33768a;
      case 'River': return 0x438eaa;
      case 'Stream': return 0x58a4ba;
      default: return 0x67aec1;
    }
  }
}
