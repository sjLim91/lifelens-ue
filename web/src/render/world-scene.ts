import * as THREE from 'three';
import type {
  DynamicEnvironment,
  Resident,
  TerrainChunk,
  TerrainWindow,
} from '../runtime/core-types';
import {
  OBSERVER_CAMERA_CONTRACT,
  WORLD_GRID_CONTRACT,
} from '../runtime/lifelens-contract';
import { AtmosphereLayer } from './atmosphere-layer';
import { GroundDetailLayer } from './ground-detail-layer';
import { ResidentWorldLayer } from './resident-world-layer';
import { createTerrainGeometryBuilder } from './terrain-geometry';
import { VegetationLayer } from './vegetation-layer';
import { WaterLayer } from './water-layer';
import { WeatherLayer } from './weather-layer';

export interface WorldSceneCameraState {
  centerChunkX: number;
  centerChunkY: number;
  zoom: number;
  angle: number;
  elevation: number;
  panX?: number;
  panZ?: number;
}

interface TerrainMeshEntry {
  mesh: THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial>;
  key: string;
  signature: string;
  baseColor: number;
}

export class WorldScene {
  readonly scene = new THREE.Scene();
  readonly camera = new THREE.PerspectiveCamera(42, 1, 0.1, 5000);

  private readonly terrainGroup = new THREE.Group();
  private readonly terrainMeshes = new Map<string, TerrainMeshEntry>();
  private readonly groundDetailLayer = new GroundDetailLayer();
  private readonly waterLayer = new WaterLayer();
  private readonly vegetationLayer = new VegetationLayer();
  private readonly residentLayer = new ResidentWorldLayer();
  private readonly weatherLayer = new WeatherLayer();
  private readonly raycaster = new THREE.Raycaster();
  private readonly pointer = new THREE.Vector2();
  private readonly atmosphere: AtmosphereLayer;
  private surfaceWetness01 = 0;
  private cameraInitialized = false;
  private currentCameraState: WorldSceneCameraState = {
    centerChunkX: 0,
    centerChunkY: 0,
    zoom: OBSERVER_CAMERA_CONTRACT.defaultDesktopZoom,
    angle: OBSERVER_CAMERA_CONTRACT.defaultAngleRadians,
    elevation: OBSERVER_CAMERA_CONTRACT.defaultElevationRadians,
    panX: 0,
    panZ: 0,
  };
  private desiredCameraState: WorldSceneCameraState = {
    ...this.currentCameraState,
  };

  constructor() {
    this.scene.add(this.terrainGroup);
    this.scene.add(this.groundDetailLayer.group);
    this.scene.add(this.waterLayer.group);
    this.scene.add(this.vegetationLayer.group);
    this.scene.add(this.residentLayer.group);
    this.scene.add(this.weatherLayer.group);
    this.atmosphere = new AtmosphereLayer(this.scene);

    this.camera.position.set(0, 160, 180);
    this.camera.lookAt(0, 0, 0);
  }

  resize(width: number, height: number): void {
    this.camera.aspect = Math.max(1, width) / Math.max(1, height);
    this.camera.updateProjectionMatrix();
  }

  setCamera(state: WorldSceneCameraState): void {
    this.desiredCameraState = {
      ...state,
      panX: Number(state.panX) || 0,
      panZ: Number(state.panZ) || 0,
    };

    if (!this.cameraInitialized) {
      this.currentCameraState = { ...this.desiredCameraState };
      this.cameraInitialized = true;
      this.applyCamera(this.currentCameraState);
    }
  }

  setSimulationMinute(minute: number): void {
    this.atmosphere.setSimulationMinute(minute);
    this.waterLayer.setSimulationMinute(minute);
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.atmosphere.setEnvironment(environment);
    this.weatherLayer.setEnvironment(environment);
    this.waterLayer.setEnvironment(environment);

    const precipitation = Math.max(
      0,
      Math.min(1, Number(environment?.precipitationIntensity01) || 0),
    );
    const summaryFloor = environment?.summary === 'Storm'
      ? 0.78
      : environment?.summary === 'Rain'
        ? 0.5
        : environment?.summary === 'Snow'
          ? 0.24
          : 0;
    const wetness = Math.max(
      Math.max(
        0,
        Math.min(1, Number(environment?.surfaceWetness01) || 0),
      ),
      precipitation * 0.76,
      summaryFloor,
    );
    this.surfaceWetness01 = Math.max(
      0,
      Math.min(1, wetness),
    );
    this.updateTerrainWeather();
    this.groundDetailLayer.setWetness(this.surfaceWetness01);
  }

