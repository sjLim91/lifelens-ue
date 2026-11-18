import type { TerrainChunk, TerrainWindow, WaterKind } from '../runtime/core-types';

/**
 * Visual-only descriptions of topology that Core has already observed.
 * A "River" tile with no adjacent downstream Core edge stays an isolated
 * water site, never a made-up connector to another body of water.
 */
export interface ObservedRiverLink {
  from: TerrainChunk;
  to: TerrainChunk;
  bend: number;
  width01: number;
}
const finite = (n: unknown): n is number => typeof n === 'number' && Number.isFinite(n);
const flow = new Set<WaterKind>(['Spring', 'Stream', 'River']);
const water = new Set<WaterKind>(['Spring', 'Stream', 'River', 'Lake', 'Coast', 'Ocean', 'Wetland']);

export function stableNatureUnit(seed: string, x: number, y: number, index: number): number {
  let hash = 2166136261 >>> 0;
  const key = seed + ':' + x + ':' + y + ':' + index;
  for (let i = 0; i < key.length; i++) hash = Math.imul(hash ^ key.charCodeAt(i), 16777619) >>> 0;
  return hash / 4294967296;
}

export function observedRiverLinks(terrain: TerrainWindow): ObservedRiverLink[] {
  if (!terrain.available) return [];
  const chunks = new Map(terrain.chunks
    .filter(c => finite(c.x) && finite(c.y)).map(c => [c.x + ':' + c.y, c]));
  const links: ObservedRiverLink[] = [];
  for (const from of terrain.chunks) {
    if (!flow.has(from.waterKind) || from.hasDownstream !== true
      || !finite(from.downstreamChunkX) || !finite(from.downstreamChunkY)) continue;
    const dx = from.downstreamChunkX - from.x, dy = from.downstreamChunkY - from.y;
    // A long projected chord cannot be assumed to follow the unseen bends
    // between the two observed endpoints. Draw only direct neighboring edges.
    if (Math.max(Math.abs(dx), Math.abs(dy)) !== 1) continue;
    const to = chunks.get(from.downstreamChunkX + ':' + from.downstreamChunkY);
    if (!to || !water.has(to.waterKind)) continue;
    const availability = Math.max(0, Math.min(1, Number(from.waterAvailability) || 0));
    const width01 = (from.waterKind === 'River' ? .048 : from.waterKind === 'Stream' ? .029 : .021)
      + availability * (from.waterKind === 'River' ? .022 : .01);
    links.push({
      from, to,
      bend: (stableNatureUnit(terrain.worldSeed ?? '0', from.x, from.y, 211) - .5) * .17,
      width01,
    });
  }
  return links;
}

/** A reproducible canopy silhouette, not a claim about individual Core trees. */
export interface ForestSilhouette {
  u: number;
  v: number;
  scale: number;
  broadleaf: boolean;
  tone: number;
}
export function forestSilhouettes(chunk: TerrainChunk, seed: string): ForestSilhouette[] {
  if (chunk.waterKind === 'Ocean' || chunk.waterKind === 'Lake') return [];
  const forest = Math.max(0, Math.min(1, Number(chunk.forestCoverage01) || 0));
  if (forest < .18) return [];
  const count = Math.min(5, 1 + Math.floor(forest * 4.9));
  const biome = (chunk.biome ?? '').toLowerCase();
  const coniferous = /boreal|taiga|tundra|conifer/.test(biome);
  const out: ForestSilhouette[] = [];
  for (let i = 0; i < count; i++) {
    out.push({
      u: .12 + stableNatureUnit(seed, chunk.x, chunk.y, i * 7 + 101) * .76,
      v: .12 + stableNatureUnit(seed, chunk.x, chunk.y, i * 7 + 102) * .76,
      scale: .78 + stableNatureUnit(seed, chunk.x, chunk.y, i * 7 + 103) * .48,
      broadleaf: !coniferous && stableNatureUnit(seed, chunk.x, chunk.y, i * 7 + 104) > .25,
      tone: stableNatureUnit(seed, chunk.x, chunk.y, i * 7 + 105),
    });
  }
  return out.sort((a, b) => a.u + a.v - b.u - b.v);
}

/** Light-facing slope contrast derived exclusively from sampled Core relief. */
export function terrainSlopeLight(a: number, b: number, c: number, d: number): number {
  const xSlope = ((a + d) - (b + c)) * 2.4;
  const ySlope = ((a + b) - (c + d)) * 1.8;
  return Math.max(.68, Math.min(1.28, 1.015 + xSlope + ySlope));
}
