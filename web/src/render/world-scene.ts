import * as THREE from 'three';
import type {
  CivilizationWorldPayload,
  DynamicEnvironment,
  Resident,
  TerrainChunk,
  TerrainWindow,
  WorldObjectsPayload,
} from '../runtime/core-types';
import {
  OBSERVER_CAMERA_CONTRACT,
  WORLD_GRID_CONTRACT,
} from '../runtime/lifelens-contract';
import { AtmosphereLayer } from './atmosphere-layer';
import { AuthoritativeSpatialTargetLayer } from './authoritative-spatial-target-layer';
import { GroundDetailLayer } from './ground-detail-layer';
import { FacilityLayer } from './facility-layer';
import { HumanTraceLayer } from './human-trace-layer';
import { ResidentWorldLayer } from './resident-world-layer';
import {
  createTerrainElevationSampler,
  createTerrainGeometryBuilder,
} from './terrain-geometry';
import { VegetationLayer } from './vegetation-layer';
import { WaterLayer } from './water-layer';
import { createTerrainSurfaceSignature } from './terrain-surface';
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
}

export class WorldScene {
  readonly scene = new THREE.Scene();
  readonly camera = new THREE.PerspectiveCamera(42, 1, 0.1, 5000);

  private readonly terrainGroup = new THREE.Group();
  private readonly terrainMeshes = new Map<string, TerrainMeshEntry>();
  private readonly groundDetailLayer = new GroundDetailLayer();
  private readonly authoritativeSpatialTargetLayer =
    new AuthoritativeSpatialTargetLayer();
  private readonly facilityLayer = new FacilityLayer();
  private readonly humanTraceLayer = new HumanTraceLayer();
  private readonly waterLayer = new WaterLayer();
  private readonly vegetationLayer = new VegetationLayer();
  private readonly residentLayer = new ResidentWorldLayer();
  private readonly weatherLayer = new WeatherLayer();
  private readonly raycaster = new THREE.Raycaster();
  private readonly pointer = new THREE.Vector2();
  private readonly atmosphere: AtmosphereLayer;
  private surfaceWetness01 = 0;
  private cameraInitialized = false;
  private terrainWorldSeed: string | undefined;
  private facilityDressingSignature = '';
  private sampleGroundHeight: (x: number, z: number) => number = () => 0;
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
    this.scene.add(this.authoritativeSpatialTargetLayer.group);
    this.scene.add(this.facilityLayer.group);
    this.scene.add(this.humanTraceLayer.group);
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
    const current = this.currentCameraState;
    const originChanged = current.centerChunkX !== state.centerChunkX
      || current.centerChunkY !== state.centerChunkY;
    if (this.cameraInitialized && originChanged) {
      // Terrain/residents have already moved into the new local origin. Rebase
      // the in-flight camera by the same amount before smoothing its new goal.
      current.panX = (current.panX ?? 0)
        + (current.centerChunkX - state.centerChunkX) * WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      current.panZ = (current.panZ ?? 0)
        + (current.centerChunkY - state.centerChunkY) * WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      current.centerChunkX = state.centerChunkX;
      current.centerChunkY = state.centerChunkY;
    }
    this.desiredCameraState = {
      ...state,
      panX: Number(state.panX) || 0,
      panZ: Number(state.panZ) || 0,
    };

