import type { TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';

/** Match the rendered 10x10 triangle surface (including its a-c diagonal),
 * not a chord between two snapshot heights or an invented collision surface. */
export class ResidentGroundSurface {
  private sample: ReturnType<typeof createTerrainElevationSampler> | null = null;
  private readonly chunks = new Map<string, Float64Array>();
  private centerX = 0; private centerY = 0;
  setTerrain(terrain: TerrainWindow, centerX: number, centerY: number): void {
    this.sample = createTerrainElevationSampler(terrain); this.centerX = centerX; this.centerY = centerY;
    this.chunks.clear(); for (const c of terrain.chunks) this.chunks.set(`${c.x}:${c.y}`, new Float64Array([this.sample(c.x,c.y,0,0),this.sample(c.x,c.y,1,0),this.sample(c.x,c.y,0,1),this.sample(c.x,c.y,1,1)]));
  }
  private bilinear(p: Float64Array,x:number,y:number): number { return (p[0]+(p[1]-p[0])*x)*(1-y)+(p[2]+(p[3]-p[2])*x)*y; }
  height(x: number, z: number): number | null {
    if (!this.sample || !Number.isFinite(x+z)) return null;
    const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const gx = x/size+this.centerX+.5, gy = z/size+this.centerY+.5;
    const cx = Math.floor(gx), cy = Math.floor(gy);
    const patch=this.chunks.get(`${cx}:${cy}`);if(!patch)return null;
    const tx = (gx-cx)*10, ty = (gy-cy)*10;
    const ix = Math.min(9, Math.floor(tx)), iy = Math.min(9, Math.floor(ty));
    const fx = tx-ix, fy = ty-iy;
    const a = this.bilinear(patch,ix/10,iy/10);
    const c = this.bilinear(patch,(ix+1)/10,(iy+1)/10);
    const h = fx >= fy
      ? a+(this.bilinear(patch,(ix+1)/10,iy/10)-a)*(fx-fy)+(c-a)*fy
      : a+(this.bilinear(patch,ix/10,(iy+1)/10)-a)*(fy-fx)+(c-a)*fx;
    return h*WORLD_GRID_CONTRACT.elevationScale;
  }
}
