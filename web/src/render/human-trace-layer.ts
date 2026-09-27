import * as THREE from 'three';
import type { HumanTrace, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { visibleHumanTraces } from '../state/human-traces';
import { createTerrainElevationSampler } from './terrain-geometry';

// Surface marks plus a selected-facility halo. Facilities themselves are rendered
// by FacilityLayer; unselected facilities must never fall back to placeholder rings.
// All surface traces share one draw call, and every vertex follows the real terrain.
export function buildHumanTraceGeometry(
  terrain: TerrainWindow, traces: HumanTrace[], selectedId: string | null = null,
) {
  const positions: number[] = [], colors: number[] = [], indices: number[] = [];
  const triangleTraceIds: string[] = [];
  const sample = createTerrainElevationSampler(terrain);
  const { gridCellsPerChunk: span, worldUnitsPerChunk: size, elevationScale } = WORLD_GRID_CONTRACT;
  const segments = 24;
  const displayTraces = traces.filter(
    (trace) => trace.kind !== 'Facility' || trace.id === selectedId,
  );
  for (const trace of displayTraces) {
    const selected = trace.id === selectedId;
    const resourceUse = trace.kind === 'ResourceUse'
      ? 1 - trace.quantity / trace.baselineQuantity : 0;
    const radius = trace.kind === 'Residue'
      ? Math.min(1.3, 0.45 + trace.radiusTiles * size / span * 0.25)
      : trace.kind === 'Facility' ? 1.2 : 0.65 + resourceUse * 0.5;
    const tint = new THREE.Color(selected ? 0xffe4a0
      : trace.kind === 'Facility' ? 0x97c9c2 : trace.kind === 'Residue' ? 0x604532 : 0x927a48);
    const strength = trace.kind === 'Residue' ? trace.intensity : trace.kind === 'ResourceUse' ? resourceUse : 0.7;
    const cx = (trace.gridX / span - terrain.centerChunkX - 0.5) * size;
    const cz = (trace.gridY / span - terrain.centerChunkY - 0.5) * size;
    const start = positions.length / 3;
    // Deterministic shape variation is cosmetic; positions/presence stay Core-owned.
    let phase = 0;
    for (const letter of trace.id) phase = (phase * 31 + letter.charCodeAt(0)) >>> 0;
    for (let ring = 0; ring < 4; ring++) {
      const fraction = [0, 0.52, 0.84, 1][ring];
      const alpha = trace.kind === 'Facility'
        ? [0, 0, 0.75, 0][ring]
        : [0.65, 0.55, 0.24, 0][ring];
      for (let step = 0; step < segments; step++) {
        const angle = step / segments * Math.PI * 2;
        const irregularity = trace.kind === 'Facility' ? 1
          : 1 + 0.12 * Math.sin(angle * 5 + phase) + 0.07 * Math.cos(angle * 3 + phase);
        const x = cx + Math.cos(angle) * radius * fraction * irregularity;
        const z = cz + Math.sin(angle) * radius * fraction * irregularity;
        const chunkX = terrain.centerChunkX + Math.floor(x / size + 0.5);
        const chunkY = terrain.centerChunkY + Math.floor(z / size + 0.5);
        const height = sample(chunkX, chunkY,
          x / size + terrain.centerChunkX - chunkX + 0.5,
          z / size + terrain.centerChunkY - chunkY + 0.5) * elevationScale;
        positions.push(x, height + 0.045, z);
        colors.push(tint.r, tint.g, tint.b, alpha * (selected ? 1 : 0.4 + strength * 0.55));
      }
    }
    for (let ring = 0; ring < 3; ring++) {
      for (let step = 0; step < segments; step++) {
        const next = (step + 1) % segments;
        const a = start + ring * segments + step;
        const b = start + ring * segments + next;
        const c = start + (ring + 1) * segments + step;
        const d = start + (ring + 1) * segments + next;
        // X/Z winding faces upward, including sloping/negative-coordinate terrain.
        indices.push(a, b, c, b, d, c);
        triangleTraceIds.push(trace.id, trace.id);
      }
    }
  }
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
  geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 4));
  geometry.setIndex(indices);
  geometry.computeBoundingSphere();
  return { geometry, triangleTraceIds };
}

export class HumanTraceLayer {
  readonly group = new THREE.Group();
  private readonly material = new THREE.MeshBasicMaterial({
    vertexColors: true, transparent: true, depthWrite: false,
    polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1,
  });
  private readonly mesh = new THREE.Mesh(new THREE.BufferGeometry(), this.material);
  private triangleTraceIds: string[] = [];
  private signature = '';
  private terrain: TerrainWindow | null = null;
  private selectedId: string | null = null;

  constructor() {
    this.group.name = 'human-traces';
    this.mesh.renderOrder = 2;
    this.group.add(this.mesh);
  }

  setTerrain(terrain: TerrainWindow): void {
    this.terrain = terrain;
    const traces = visibleHumanTraces(terrain);
    const signature = JSON.stringify([
      terrain.worldSeed, terrain.centerChunkX, terrain.centerChunkY, this.selectedId,
      terrain.chunks.map(chunk => [chunk.x, chunk.y, chunk.elevation01]),
      traces.map(trace => trace.kind === 'Residue'
        ? { ...trace, amount: Math.round(trace.amount * 10), intensity: Math.round(trace.intensity * 100) }
        : trace),
    ]);
    if (signature === this.signature) return;
    this.signature = signature;
    const displayTraces = traces.filter(
      (trace) => trace.kind !== 'Facility' || trace.id === this.selectedId,
    );
    const next = buildHumanTraceGeometry(terrain, displayTraces, this.selectedId);
    this.mesh.geometry.dispose();
    this.mesh.geometry = next.geometry;
    this.triangleTraceIds = next.triangleTraceIds;
    this.mesh.visible = displayTraces.length > 0;
  }

  setSelectedTrace(id: string | null): void {
    if (this.selectedId === id) return;
    this.selectedId = id;
    if (this.terrain) this.setTerrain(this.terrain);
  }

  pickTrace(raycaster: THREE.Raycaster): string | null {
    if (!this.mesh.visible) return null;
    const hit = raycaster.intersectObject(this.mesh)[0];
    return hit?.faceIndex != null ? this.triangleTraceIds[hit.faceIndex] ?? null : null;
  }

  dispose(): void {
    this.mesh.geometry.dispose();
    this.material.dispose();
    this.group.clear();
    this.terrain = null;
    this.triangleTraceIds = [];
  }
}
