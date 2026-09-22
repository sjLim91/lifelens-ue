import * as THREE from 'three';
import type {
  DynamicEnvironment,
  Resident,
  TerrainChunk,
  TerrainWindow,
} from '../runtime/core-types';
import { AtmosphereLayer } from './atmosphere-layer';
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
  private readonly waterLayer = new WaterLayer();
  private readonly vegetationLayer = new VegetationLayer();
  private readonly residentLayer = new ResidentWorldLayer();
  private readonly weatherLayer = new WeatherLayer();
  private readonly raycaster = new THREE.Raycaster();
  private readonly pointer = new THREE.Vector2();
  private readonly atmosphere: AtmosphereLayer;

  constructor() {
    this.scene.add(this.terrainGroup);
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
    const distance = 180 / Math.max(0.35, state.zoom);
    const horizontal = Math.cos(state.angle) * distance;
    const depth = Math.sin(state.angle) * distance;

    const panX = Number(state.panX) || 0;
    const panZ = Number(state.panZ) || 0;

    this.camera.position.set(
      horizontal + panX,
      distance * 0.8,
      depth + panZ,
    );
    this.camera.lookAt(panX, 0, panZ);
  }

  setSimulationMinute(minute: number): void {
    this.atmosphere.setSimulationMinute(minute);
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.atmosphere.setEnvironment(environment);
    this.weatherLayer.setEnvironment(environment);
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
    this.residentLayer.update(deltaSeconds);
    this.weatherLayer.update(deltaSeconds);
  }

  setTerrain(window: TerrainWindow): void {
    const active = new Set<string>();
    const buildGeometry = createTerrainGeometryBuilder(window, {
      chunkWorldSize: 8,
      elevationScale: 48,
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
          previousGeometry.dispose();
          existing.mesh.material.color.set(this.terrainColor(chunk));
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
      color: this.terrainColor(chunk),
      roughness: 0.92,
      metalness: 0,
    });

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
    const chunkWorldSize = 8;
    mesh.position.set(
      (chunk.x - window.centerChunkX) * chunkWorldSize,
      0,
      (chunk.y - window.centerChunkY) * chunkWorldSize,
    );
    mesh.scale.set(1, 1, 1);
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
