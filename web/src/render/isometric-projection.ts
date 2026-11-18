import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

export interface IsometricView {
  width: number;
  height: number;
  extent: number;
  zoom: number;
  centerChunkX: number;
  centerChunkY: number;
  panX?: number;
  panZ?: number;
}

/** Deterministic 45° orthographic projection. Distances are in Core chunks. */
export function createIsometricProjection(view: IsometricView) {
  const extent = Math.max(1, view.extent);
  const zoom = Math.max(.55, Math.min(6.4, Number.isFinite(view.zoom) ? view.zoom : 1));
  const tile = Math.max(.1, Math.min(
    view.width / (extent * 1.16), view.height / (extent * .72),
  ) * zoom);
  const originX = view.width * .5, originY = view.height * .56;
  // Vertical exaggeration belongs to the viewer only: the underlying 17×17
  // Core elevation samples and inverse ground picking remain authoritative.
  const relief = tile * 2.35;
  const chunkSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const panX = (view.panX ?? 0) / chunkSize;
  const panZ = (view.panZ ?? 0) / chunkSize;
  const project = (localX: number, localZ: number, elevation: number) => {
    const x = localX - panX, z = localZ - panZ;
    return {
      x: originX + (x - z) * tile * .5,
      y: originY + (x + z) * tile * .25
        - (Number.isFinite(elevation) ? elevation - .5 : 0) * relief,
    };
  };
  const gridPoint = (gridX: number, gridY: number, elevation: number) =>
    project(
      gridX / WORLD_GRID_CONTRACT.gridCellsPerChunk - view.centerChunkX - .5,
      gridY / WORLD_GRID_CONTRACT.gridCellsPerChunk - view.centerChunkY - .5,
      elevation,
    );
  const inverse = (pixelX: number, pixelY: number, elevation = .5) => {
    const a = (pixelX - originX) / (tile * .5);
    const b = (pixelY - originY + (elevation - .5) * relief) / (tile * .25);
    return {
      x: ((a + b) * .5 + panX) * chunkSize,
      z: ((b - a) * .5 + panZ) * chunkSize,
    };
  };
  return { tile, relief, project, gridPoint, inverse };
}
