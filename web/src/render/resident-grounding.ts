import type { TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';

/** Actual rendered triangles: 16x16 for detailed Core terrain, 10x10 for
 * legacy patches. Both use the geometry builder's a-c diagonal. */
export class ResidentGroundSurface {
  private readonly chunks = new Map<string, { subdivisions: number; heights: Float64Array }>();
  private centerX = 0;
  private centerY = 0;

  setTerrain(terrain: TerrainWindow, centerX: number, centerY: number): void {
    const sample = createTerrainElevationSampler(terrain);
    this.centerX = centerX;
    this.centerY = centerY;
    this.chunks.clear();
    for (const chunk of terrain.chunks) {
      const subdivisions = chunk.elevationSamples?.length === 289 ? 16 : 10;
      const heights = new Float64Array((subdivisions + 1) ** 2);
      for (let y = 0; y <= subdivisions; y++) {
        for (let x = 0; x <= subdivisions; x++) {
          heights[y * (subdivisions + 1) + x] = sample(chunk.x, chunk.y,
            x / subdivisions, y / subdivisions) * WORLD_GRID_CONTRACT.elevationScale;
        }
      }
      this.chunks.set(`${chunk.x}:${chunk.y}`, { subdivisions, heights });
    }
  }

  height(x: number, z: number): number | null {
    if (!Number.isFinite(x) || !Number.isFinite(z)) return null;
    const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const gx = x / size + this.centerX + .5;
    const gy = z / size + this.centerY + .5;
    const cx = Math.floor(gx), cy = Math.floor(gy);
    const patch = this.chunks.get(`${cx}:${cy}`);
    if (!patch) return null;
    const { subdivisions: n, heights } = patch;
    const tx = (gx - cx) * n, ty = (gy - cy) * n;
    const ix = Math.min(n - 1, Math.floor(tx)), iy = Math.min(n - 1, Math.floor(ty));
    const fx = tx - ix, fy = ty - iy, stride = n + 1;
    const a = heights[iy * stride + ix], c = heights[(iy + 1) * stride + ix + 1];
    return fx >= fy
      ? a * (1 - fx) + heights[iy * stride + ix + 1] * (fx - fy) + c * fy
      : a * (1 - fy) + heights[(iy + 1) * stride + ix] * (fy - fx) + c * fx;
  }
}

