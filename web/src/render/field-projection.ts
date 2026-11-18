import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

/**
 * Close, readable top-down 2.5D field view. Unlike the 45° region map,
 * world X stays screen X and world Y is tilted/compressed down-screen.
 * Core grid/height stays authoritative; only screen projection changes.
 */
export interface FieldView {
  width: number;
  height: number;
  zoom: number;
  centerChunkX: number;
  centerChunkY: number;
  panX?: number;
  panZ?: number;
}
export function createFieldProjection(view: FieldView) {
  const zoom = Number.isFinite(view.zoom) ? Math.max(.6, Math.min(3.6, view.zoom)) : 1;
  const tile = Math.max(16, Math.min(view.width * .49, view.height * .35) * zoom);
  const depth = .76;
  const heightScale = tile * .9;
  const panX = (view.panX ?? 0) / WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const panZ = (view.panZ ?? 0) / WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const ox = view.width * .5, oy = view.height * .52;
  const project = (localX: number, localY: number, elevation = .5) => ({
    x: ox + (localX - panX) * tile,
    y: oy + (localY - panZ) * tile * depth
      - (Number.isFinite(elevation) ? elevation - .5 : 0) * heightScale,
  });
  const gridPoint = (gridX: number, gridY: number, elevation: number) =>
    project(
      gridX / WORLD_GRID_CONTRACT.gridCellsPerChunk - view.centerChunkX - .5,
      gridY / WORLD_GRID_CONTRACT.gridCellsPerChunk - view.centerChunkY - .5,
      elevation,
    );
  const inverse = (screenX: number, screenY: number, elevation = .5) => ({
    x: ((screenX - ox) / tile + panX) * WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    z: ((screenY - oy + (elevation - .5) * heightScale) / (tile * depth) + panZ)
      * WORLD_GRID_CONTRACT.worldUnitsPerChunk,
  });
  return { tile, depth, heightScale, gridPoint, project, inverse };
}
