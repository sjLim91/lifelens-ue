import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createWaterGeometryBuilder } from './water-geometry';

const WATER_KINDS = new Set<WaterKind>([
  'Spring',
  'Stream',
  'River',
  'Lake',
  'Coast',
  'Ocean',
]);

type VisibleWaterKind =
  | 'Spring'
  | 'Stream'
  | 'River'
  | 'Lake'
  | 'Coast'
  | 'Ocean';

interface WaterEntry {
  mesh: THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial>;
  signature: string;
  kind: VisibleWaterKind;
}

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function isVisibleWaterKind(kind: WaterKind): kind is VisibleWaterKind {
  return WATER_KINDS.has(kind);
}

export class WaterLayer {
  readonly group = new THREE.Group();

  private readonly entries = new Map<string, WaterEntry>();
  private wind01 = 0;
  private rain01 = 0;
  private daylight01 = 1;

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
        chunkMap.get(`${chunk.x + dx}:${chunk.y + dy}`)?.waterKind
        ?? 'None'
      ));
      return `${chunk.waterKind}|${neighbors.join(',')}`;
    };

    for (const chunk of window.chunks) {
      if (!isVisibleWaterKind(chunk.waterKind)) continue;

      const key = `${chunk.x}:${chunk.y}`;
      const signature = topologySignature(chunk);
      active.add(key);

      let entry = this.entries.get(key);
      if (!entry) {
        const mesh = this.createMesh(
          chunk.waterKind,
          buildGeometry(chunk),
        );
        entry = {
          mesh,
          signature,
          kind: chunk.waterKind,
        };
        this.entries.set(key, entry);
        this.group.add(mesh);
      } else if (
        entry.signature !== signature
        || entry.kind !== chunk.waterKind
      ) {
        const previous = entry.mesh.geometry;
        entry.mesh.geometry = buildGeometry(chunk);
        previous.dispose();
        entry.kind = chunk.waterKind;
        entry.signature = signature;
      }

      this.applySurfaceState(entry);

      const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      entry.mesh.position.set(
        (chunk.x - window.centerChunkX) * chunkWorldSize,
        (chunk.elevation01 * WORLD_GRID_CONTRACT.elevationScale) + 0.08,
        (chunk.y - window.centerChunkY) * chunkWorldSize,
      );
      entry.mesh.scale.set(1, 1, 1);
    }

    for (const [key, entry] of this.entries) {
      if (active.has(key)) continue;
      this.group.remove(entry.mesh);
      entry.mesh.geometry.dispose();
      entry.mesh.material.dispose();
      this.entries.delete(key);
    }
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    if (!environment || environment.available === false) {
      this.wind01 = 0;
      this.rain01 = 0;
    } else {
      this.wind01 = clamp01(environment.windIntensity01);
      const rainy =
        environment.precipitationType === 'Rain'
        || environment.summary === 'Rain'
        || environment.summary === 'Storm';
      this.rain01 = rainy
        ? clamp01(environment.precipitationIntensity01)
        : 0;
    }

    this.applyAllSurfaceStates();
  }

  setSimulationMinute(minuteValue: number): void {
    const minute = Math.max(0, Number(minuteValue) || 0) % 1440;
    const dayAngle =
      ((minute / 1440) * Math.PI * 2) - (Math.PI * 0.5);
    const solar = Math.sin(dayAngle);
    this.daylight01 = clamp01((solar + 0.22) / 1.22);
    this.applyAllSurfaceStates();
  }

  update(_deltaSeconds: number): void {
    // Deliberately stable for the regression-recovery pass.
    // Reintroduce animation only after a single continuous surface strategy
    // replaces per-chunk transparent/specular shading.
  }

  dispose(): void {
    for (const entry of this.entries.values()) {
      entry.mesh.geometry.dispose();
      entry.mesh.material.dispose();
    }
    this.entries.clear();
  }

  private createMesh(
    kind: VisibleWaterKind,
    geometry: THREE.BufferGeometry,
  ): THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial> {
    const material = new THREE.MeshStandardMaterial({
      color: this.colorFor(kind),
      transparent: false,
      opacity: 1,
      roughness: 0.5,
      metalness: 0.015,
      depthWrite: true,
      depthTest: true,
      dithering: true,
    });

    const mesh = new THREE.Mesh(geometry, material);
    mesh.renderOrder = 1;
    mesh.castShadow = false;
    mesh.receiveShadow = false;
    return mesh;
  }

  private applyAllSurfaceStates(): void {
    for (const entry of this.entries.values()) {
      this.applySurfaceState(entry);
    }
  }

  private applySurfaceState(entry: WaterEntry): void {
    const material = entry.mesh.material;
    const brightness = 0.68 + this.daylight01 * 0.32;

    material.color
      .setHex(this.colorFor(entry.kind))
      .multiplyScalar(brightness);
    material.roughness = THREE.MathUtils.clamp(
      0.48 + this.wind01 * 0.18 + this.rain01 * 0.12,
      0.46,
      0.76,
    );
    material.metalness = 0.015;
  }

  private colorFor(kind: VisibleWaterKind): number {
    switch (kind) {
      case 'Ocean': return 0x174b67;
      case 'Coast': return 0x2f7487;
      case 'Lake': return 0x2e7188;
      case 'River': return 0x3f8daa;
      case 'Stream': return 0x55a0ba;
      default: return 0x63acc2;
    }
  }
}
