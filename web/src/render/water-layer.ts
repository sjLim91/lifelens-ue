import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import {
  buildOpenWaterSurfaceGeometry,
  createWaterGeometryBuilder,
} from './water-geometry';

const FLOW_WATER_KINDS = new Set<WaterKind>([
  'Spring',
  'Stream',
  'River',
]);

type VisibleFlowKind = 'Spring' | 'Stream' | 'River';

interface WaterEntry {
  mesh: THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial>;
  signature: string;
  kind: VisibleFlowKind;
}

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function isVisibleFlowKind(kind: WaterKind): kind is VisibleFlowKind {
  return FLOW_WATER_KINDS.has(kind);
}

export class WaterLayer {
  readonly group = new THREE.Group();

  private readonly entries = new Map<string, WaterEntry>();
  private readonly openWaterMaterial = new THREE.MeshStandardMaterial({
    color: 0xffffff,
    vertexColors: true,
    transparent: false,
    opacity: 1,
    roughness: 0.56,
    metalness: 0.01,
    depthWrite: true,
    depthTest: true,
    dithering: true,
    side: THREE.DoubleSide,
  });
  private readonly openWaterMesh = new THREE.Mesh(
    new THREE.BufferGeometry(),
    this.openWaterMaterial,
  );

  private openSignature = '';
  private wind01 = 0;
  private rain01 = 0;
  private daylight01 = 1;

  constructor() {
    this.openWaterMesh.renderOrder = 1;
    this.openWaterMesh.castShadow = false;
    this.openWaterMesh.receiveShadow = false;
    this.group.add(this.openWaterMesh);
  }

  setTerrain(window: TerrainWindow): void {
    const surfaceSignature = [
      window.centerChunkX,
      window.centerChunkY,
      ...window.chunks
        .filter((chunk) => (
          chunk.waterKind === 'Ocean'
          || chunk.waterKind === 'Coast'
          || chunk.waterKind === 'Lake'
        ))
        .map((chunk) => (
          `${chunk.x}:${chunk.y}:${chunk.waterKind}:${Number(
            chunk.elevation01,
          ).toFixed(5)}`
        ))
        .sort(),
    ].join('|');

    if (surfaceSignature !== this.openSignature) {
      const previous = this.openWaterMesh.geometry;
      this.openWaterMesh.geometry = buildOpenWaterSurfaceGeometry(
        window,
        WORLD_GRID_CONTRACT.worldUnitsPerChunk,
      );
      previous.dispose();
      this.openSignature = surfaceSignature;
    }

    const active = new Set<string>();
    const buildFlowGeometry = createWaterGeometryBuilder(
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
      if (!isVisibleFlowKind(chunk.waterKind)) continue;

      const key = `${chunk.x}:${chunk.y}`;
      const signature = topologySignature(chunk);
      active.add(key);

      let entry = this.entries.get(key);
      if (!entry) {
        const mesh = this.createFlowMesh(
          chunk.waterKind,
          buildFlowGeometry(chunk),
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
        entry.mesh.geometry = buildFlowGeometry(chunk);
        previous.dispose();
        entry.kind = chunk.waterKind;
        entry.signature = signature;
      }

      this.applyFlowSurfaceState(entry);

      const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      entry.mesh.position.set(
        (chunk.x - window.centerChunkX) * chunkWorldSize,
        (chunk.elevation01 * WORLD_GRID_CONTRACT.elevationScale) + 0.09,
        (chunk.y - window.centerChunkY) * chunkWorldSize,
      );
    }

    for (const [key, entry] of this.entries) {
      if (active.has(key)) continue;
      this.group.remove(entry.mesh);
      entry.mesh.geometry.dispose();
      entry.mesh.material.dispose();
      this.entries.delete(key);
    }

    this.applyOpenSurfaceState();
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
    // Intentionally stable. Animation returns only after it can run over the
    // connected surface as one field instead of exposing source chunks.
  }

  dispose(): void {
    this.openWaterMesh.geometry.dispose();
    this.openWaterMaterial.dispose();

    for (const entry of this.entries.values()) {
      entry.mesh.geometry.dispose();
      entry.mesh.material.dispose();
    }
    this.entries.clear();
  }

  private createFlowMesh(
    kind: VisibleFlowKind,
    geometry: THREE.BufferGeometry,
  ): THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial> {
    const material = new THREE.MeshStandardMaterial({
      color: this.colorForFlow(kind),
      transparent: false,
      opacity: 1,
      roughness: 0.58,
      metalness: 0.01,
      depthWrite: true,
      depthTest: true,
      dithering: true,
      side: THREE.DoubleSide,
    });

    const mesh = new THREE.Mesh(geometry, material);
    mesh.renderOrder = 1;
    mesh.castShadow = false;
    mesh.receiveShadow = false;
    return mesh;
  }

  private applyAllSurfaceStates(): void {
    this.applyOpenSurfaceState();
    for (const entry of this.entries.values()) {
      this.applyFlowSurfaceState(entry);
    }
  }

  private applyOpenSurfaceState(): void {
    const brightness = 0.68 + this.daylight01 * 0.32;
    this.openWaterMaterial.color.setScalar(brightness);
    this.openWaterMaterial.roughness = THREE.MathUtils.clamp(
      0.54 + this.wind01 * 0.16 + this.rain01 * 0.12,
      0.5,
      0.78,
    );
  }

  private applyFlowSurfaceState(entry: WaterEntry): void {
    const brightness = 0.7 + this.daylight01 * 0.3;
    entry.mesh.material.color
      .setHex(this.colorForFlow(entry.kind))
      .multiplyScalar(brightness);
    entry.mesh.material.roughness = THREE.MathUtils.clamp(
      0.56 + this.wind01 * 0.14 + this.rain01 * 0.12,
      0.52,
      0.78,
    );
  }

  private colorForFlow(kind: VisibleFlowKind): number {
    switch (kind) {
      case 'River': return 0x3f8daa;
      case 'Stream': return 0x55a0ba;
      default: return 0x63acc2;
    }
  }
}