  setSimulationSpeed(speed: number): void {
    this.residentLayer.setSimulationSpeed(speed);
  }

  pickResident(normalizedX: number, normalizedY: number): string | null {
    this.pointer.set(normalizedX, normalizedY);
    this.raycaster.setFromCamera(this.pointer, this.camera);
    return this.residentLayer.pickResident(this.raycaster);
  }

  setSelectedResident(residentId: string | null): void {
    this.residentLayer.setSelectedResident(residentId);
  }

  setResidents(
    residents: Resident[],
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    this.residentLayer.setResidents(
      residents,
      terrain,
      centerX,
      centerY,
    );
  }

  update(deltaSeconds: number): void {
    const t = 1 - Math.exp(-Math.max(0, deltaSeconds) * 9);
    const current = this.currentCameraState;
    const desired = this.desiredCameraState;

    current.angle += (desired.angle - current.angle) * t;
    current.elevation += (desired.elevation - current.elevation) * t;
    current.zoom += (desired.zoom - current.zoom) * t;
    current.panX = (Number(current.panX) || 0)
      + ((Number(desired.panX) || 0) - (Number(current.panX) || 0)) * t;
    current.panZ = (Number(current.panZ) || 0)
      + ((Number(desired.panZ) || 0) - (Number(current.panZ) || 0)) * t;
    this.applyCamera(current);

    this.residentLayer.update(deltaSeconds);
    this.waterLayer.update(deltaSeconds);
    this.weatherLayer.update(deltaSeconds);
  }

  private applyCamera(state: WorldSceneCameraState): void {
    const distance = 180 / Math.max(0.35, state.zoom);
    const elevation = Math.max(0.12, Math.min(1.35, state.elevation));
    const groundRadius = Math.cos(elevation) * distance;
    const panX = Number(state.panX) || 0;
    const panZ = Number(state.panZ) || 0;

    this.camera.position.set(
      Math.cos(state.angle) * groundRadius + panX,
      Math.sin(elevation) * distance,
      Math.sin(state.angle) * groundRadius + panZ,
    );
    this.camera.lookAt(panX, 0, panZ);
    this.weatherLayer.setFocus(panX, panZ);
  }

