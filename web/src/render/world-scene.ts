import * as THREE from 'three';
import type {
  DynamicEnvironment,
  Resident,
  TerrainChunk,
  TerrainWindow,
  WorldPresentationSnapshot,
} from '../runtime/core-types';
import {
  OBSERVER_CAMERA_CONTRACT,
  WORLD_GRID_CONTRACT,
} from '../runtime/lifelens-contract';
import { AtmosphereLayer } from './atmosphere-layer';
import { ResidentWorldLayer } from './resident-world-layer';
import { createTerrainGeometryBuilder } from './terrain-geometry';
import { VegetationLayer } from './vegetation-layer';
import { WaterLayer } from './water-layer';
import { WeatherLayer } from './weather-layer';
import { WorldConsequenceLayer } from './world-consequence-layer';

export interface WorldSceneCameraState {
  centerChunkX: number;
  centerChunkY: number;
  zoom: number;
  angle: number;
  elevation: number;
  panX?: number;
  panY?: number;
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
  private readonly waterLayer = new WaterLayer();
  private readonly vegetationLayer = new VegetationLayer();
  private readonly residentLayer = new ResidentWorldLayer();
  private readonly consequenceLayer = new WorldConsequenceLayer();
  private readonly weatherLayer = new WeatherLayer();
  private readonly raycaster = new THREE.Raycaster();
  private readonly pointer = new THREE.Vector2();
  private readonly atmosphere: AtmosphereLayer;
  private environment: DynamicEnvironment | null = null;
  private cameraInitialized = false;
  private currentCameraState: WorldSceneCameraState = {
    centerChunkX: 0,
    centerChunkY: 0,
    zoom: OBSERVER_CAMERA_CONTRACT.defaultDesktopZoom,
    angle: OBSERVER_CAMERA_CONTRACT.defaultAngleRadians,
    elevation: OBSERVER_CAMERA_CONTRACT.defaultElevationRadians,
    panX: 0,
    panY: 0,
    panZ: 0,
  };
  private desiredCameraState: WorldSceneCameraState = {
    ...this.currentCameraState,
  };

  constructor() {
    this.scene.add(this.terrainGroup);
    this.scene.add(this.waterLayer.group);
    this.scene.add(this.vegetationLayer.group);
    this.scene.add(this.consequenceLayer.group);
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
      panY: Number(state.panY) || 0,
      panZ: Number(state.panZ) || 0,
    };
    this.consequenceLayer.setCameraZoom(this.desiredCameraState.zoom);

