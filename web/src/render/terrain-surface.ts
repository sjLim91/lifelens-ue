import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';

const clamp01 = (value: unknown): number => Math.max(0, Math.min(1, Number(value) || 0));
const smooth = (t: number): number => t * t * (3 - 2 * t);
const key = (x: number, y: number): string => `${x}:${y}`;
const ELEVATION_PALETTE = [
  [0.25, 0x425339], [0.42, 0x566246], [0.59, 0x6f6d52], [0.77, 0x898476],
] as const;

/** Actual 3D surface palette, in Three's linear working color space. */
export function terrainSurfaceColor(chunk: TerrainChunk): THREE.Color {
  if (chunk.waterKind === 'Ocean') return new THREE.Color(0x1b4b63);
  if (chunk.waterKind === 'Coast') return new THREE.Color(0x66705a);
  if (chunk.waterKind === 'Wetland') return new THREE.Color(0x496e58);
  const elevation = clamp01(chunk.elevation01);
  const base = new THREE.Color(ELEVATION_PALETTE[0][1]);
  for (let i = 1; i < ELEVATION_PALETTE.length; i += 1) {
    const [low, from] = ELEVATION_PALETTE[i - 1];
    const [high, to] = ELEVATION_PALETTE[i];
    base.setHex(from).lerp(new THREE.Color(to), smooth(clamp01((elevation - low) / (high - low))));
    if (elevation <= high) break;
  }
  const moisture = clamp01(chunk.moisture01);
  base.lerp(new THREE.Color(0x3f5f35), Math.min(0.42,
    clamp01(chunk.grassCoverage01) * 0.28 + clamp01(chunk.forestCoverage01) * 0.16));
  base.lerp(new THREE.Color(0x6a5a45), Math.min(0.2, (1 - moisture) * 0.14));
  base.lerp(new THREE.Color(0x77766d), Math.min(0.38, clamp01(chunk.rockCoverage01) * 0.36));
  return base.multiplyScalar(0.92 + moisture * 0.08);
}

/** Interpolate observed chunk-center colors; never generate extra terrain. */
export function createTerrainColorSampler(window: TerrainWindow) {
  const colors = new Map(window.chunks.map(chunk => [key(chunk.x, chunk.y), terrainSurfaceColor(chunk)]));
  return (chunkX: number, chunkY: number, tx: number, ty: number, target: THREE.Color): THREE.Color => {
    const x = chunkX + tx - 0.5;
    const y = chunkY + ty - 0.5;
    const ix = Math.floor(x), iy = Math.floor(y);
    const fx = smooth(x - ix), fy = smooth(y - iy);
    target.setRGB(0, 0, 0);
    let weight = 0;
    for (let dy = 0; dy <= 1; dy += 1) {
      for (let dx = 0; dx <= 1; dx += 1) {
        const color = colors.get(key(ix + dx, iy + dy));
        const w = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
        if (!color || w === 0) continue;
        target.r += color.r * w; target.g += color.g * w; target.b += color.b * w;
        weight += w;
      }
    }
    return weight > 0 ? target.multiplyScalar(1 / weight) : target.copy(colors.get(key(chunkX, chunkY)) ?? new THREE.Color(0));
  };
}

/** Normals at an edge depend on both adjacent patches and their corners. */
export function createTerrainSurfaceSignature(window: TerrainWindow) {
  const signatures = new Map(window.chunks.map(chunk => [key(chunk.x, chunk.y), JSON.stringify([
    chunk.elevation01, chunk.waterKind, chunk.grassCoverage01,
    chunk.forestCoverage01, chunk.rockCoverage01, chunk.moisture01,
  ])]));
  return (chunk: TerrainChunk): string => {
    const neighbors: string[] = [];
    for (let dy = -2; dy <= 2; dy += 1) {
      for (let dx = -2; dx <= 2; dx += 1) {
        neighbors.push(signatures.get(key(chunk.x + dx, chunk.y + dy)) ?? 'missing');
      }
    }
    return `${window.worldSeed ?? '0'}|${neighbors.join('|')}`;
  };
}
