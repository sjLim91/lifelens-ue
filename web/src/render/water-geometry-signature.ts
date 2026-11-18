import type { TerrainWindow } from '../runtime/core-types';

/**
 * Cache key for every Core field used to build visible open / fresh / flowing
 * water geometry. The ordering of equivalent observed chunks is irrelevant.
 * Deliberately omit weather/time: those are material uniforms, not geometry.
 */
export function waterGeometrySignature(window: TerrainWindow): string {
  const chunks = window.chunks.map(chunk => JSON.stringify([
    chunk.x,
    chunk.y,
    chunk.waterKind,
    chunk.elevation01,
    chunk.salinity ?? null,
    chunk.waterAvailability,
    chunk.flowPotential ?? null,
    chunk.hasDownstream ?? false,
    chunk.downstreamChunkX ?? null,
    chunk.downstreamChunkY ?? null,
  ])).sort();
  return JSON.stringify([
    window.worldSeed ?? '0',
    window.centerChunkX,
    window.centerChunkY,
    chunks,
  ]);
}
