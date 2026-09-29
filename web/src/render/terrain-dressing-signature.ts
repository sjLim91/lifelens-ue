import type { HumanTrace, TerrainWindow } from '../runtime/core-types';

function mix(hash: number, value: string): number {
  let next = hash >>> 0;
  for (let index = 0; index < value.length; index += 1) {
    next ^= value.charCodeAt(index);
    next = Math.imul(next, 16777619) >>> 0;
  }
  return next >>> 0;
}

function numberToken(value: unknown): string {
  const numeric = Number(value);
  return Number.isFinite(numeric) ? numeric.toFixed(5) : '0';
}

function facilityTraceToken(trace: HumanTrace): string | null {
  if (trace.kind !== 'Facility') return null;
  return [
    trace.id,
    trace.gridX,
    trace.gridY,
    trace.facilityKind,
  ].join(':');
}

export function terrainDressingSignature(
  window: TerrainWindow,
): string {
  let hash = 2166136261 >>> 0;
  hash = mix(hash, window.worldSeed ?? '0');
  hash = mix(hash, `${window.centerChunkX}:${window.centerChunkY}`);
  hash = mix(hash, String(window.chunks.length));

  for (const chunk of window.chunks) {
    hash = mix(hash, [
      chunk.x,
      chunk.y,
      chunk.waterKind,
      chunk.salinity ?? '',
      numberToken(chunk.elevation01),
      numberToken(chunk.waterAvailability),
      numberToken(chunk.flowPotential),
      numberToken(chunk.drainageAccumulationPotential),
      chunk.hasDownstream ? 1 : 0,
      chunk.downstreamChunkX ?? '',
      chunk.downstreamChunkY ?? '',
      numberToken(chunk.forestCoverage01),
      numberToken(chunk.grassCoverage01),
      numberToken(chunk.shrubCoverage01),
      numberToken(chunk.rockCoverage01),
      numberToken(chunk.wetlandCoverage01),
    ].join(':'));
  }

  for (const trace of window.humanTraces?.entries ?? []) {
    const token = facilityTraceToken(trace);
    if (token) hash = mix(hash, token);
  }

  return hash.toString(16);
}
