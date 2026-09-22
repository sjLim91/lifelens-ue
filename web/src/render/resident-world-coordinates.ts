import type { Resident } from '../runtime/core-types';

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

  const chunkGridSize = options.chunkGridSize ?? 32;
  const chunkWorldSize = options.chunkWorldSize ?? 8;
  const elevationScale = options.elevationScale ?? 48;

  const chunkX = Math.floor(resident.gridX / chunkGridSize);
  const chunkY = Math.floor(resident.gridY / chunkGridSize);
  const localX = (
    resident.gridX - chunkX * chunkGridSize
  ) / chunkGridSize;
  const localY = (
    resident.gridY - chunkY * chunkGridSize
  ) / chunkGridSize;

  return {
    x: (
      chunkX - centerChunkX + localX - 0.5
    ) * chunkWorldSize,
    y: elevation01 * elevationScale,
    z: (
      chunkY - centerChunkY + localY - 0.5
    ) * chunkWorldSize,
  };
}
