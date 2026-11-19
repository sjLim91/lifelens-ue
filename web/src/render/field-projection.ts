import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

/**
 * Grounded quarter-view (2.5D) projection: the two Core horizontal axes
 * recede diagonally and observed elevation lifts ground and sprites.
 * This is a reversible view transform; it cannot change Core positions.
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
  // Parallel axes form a 3/4 diamond footprint, not a flat overhead map.
  const diagonal = tile * .70;
  const depth = .38;
  const recede = tile * depth;
  const heightScale = tile * 1.72;
  const panX = (view.panX ?? 0) / WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const panZ = (view.panZ ?? 0) / WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const ox = view.width * .5, oy = view.height * .51;
  const project = (localX: number, localY: number, elevation = .5) => {
    const x = localX - panX, y = localY - panZ;
    return {
      x: ox + (x - y) * diagonal,
      y: oy + (x + y) * recede
        - (Number.isFinite(elevation) ? elevation - .5 : 0) * heightScale,
    };
  };
  const gridPoint = (gridX: number, gridY: number, elevation: number) =>
    project(
      gridX / WORLD_GRID_CONTRACT.gridCellsPerChunk - view.centerChunkX - .5,
      gridY / WORLD_GRID_CONTRACT.gridCellsPerChunk - view.centerChunkY - .5,
      elevation,
    );
  const inverse = (screenX: number, screenY: number, elevation = .5) => {
    const a = (screenX - ox) / diagonal;
    const b = (screenY - oy + (elevation - .5) * heightScale) / recede;
    return {
      x: ((a + b) * .5 + panX) * WORLD_GRID_CONTRACT.worldUnitsPerChunk,
      z: ((b - a) * .5 + panZ) * WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    };
  };
  return { tile, depth, heightScale, gridPoint, project, inverse };
}