    if (!this.cameraInitialized) {
      this.currentCameraState = { ...this.desiredCameraState };
      this.cameraInitialized = true;
      this.applyCamera(this.currentCameraState);
    }
  }

  setSimulationMinute(minute: number): void {
    this.atmosphere.setSimulationMinute(minute);
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.environment = environment;
    this.atmosphere.setEnvironment(environment);
    this.weatherLayer.setEnvironment(environment);
    this.waterLayer.setEnvironment(environment);
    this.consequenceLayer.setEnvironment(environment);
    this.vegetationLayer.setWindIntensity(
      Number(environment?.windIntensity01) || 0,
    );
    for (const entry of this.terrainMeshes.values()) {
      this.applyTerrainWeatherMaterial(entry.mesh.material);
    }
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

  setWorldPresentation(
    snapshot: WorldPresentationSnapshot | null,
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    this.consequenceLayer.setSnapshot(
      snapshot,
      terrain,
      centerX,
      centerY,
    );
    this.vegetationLayer.setPresentation(snapshot, terrain);
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
    this.consequenceLayer.setCameraZoom(current.zoom);
    current.panX = (Number(current.panX) || 0)
      + ((Number(desired.panX) || 0) - (Number(current.panX) || 0)) * t;
    current.panY = (Number(current.panY) || 0)
      + ((Number(desired.panY) || 0) - (Number(current.panY) || 0)) * t;
    current.panZ = (Number(current.panZ) || 0)
      + ((Number(desired.panZ) || 0) - (Number(current.panZ) || 0)) * t;
    this.applyCamera(current);

    this.residentLayer.update(deltaSeconds);
    this.consequenceLayer.update(deltaSeconds);
    this.vegetationLayer.update(deltaSeconds);
    this.waterLayer.update(deltaSeconds);
    this.atmosphere.update(deltaSeconds);
    this.weatherLayer.update(deltaSeconds);
  }

  private applyCamera(state: WorldSceneCameraState): void {
    const distance = 180 / Math.max(0.35, state.zoom);
    const elevation = Math.max(0.12, Math.min(1.35, state.elevation));
    const groundRadius = Math.cos(elevation) * distance;
    const panX = Number(state.panX) || 0;
    const panY = Number(state.panY) || 0;
    const panZ = Number(state.panZ) || 0;

    this.weatherLayer.setAnchor(panX, panY, panZ);

    this.camera.position.set(
      Math.cos(state.angle) * groundRadius + panX,
      Math.sin(elevation) * distance + panY,
      Math.sin(state.angle) * groundRadius + panZ,
    );
    this.camera.lookAt(panX, panY, panZ);
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
          this.applyTerrainVertexColors(existing.mesh.geometry, chunk);
          previousGeometry.dispose();
          existing.signature = signature;
        }
        continue;
      }

      const mesh = this.createTerrainMesh(chunk, window, buildGeometry);
      this.terrainMeshes.set(key, { mesh, key, signature });
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
    this.waterLayer.dispose();
    this.vegetationLayer.dispose();
    this.consequenceLayer.dispose();
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
    this.applyTerrainVertexColors(geometry, chunk);

    const material = new THREE.MeshStandardMaterial({
      color: 0xffffff,
      vertexColors: true,
      roughness: 0.96,
      metalness: 0,
    });

    this.applyTerrainWeatherMaterial(material);

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

  private applyTerrainWeatherMaterial(
    material: THREE.MeshStandardMaterial,
  ): void {
    const precipitation = Math.max(
      0,
      Math.min(1, Number(this.environment?.precipitationIntensity01) || 0),
    );
    const cloud = Math.max(
      0,
      Math.min(1, Number(this.environment?.cloudCover01) || 0),
    );
    const snow = this.environment?.precipitationType === 'Snow';
    const rain = this.environment?.precipitationType === 'Rain';

    const tint = new THREE.Color(0xffffff);
    if (snow && precipitation > 0) {
      tint.lerp(
        new THREE.Color(0xdce6e7),
        Math.min(0.52, precipitation * 0.48),
      );
    } else if (rain && precipitation > 0) {
      tint.lerp(
        new THREE.Color(0xd3ddd8),
        Math.min(0.22, precipitation * 0.2),
      );
    }
    if (cloud > 0.45) {
      tint.multiplyScalar(1 - (cloud - 0.45) * 0.045);
    }

    material.color.copy(tint);
    material.roughness = rain
      ? Math.max(0.7, 0.96 - precipitation * 0.2)
      : snow
        ? 0.92
        : 0.96;
    material.needsUpdate = true;
  }

  private applyTerrainVertexColors(
    geometry: THREE.BufferGeometry,
    chunk: TerrainChunk,
  ): void {
    const position = geometry.getAttribute('position');
    if (!(position instanceof THREE.BufferAttribute)) return;

    const base = new THREE.Color(this.terrainColor(chunk));
    const forest = Math.max(
      0,
      Math.min(1, Number(chunk.forestCoverage01) || 0),
    );
    const grass = Math.max(
      0,
      Math.min(1, Number(chunk.grassCoverage01) || 0),
    );
    const rock = Math.max(
      0,
      Math.min(1, Number(chunk.rockCoverage01) || 0),
    );
    const wetland = Math.max(
      0,
      Math.min(1, Number(chunk.wetlandCoverage01) || 0),
    );
    const rockTone = new THREE.Color(0x7a766b);
    const wetTone = new THREE.Color(0x405d4b);
    const grassTone = new THREE.Color(0x68784a);
    const forestTone = new THREE.Color(0x31513a);
    const color = new THREE.Color();
    const colors = new Float32Array(position.count * 3);

    for (let index = 0; index < position.count; index += 1) {
      const x = position.getX(index);
      const y = position.getY(index);
      const z = position.getZ(index);
      const worldNoise = Math.sin(
        (chunk.x * 9.37 + x * 0.41)
        + (chunk.y * 7.13 + z * 0.53),
      ) * 0.5 + Math.sin(
        (chunk.x * 3.17 - z * 0.27)
        + (chunk.y * 5.91 + x * 0.31),
      ) * 0.5;
      const height01 = Math.max(
        0,
        Math.min(1, y / Math.max(0.001, WORLD_GRID_CONTRACT.elevationScale)),
      );

      color.copy(base);
      color.lerp(forestTone, forest * 0.28);
      color.lerp(grassTone, grass * 0.18);
      color.lerp(wetTone, wetland * 0.32);
      color.lerp(rockTone, rock * (0.12 + height01 * 0.24));
      color.offsetHSL(
        worldNoise * 0.012,
        worldNoise * 0.018,
        worldNoise * 0.028 + (height01 - 0.5) * 0.018,
      );

      colors[index * 3] = color.r;
      colors[index * 3 + 1] = color.g;
      colors[index * 3 + 2] = color.b;
    }

    geometry.setAttribute(
      'color',
      new THREE.BufferAttribute(colors, 3),
    );
  }

  private terrainColor(chunk: TerrainChunk): number {
    switch (chunk.waterKind) {
      case 'Ocean': return 0x1b4b63;
      case 'Coast': return 0x276878;
      case 'Wetland': return 0x506b55;
      default:
        if (chunk.elevation01 < 0.34) return 0x3f5a3d;
        if (chunk.elevation01 < 0.48) return 0x506b41;
        if (chunk.elevation01 < 0.62) return 0x65764c;
        if (chunk.elevation01 < 0.76) return 0x77765b;
        return 0x92907d;
    }
  }
}
