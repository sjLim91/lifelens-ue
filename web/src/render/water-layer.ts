import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainWindow,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import {
  buildFlowWaterSurfaceGeometry,
  buildOpenWaterSurfaceGeometry,
} from './water-geometry';

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

export class WaterLayer {
  readonly group = new THREE.Group();

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
    side: THREE.FrontSide,
  });
  private readonly openWaterMesh = new THREE.Mesh(
    new THREE.BufferGeometry(),
    this.openWaterMaterial,
  );

  private readonly flowWaterMaterial = new THREE.MeshStandardMaterial({
    color: 0xffffff,
    vertexColors: true,
    transparent: false,
    opacity: 1,
    roughness: 0.6,
    metalness: 0.005,
    depthWrite: true,
    depthTest: true,
    dithering: true,
    side: THREE.FrontSide,
  });
  private readonly flowWaterMesh = new THREE.Mesh(
    new THREE.BufferGeometry(),
    this.flowWaterMaterial,
  );

  private surfaceSignature = '';
  private wind01 = 0;
  private rain01 = 0;
  private daylight01 = 1;

  constructor() {
    for (const mesh of [this.openWaterMesh, this.flowWaterMesh]) {
      mesh.renderOrder = 1;
      mesh.castShadow = false;
      mesh.receiveShadow = false;
      this.group.add(mesh);
    }
  }

  setTerrain(window: TerrainWindow): void {
    const signature = [
      window.worldSeed ?? '0',
      window.centerChunkX,
      window.centerChunkY,
      ...window.chunks.map((chunk) => (
        `${chunk.x}:${chunk.y}:${chunk.waterKind}:${Number(
          chunk.elevation01,
        ).toFixed(5)}`
      )).sort(),
    ].join('|');

    if (signature === this.surfaceSignature) {
      return;
    }

    const previousOpen = this.openWaterMesh.geometry;
    const previousFlow = this.flowWaterMesh.geometry;

    this.openWaterMesh.geometry = buildOpenWaterSurfaceGeometry(
      window,
      WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    );
    this.flowWaterMesh.geometry = buildFlowWaterSurfaceGeometry(
      window,
      WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    );

    previousOpen.dispose();
    previousFlow.dispose();
    this.surfaceSignature = signature;
    this.applyAllSurfaceStates();
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
    // Keep water geometry stable. Motion returns only as a single continuous
    // field over the reconstructed surfaces, never per source chunk.
  }

  dispose(): void {
    this.openWaterMesh.geometry.dispose();
    this.flowWaterMesh.geometry.dispose();
    this.openWaterMaterial.dispose();
    this.flowWaterMaterial.dispose();
  }

  private applyAllSurfaceStates(): void {
    const openBrightness = 0.68 + this.daylight01 * 0.32;
    this.openWaterMaterial.color.setScalar(openBrightness);
    this.openWaterMaterial.roughness = THREE.MathUtils.clamp(
      0.54 + this.wind01 * 0.16 + this.rain01 * 0.12,
      0.5,
      0.78,
    );

    const flowBrightness = 0.72 + this.daylight01 * 0.28;
    this.flowWaterMaterial.color.setScalar(flowBrightness);
    this.flowWaterMaterial.roughness = THREE.MathUtils.clamp(
      0.58 + this.wind01 * 0.12 + this.rain01 * 0.1,
      0.54,
      0.78,
    );
  }
}
