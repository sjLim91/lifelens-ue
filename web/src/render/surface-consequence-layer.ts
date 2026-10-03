import * as THREE from 'three';
import type { TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { facilityPresentationFootprints, outsideFacilityFootprints } from './facility-layer';
import { createTerrainElevationSampler } from './terrain-geometry';
import { terrainDressingSignature } from './terrain-dressing-signature';
import { createVisibleWaterFootprintTester } from './water-geometry';
import { clamp01, presentationHash01 } from './environment-surface-presentation';
import { WORLD_PRESENTATION } from './world-presentation-config';

export interface PuddleCandidate { x: number; y: number; z: number; radius: number; phase: number }

/** Cosmetic low-ground candidates from the observed, rendered height field.
 * NOT Core Water: cannot be drunk/washed in, never ResourceNodes, navigation
 * obstacles, contamination authority or saved state. No water quantities inferred.
 */
export function puddleCandidates(terrain: TerrainWindow): PuddleCandidate[] {
  if (!terrain.available) return [];
  const config = WORLD_PRESENTATION.weather;
  const { worldUnitsPerChunk: size, elevationScale } = WORLD_GRID_CONTRACT;
  const chunks = new Map(terrain.chunks.map(chunk => [`${chunk.x}:${chunk.y}`, chunk]));
  const sample = createTerrainElevationSampler(terrain);
  const height = (x: number, z: number): number | null => {
    const gx = x / size + terrain.centerChunkX + 0.5;
    const gz = z / size + terrain.centerChunkY + 0.5;
    const cx = Math.floor(gx), cy = Math.floor(gz);
    const chunk = chunks.get(`${cx}:${cy}`);
    if (!chunk || !Number.isFinite(chunk.elevation01)) return null;
    return sample(cx, cy, gx - cx, gz - cy) * elevationScale;
  };
  const water = createVisibleWaterFootprintTester(terrain);
  const facilities = facilityPresentationFootprints(terrain);
  const result: PuddleCandidate[] = [];
  for (const chunk of terrain.chunks) {
    if (!Number.isFinite(chunk.elevation01) || chunk.waterKind === 'Ocean' || chunk.waterKind === 'Coast') continue;
    if ([-1, 0, 1].some(dx => [-1, 0, 1].some(dy => !Number.isFinite(chunks.get(`${chunk.x + dx}:${chunk.y + dy}`)?.elevation01)))) continue;
    for (let ix = 0; ix < config.puddleSamplesPerAxis; ix++) {
      for (let iz = 0; iz < config.puddleSamplesPerAxis; iz++) {
        const token = `${terrain.worldSeed ?? '0'}:puddle:${chunk.x}:${chunk.y}:${ix}:${iz}`;
        const phase = presentationHash01(token);
        const x = (chunk.x - terrain.centerChunkX - 0.5 + (ix + 0.1 + phase * 0.8) / config.puddleSamplesPerAxis) * size;
        const z = (chunk.y - terrain.centerChunkY - 0.5 + (iz + 0.1 + presentationHash01(token + ':z') * 0.8) / config.puddleSamplesPerAxis) * size;
        const y = height(x, z);
        if (y === null) continue;
        const radius = config.puddleRadius * (0.75 + phase * 0.5);
        if (!outsideFacilityFootprints(x, z, facilities, radius)) continue;
        const neighbors = [[1, 0], [-1, 0], [0, 1], [0, -1]].map(([dx, dz]) => height(x + dx * size * 0.75, z + dz * size * 0.75));
        if (neighbors.some(h => h === null)) continue; // Never extrapolate into missing terrain.
        const heights = neighbors as number[];
        const depression = heights.reduce((sum, h) => sum + h - y, 0) / heights.length;
        const slope = Math.max(...heights.map(h => Math.abs(h - y))) / (size * 0.75);
        if (depression < config.puddleMinDepression || slope > config.puddleMaxSlope) continue;
        // Entire patch must fit dry, valid, locally flat ground, not merely its center.
        const rim = Array.from({ length: config.puddleSegments }, (_, i) => {
          const angle = i / config.puddleSegments * Math.PI * 2;
          return [x + Math.cos(angle) * radius, z + Math.sin(angle) * radius] as const;
        });
        if (water(x, z) || rim.some(([px, pz]) => {
          const h = height(px, pz);
          return h === null || water(px, pz) || Math.abs(h - y) / radius > config.puddleMaxSlope;
        })) continue;
        result.push({ x, y, z, radius, phase });
      }
    }
  }
  // Stable nearest-first admission independent of DTO ordering, bounded to viewport.
  return result.sort((a, b) => (a.x * a.x + a.z * a.z) - (b.x * b.x + b.z * b.z)
    || a.x - b.x || a.z - b.z).slice(0, config.puddleMaxInstances);
}

export class SurfaceConsequenceLayer {
  readonly group = new THREE.Group();
  private readonly geometry = new THREE.BufferGeometry();
  private readonly material = new THREE.MeshStandardMaterial({
    color: WORLD_PRESENTATION.weather.puddleColor,
    roughness: WORLD_PRESENTATION.weather.puddleRoughness, metalness: 0,
    vertexColors: true, transparent: true, depthWrite: false,
    polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1,
  });
  private readonly mesh = new THREE.Mesh(this.geometry, this.material);
  private readonly positions = new Float32Array(WORLD_PRESENTATION.weather.puddleMaxInstances * WORLD_PRESENTATION.weather.puddleSegments * 3 * 3);
  private readonly colors = new Float32Array(WORLD_PRESENTATION.weather.puddleMaxInstances * WORLD_PRESENTATION.weather.puddleSegments * 3 * 4);
  private signature = '';
  candidateCount = 0;

  constructor() {
    this.group.name = 'surface-consequences';
    this.geometry.setAttribute('position', new THREE.BufferAttribute(this.positions, 3));
    this.geometry.setAttribute('color', new THREE.BufferAttribute(this.colors, 4));
    this.geometry.setAttribute('normal', new THREE.BufferAttribute(new Float32Array(this.positions.length), 3));
    for (let i = 1; i < this.positions.length; i += 3) this.geometry.attributes.normal.array[i] = 1;
    this.geometry.setDrawRange(0, 0);
    this.mesh.frustumCulled = false;
    this.mesh.renderOrder = 1;
    this.group.add(this.mesh);
    this.setWetness(0);
  }

  setTerrain(terrain: TerrainWindow): void {
    const signature = String(terrain.available) + terrainDressingSignature(terrain);
    if (signature === this.signature) return;
    this.signature = signature;
    const candidates = puddleCandidates(terrain);
    this.candidateCount = candidates.length;
    const sample = createTerrainElevationSampler(terrain);
    const { worldUnitsPerChunk: size, elevationScale } = WORLD_GRID_CONTRACT;
    const config = WORLD_PRESENTATION.weather;
    let vertex = 0;
    for (const candidate of candidates) {
      const point = (angle: number): [number, number] => {
        const radius = candidate.radius * (0.85 + 0.12 * Math.sin(angle * 3 + candidate.phase * 6));
        return [candidate.x + Math.cos(angle) * radius, candidate.z + Math.sin(angle) * radius];
      };
      for (let i = 0; i < config.puddleSegments; i++) {
        const a = point(i / config.puddleSegments * Math.PI * 2);
        const b = point((i + 1) / config.puddleSegments * Math.PI * 2);
        // Upward triangles, feathered vertex alpha; sampled to actual ground.
        for (const [x, z, alpha] of [[candidate.x, candidate.z, 1], [...b, 0], [...a, 0]]) {
          const gx = x / size + terrain.centerChunkX + 0.5;
          const gz = z / size + terrain.centerChunkY + 0.5;
          const cx = Math.floor(gx), cy = Math.floor(gz);
          this.positions.set([x, sample(cx, cy, gx - cx, gz - cy) * elevationScale + config.surfaceLift, z], vertex * 3);
          this.colors.set([1, 1, 1, alpha], vertex * 4);
          vertex++;
        }
      }
    }
    this.geometry.setDrawRange(0, vertex);
    this.geometry.attributes.position.needsUpdate = true;
    this.geometry.attributes.color.needsUpdate = true;
  }

  setWetness(wetness: number, snowCoverage = 0): void {
    const config = WORLD_PRESENTATION.weather;
    // Snow can visually obscure a cosmetic puddle; no Core freezing/melting state.
    const exposed = 1 - clamp01(snowCoverage / config.snowMaxCoverage);
    this.material.opacity = config.puddleMaxOpacity * exposed * clamp01((clamp01(wetness) - config.puddleWetnessThreshold) / (1 - config.puddleWetnessThreshold));
    this.mesh.visible = this.material.opacity > 0;
  }

  dispose(): void { this.geometry.dispose(); this.material.dispose(); this.group.clear(); }
}