    if (!this.cameraInitialized) {
      this.currentCameraState = { ...this.desiredCameraState };
      this.cameraInitialized = true;
      this.applyCamera(this.currentCameraState);
    } else if (originChanged) {
      this.applyCamera(current);
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

  pickHumanTrace(normalizedX: number, normalizedY: number): string | null {
    this.camera.updateMatrixWorld();
    this.pointer.set(normalizedX, normalizedY);
    this.raycaster.setFromCamera(this.pointer, this.camera);
    this.facilityLayer.group.updateMatrixWorld(true);
    const facility = this.facilityLayer.pickTrace(this.raycaster);
    if (facility) return facility;
    this.humanTraceLayer.group.updateMatrixWorld(true);
    return this.humanTraceLayer.pickTrace(this.raycaster);
  }

  setSelectedHumanTrace(id: string | null): void {
    this.humanTraceLayer.setSelectedTrace(id);
  }

  setAuthoritativeSpatialTargets(
    civilization: CivilizationWorldPayload,
    worldObjects: WorldObjectsPayload,
    terrain: TerrainWindow,
  ): void {
    this.authoritativeSpatialTargetLayer.setTargets(
      civilization,
      worldObjects,
      terrain,
    );
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

  private facilityTraceSignature(window: TerrainWindow): string {
    return (window.humanTraces?.entries ?? [])
      .filter((trace) => trace.kind === 'Facility')
      .map((trace) => [
        trace.id,
        trace.gridX,
        trace.gridY,
        trace.facilityKind,
      ].join(':'))
      .sort()
      .join('|');
  }

  private applyCamera(state: WorldSceneCameraState): void {
    const distance = 180 / Math.max(0.35, state.zoom);
    const elevation = Math.max(0.12, Math.min(1.35, state.elevation));
    const groundRadius = Math.cos(elevation) * distance;
    const panX = Number(state.panX) || 0;
    const panZ = Number(state.panZ) || 0;
    const focusHeight = this.sampleGroundHeight(panX, panZ);
    const cameraX = Math.cos(state.angle) * groundRadius + panX;
    const cameraZ = Math.sin(state.angle) * groundRadius + panZ;

    this.camera.position.set(
      cameraX,
      Math.max(
        focusHeight + Math.sin(elevation) * distance,
        this.sampleGroundHeight(cameraX, cameraZ) + 1.6,
      ),
      cameraZ,
    );
    this.camera.lookAt(panX, focusHeight, panZ);
    this.weatherLayer.setFocus(panX, panZ);
  }

  setHumanTraces(window: TerrainWindow): void {
    const facilitySignature = this.facilityTraceSignature(window);
    this.facilityLayer.setTerrain(window);
    this.humanTraceLayer.setDynamicTraces(window);

    // Trees/grass/rocks only need an expensive placement rebuild when the
    // actual facility footprint set changes. Ordinary residue/resource-use
    // trace refreshes stay on the lightweight path.
    if (facilitySignature !== this.facilityDressingSignature) {
      this.facilityDressingSignature = facilitySignature;
      this.groundDetailLayer.setTerrain(window);
      this.vegetationLayer.setTerrain(window);
    }
  }

  setTerrain(window: TerrainWindow): void {
    if (
      this.terrainWorldSeed !== undefined
      && window.worldSeed !== undefined
      && this.terrainWorldSeed !== window.worldSeed
    ) {
      this.cameraInitialized = false;
    }
    this.terrainWorldSeed = window.worldSeed;
    const sampleElevation = createTerrainElevationSampler(window);
    const chunkSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    this.sampleGroundHeight = (x, z) => {
      // Terrain meshes are centered on their chunk; grid-cell positions start
      // at its negative edge. Use that same half-chunk offset for the camera.
      const chunkOffsetX = Math.floor(x / chunkSize + 0.5);
      const chunkOffsetY = Math.floor(z / chunkSize + 0.5);
      return sampleElevation(
        window.centerChunkX + chunkOffsetX,
        window.centerChunkY + chunkOffsetY,
        x / chunkSize - chunkOffsetX + 0.5,
        z / chunkSize - chunkOffsetY + 0.5,
      ) * WORLD_GRID_CONTRACT.elevationScale;
    };
    const active = new Set<string>();
    const buildGeometry = createTerrainGeometryBuilder(window, {
      chunkWorldSize: WORLD_GRID_CONTRACT.worldUnitsPerChunk,
      elevationScale: WORLD_GRID_CONTRACT.elevationScale,
    });
    const terrainSignature = createTerrainSurfaceSignature(window);

    this.waterLayer.setTerrain(window);
    this.facilityDressingSignature = this.facilityTraceSignature(window);
    this.groundDetailLayer.setTerrain(window);
    this.facilityLayer.setTerrain(window);
    this.humanTraceLayer.setTerrain(window);
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
          this.applyTerrainWeather(existing.mesh.material);
          existing.signature = signature;
        }
        continue;
      }

      const mesh = this.createTerrainMesh(
        chunk,
        window,
        buildGeometry,
      );
      this.terrainMeshes.set(
        key,
        { mesh, key, signature },
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
    this.authoritativeSpatialTargetLayer.dispose();
    this.facilityLayer.dispose();
    this.humanTraceLayer.dispose();
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
  ): THREE.Mesh<THREE.BufferGeometry, THREE.MeshStandardMaterial> {
    const geometry = buildGeometry(chunk);

    const material = new THREE.MeshStandardMaterial({
      color: 0xffffff,
      roughness: 0.92,
      metalness: 0,
      vertexColors: true,
    });
    this.applyTerrainWeather(material);

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
      this.applyTerrainWeather(entry.mesh.material);
    }
  }

  private applyTerrainWeather(
    material: THREE.MeshStandardMaterial,
  ): void {
    const wetness = this.surfaceWetness01;
    material.color
      .setHex(0xffffff)
      .multiplyScalar(1 - wetness * 0.3);
    material.roughness = Math.max(
      0.42,
      0.92 - wetness * 0.46,
    );
  }

}