  setTerrain(window: TerrainWindow): void {
    const active = new Set<string>();
    const buildGeometry = createTerrainGeometryBuilder(window, {
      chunkWorldSize: WORLD_GRID_CONTRACT.worldUnitsPerChunk,
      elevationScale: WORLD_GRID_CONTRACT.elevationScale,
    });
    const chunkMap = new Map(
      window.chunks.map((chunk) => [`${chunk.x}:${chunk.y}`, chunk]),
    );
    const terrainSignature = (chunk: TerrainChunk): string => {
      const neighborhood: string[] = [];
      for (let dy = -1; dy <= 1; dy += 1) {
        for (let dx = -1; dx <= 1; dx += 1) {
          const neighbor = chunkMap.get(
            `${chunk.x + dx}:${chunk.y + dy}`,
          );
          neighborhood.push(
            neighbor
              ? (Number(neighbor.elevation01) || 0).toFixed(5)
              : 'x',
          );
        }
      }
      return `${window.worldSeed ?? '0'}|${chunk.waterKind}|${neighborhood.join(",")}`;
    };

    this.waterLayer.setTerrain(window);
    this.groundDetailLayer.setTerrain(window);
    this.vegetationLayer.setTerrain(window);

    for (const chunk of window.chunks) {
      const key = this.chunkKey(chunk);
      const signature = terrainSignature(chunk);
      active.add(key);
      const existing = this.terrainMeshes.get(key);
      if (existing) {
        this.positionTerrainMesh(existing.mesh, chunk, window);
        if (existing.signature !== signature) {
          const previousGeometry = existing.mesh.geometry;
          existing.mesh.geometry = buildGeometry(chunk);
          previousGeometry.dispose();
          const baseColor = this.terrainColor(chunk);
          existing.baseColor = baseColor;
          this.applyTerrainWeather(existing.mesh.material, baseColor);
          existing.signature = signature;
        }
        continue;
      }

      const baseColor = this.terrainColor(chunk);
      const mesh = this.createTerrainMesh(
        chunk,
        window,
        buildGeometry,
        baseColor,
      );
      this.terrainMeshes.set(
        key,
        { mesh, key, signature, baseColor },
      );
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
    this.groundDetailLayer.dispose();
    this.waterLayer.dispose();
    this.vegetationLayer.dispose();
    this.residentLayer.dispose();
    this.weatherLayer.dispose();
    this.atmosphere.dispose();
  }

  private chunkKey(chunk: TerrainChunk): string {
    return `${chunk.x}:${chunk.y}`;
  }

  private createTerrainMesh(
    chunk: TerrainChunk,
    window: TerrainWindow,
    buildGeometry: (chunk: TerrainChunk) => THREE.BufferGeometry,
    baseColor: number,
  ): THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial> {
    const geometry = buildGeometry(chunk);

    const material = new THREE.MeshStandardMaterial({
      color: baseColor,
      roughness: 0.92,
      metalness: 0,
      vertexColors: true,
    });
    this.applyTerrainWeather(material, baseColor);

    const mesh = new THREE.Mesh(geometry, material);
    mesh.receiveShadow = true;
    this.positionTerrainMesh(mesh, chunk, window);
    return mesh;
  }

  private positionTerrainMesh(
    mesh: THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial>,
    chunk: TerrainChunk,
    window: TerrainWindow,
  ): void {
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    mesh.position.set(
      (chunk.x - window.centerChunkX) * chunkWorldSize,
      0,
      (chunk.y - window.centerChunkY) * chunkWorldSize,
    );
    mesh.scale.set(1, 1, 1);
  }

  private updateTerrainWeather(): void {
    for (const entry of this.terrainMeshes.values()) {
      this.applyTerrainWeather(entry.mesh.material, entry.baseColor);
    }
  }

  private applyTerrainWeather(
    material: THREE.MeshStandardMaterial,
    baseColor: number,
  ): void {
    const wetness = this.surfaceWetness01;
    material.color
      .set(baseColor)
      .multiplyScalar(1 - wetness * 0.3);
    material.roughness = Math.max(
      0.42,
      0.92 - wetness * 0.46,
    );
  }

  private terrainColor(chunk: TerrainChunk): number {
    switch (chunk.waterKind) {
      case 'Ocean': return 0x1b4b63;
      case 'Coast': return 0x276878;
      case 'Wetland': return 0x496e58;
      default:
        break;
    }

    const elevation = Math.max(
      0,
      Math.min(1, Number(chunk.elevation01) || 0),
    );
    const grass = Math.max(
      0,
      Math.min(1, Number(chunk.grassCoverage01) || 0),
    );
    const forest = Math.max(
      0,
      Math.min(1, Number(chunk.forestCoverage01) || 0),
    );
    const rock = Math.max(
      0,
      Math.min(1, Number(chunk.rockCoverage01) || 0),
    );
    const moisture = Math.max(
      0,
      Math.min(1, Number(chunk.moisture01) || 0),
    );

    const base = new THREE.Color(
      elevation < 0.34
        ? 0x425339
        : elevation < 0.5
          ? 0x566246
          : elevation < 0.68
            ? 0x6f6d52
            : 0x898476,
    );
    const green = new THREE.Color(0x3f5f35);
    const earth = new THREE.Color(0x6a5a45);
    const stone = new THREE.Color(0x77766d);

    base.lerp(green, Math.min(0.42, grass * 0.28 + forest * 0.16));
    base.lerp(earth, Math.min(0.2, (1 - moisture) * 0.14));
    base.lerp(stone, Math.min(0.38, rock * 0.36));
    base.multiplyScalar(0.92 + moisture * 0.08);
    return base.getHex();
  }
}
