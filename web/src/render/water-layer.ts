import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createWaterGeometryBuilder } from './water-geometry';

const WATER_KINDS = new Set(['Spring', 'Stream', 'River', 'Lake', 'Coast', 'Ocean']);

interface WaterEntry {
  mesh: THREE.Mesh;
  signature: string;
}

interface WaterUniformSet {
  time: { value: number };
  energy: { value: number };
}

export class WaterLayer {
  readonly group = new THREE.Group();

  private readonly entries = new Map<string, WaterEntry>();
  private readonly waterUniforms: WaterUniformSet[] = [];
  private animationTime = 0;
  private weatherEnergy = 0;

  setEnvironment(
    environment: DynamicEnvironment | null,
  ): void {
    const wind = Math.max(
      0,
      Math.min(1, Number(environment?.windIntensity01) || 0),
    );
    const precipitation = Math.max(
      0,
      Math.min(
        1,
        Number(environment?.precipitationIntensity01) || 0,
      ),
    );
    this.weatherEnergy = Math.max(
      0,
      Math.min(1, wind * 0.72 + precipitation * 0.44),
    );
    for (const uniforms of this.waterUniforms) {
      uniforms.energy.value = this.weatherEnergy;
    }
  }

  update(deltaSeconds: number): void {
    this.animationTime += Math.min(
      0.05,
      Math.max(0, deltaSeconds),
    );
    for (const uniforms of this.waterUniforms) {
      uniforms.time.value = this.animationTime;
      uniforms.energy.value = this.weatherEnergy;
    }
  }

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
    this.waterUniforms.length = 0;
  }

  private enableWaterMotion(
    material: THREE.MeshPhysicalMaterial,
    waterKind: WaterKind,
  ): void {
    const amplitudeByKind: Partial<Record<WaterKind, number>> = {
      Ocean: 0.085,
      Coast: 0.062,
      Lake: 0.032,
      River: 0.038,
      Stream: 0.022,
      Spring: 0.015,
      Wetland: 0.012,
    };
    const amplitude = amplitudeByKind[waterKind] ?? 0.02;

    material.onBeforeCompile = (shader) => {
      const uniforms: WaterUniformSet = {
        time: { value: this.animationTime },
        energy: { value: this.weatherEnergy },
      };
      shader.uniforms.uLifeLensWaterTime = uniforms.time;
      shader.uniforms.uLifeLensWaterEnergy = uniforms.energy;
      this.waterUniforms.push(uniforms);

      shader.vertexShader = shader.vertexShader.replace(
        '#include <common>',
        `#include <common>
uniform float uLifeLensWaterTime;
uniform float uLifeLensWaterEnergy;`,
      );
      shader.vertexShader = shader.vertexShader.replace(
        '#include <begin_vertex>',
        `#include <begin_vertex>
float lifeLensWaterPhaseA =
  position.x * 0.47
  + position.z * 0.31
  + uLifeLensWaterTime * 1.45;
float lifeLensWaterPhaseB =
  position.x * -0.29
  + position.z * 0.61
  + uLifeLensWaterTime * 2.05;
float lifeLensWaterMotion =
  sin(lifeLensWaterPhaseA)
  + sin(lifeLensWaterPhaseB) * 0.44;
transformed.y +=
  lifeLensWaterMotion
  * ${amplitude.toFixed(4)}
  * (0.38 + uLifeLensWaterEnergy * 0.72);`,
      );
    };
    material.customProgramCacheKey = () => (
      `lifelens-water-v2-${amplitude.toFixed(4)}`
    );
    material.needsUpdate = true;
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
    this.enableWaterMotion(material, chunk.waterKind);
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
