import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createWaterGeometryBuilder } from './water-geometry';
import {
  configureWaterSurfaceEnvironment,
  configureWaterSurfaceKind,
  createWaterSurfaceMaterial,
} from './water-surface-material';

const WATER_KINDS = new Set<WaterKind>([
  'Spring',
  'Stream',
  'River',
  'Lake',
  'Wetland',
  'Coast',
  'Ocean',
]);

type VisibleWaterKind = Exclude<WaterKind, 'None'>;

interface WaterEntry {
  mesh: THREE.Mesh<THREE.BufferGeometry, THREE.ShaderMaterial>;
  signature: string;
  kind: VisibleWaterKind;
}

interface WaterEnvironmentState {
  wind01: number;
  rain01: number;
  daylight01: number;
}

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function isVisibleWaterKind(kind: WaterKind): kind is VisibleWaterKind {
  return kind !== 'None' && WATER_KINDS.has(kind);
}

export class WaterLayer {
  readonly group = new THREE.Group();

  private readonly entries = new Map<string, WaterEntry>();
  private elapsedSeconds = 0;
  private environment: WaterEnvironmentState = {
    wind01: 0,
    rain01: 0,
    daylight01: 1,
  };

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
      ).map(([dx, dy]) => {
        const neighbor = chunkMap.get(`${chunk.x + dx}:${chunk.y + dy}`);
        return neighbor
          ? `${neighbor.waterKind}:${Number(neighbor.elevation01).toFixed(5)}`
          : 'None:x';
      });
      return [
        chunk.waterKind,
        Number(chunk.elevation01).toFixed(5),
        ...neighbors,
      ].join('|');
    };

    for (const chunk of window.chunks) {
      if (!isVisibleWaterKind(chunk.waterKind)) continue;

      const key = `${chunk.x}:${chunk.y}`;
      const signature = topologySignature(chunk);
      active.add(key);

      let entry = this.entries.get(key);
      if (!entry) {
        const mesh = this.createMesh(chunk.waterKind, buildGeometry(chunk));
        entry = {
          mesh,
          signature,
          kind: chunk.waterKind,
        };
        this.entries.set(key, entry);
        this.group.add(mesh);
      } else if (entry.signature !== signature) {
        const previousGeometry = entry.mesh.geometry;
        entry.mesh.geometry = buildGeometry(chunk);
        previousGeometry.dispose();

        if (entry.kind !== chunk.waterKind) {
          entry.kind = chunk.waterKind;
          configureWaterSurfaceKind(
            entry.mesh.material,
            chunk.waterKind,
          );
        }

        entry.signature = signature;
      }

      this.configureFlow(
        entry.mesh.material,
        chunk,
        chunkMap,
      );
      configureWaterSurfaceEnvironment(
        entry.mesh.material,
        this.environment,
      );
      entry.mesh.material.uniforms.uTime.value = this.elapsedSeconds;

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
    const observation = environment?.available === false
      ? null
      : environment;
    const intensity = clamp01(
      observation?.precipitationIntensity01,
    );
    const precipitationLooksWet =
      observation?.precipitationType === 'Rain'
      || observation?.summary === 'Rain'
      || observation?.summary === 'Storm';

    this.environment.wind01 = clamp01(
      observation?.windIntensity01,
    );
    this.environment.rain01 = precipitationLooksWet
      ? intensity
      : 0;
    this.applyEnvironment();
  }

  setSimulationMinute(minuteValue: number): void {
    const minute = Math.max(0, Number(minuteValue) || 0) % 1440;
    const dayAngle =
      ((minute / 1440) * Math.PI * 2) - (Math.PI * 0.5);
    const solar = Math.sin(dayAngle);
    this.environment.daylight01 = clamp01(
      (solar + 0.18) / 1.18,
    );
    this.applyEnvironment();
  }

  update(deltaSeconds: number): void {
    this.elapsedSeconds += Math.min(
      0.05,
      Math.max(0, deltaSeconds),
    );

    for (const entry of this.entries.values()) {
      entry.mesh.material.uniforms.uTime.value = this.elapsedSeconds;
    }
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
  ): THREE.Mesh<THREE.BufferGeometry, THREE.ShaderMaterial> {
    const material = createWaterSurfaceMaterial(kind);
    configureWaterSurfaceEnvironment(material, this.environment);
    material.uniforms.uTime.value = this.elapsedSeconds;

    const mesh = new THREE.Mesh(geometry, material);
    mesh.renderOrder = 2;
    mesh.castShadow = false;
    mesh.receiveShadow = false;
    return mesh;
  }

  private configureFlow(
    material: THREE.ShaderMaterial,
    chunk: TerrainChunk,
    chunkMap: Map<string, TerrainChunk>,
  ): void {
    const flow = material.uniforms.uFlow.value as THREE.Vector2;

    if (
      chunk.waterKind === 'Ocean'
      || chunk.waterKind === 'Coast'
      || chunk.waterKind === 'Lake'
      || chunk.waterKind === 'Wetland'
    ) {
      flow.set(0, 0);
      return;
    }

    const neighbors = (
      [[1, 0], [-1, 0], [0, 1], [0, -1]] as Array<[number, number]>
    )
      .map(([dx, dy]) => ({
        dx,
        dy,
        chunk: chunkMap.get(`${chunk.x + dx}:${chunk.y + dy}`),
      }))
      .filter((candidate) => (
        candidate.chunk
        && isVisibleWaterKind(candidate.chunk.waterKind)
      ));

    if (neighbors.length === 0) {
      flow.set(0, 0);
      return;
    }

    neighbors.sort((a, b) => (
      Number(a.chunk?.elevation01 ?? 1)
      - Number(b.chunk?.elevation01 ?? 1)
    ));
    flow.set(neighbors[0].dx, neighbors[0].dy).normalize();
  }

  private applyEnvironment(): void {
    for (const entry of this.entries.values()) {
      configureWaterSurfaceEnvironment(
        entry.mesh.material,
        this.environment,
      );
    }
  }
}
