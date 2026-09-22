import type { Resident } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

export interface ResidentWorldPosition {
  x: number;
  y: number;
  z: number;
}

export interface ResidentWorldMappingOptions {
  chunkGridSize?: number;
  chunkWorldSize?: number;
  elevationScale?: number;
}

export function gridToWorldPosition(
  gridX: number,
  gridY: number,
  centerChunkX: number,
  centerChunkY: number,
  elevation01: number,
  options: ResidentWorldMappingOptions = {},
): ResidentWorldPosition {
  const chunkGridSize =
    options.chunkGridSize ?? WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const chunkWorldSize =
    options.chunkWorldSize ?? WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const elevationScale =
    options.elevationScale ?? WORLD_GRID_CONTRACT.elevationScale;

  const chunkX = Math.floor(gridX / chunkGridSize);
  const chunkY = Math.floor(gridY / chunkGridSize);
  const localX = (gridX - chunkX * chunkGridSize) / chunkGridSize;
  const localY = (gridY - chunkY * chunkGridSize) / chunkGridSize;

  return {
    x: (chunkX - centerChunkX + localX - 0.5) * chunkWorldSize,
    y: elevation01 * elevationScale,
    z: (chunkY - centerChunkY + localY - 0.5) * chunkWorldSize,
  };
}

export function residentToWorldPosition(
  resident: Resident,
  centerChunkX: number,
  centerChunkY: number,
  elevation01: number,
  options: ResidentWorldMappingOptions = {},
): ResidentWorldPosition | null {
  if (
    !resident.hasPosition
    || resident.gridX === undefined
    || resident.gridY === undefined
  ) {
    return null;
  }

  return gridToWorldPosition(
    resident.gridX,
    resident.gridY,
    centerChunkX,
    centerChunkY,
    elevation01,
    options,
  );
}
