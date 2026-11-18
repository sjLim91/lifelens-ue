import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';
import { stableNatureUnit } from './isometric-natural-presentation';

export type ShoreSide = 'north' | 'east' | 'south' | 'west';
export interface ObservedShoreEdge {
  water: TerrainChunk;
  neighbor: TerrainChunk;
  side: ShoreSide;
  kind: 'sea' | 'lake';
  variation: number;
}
const cardinal: ReadonlyArray<{side: ShoreSide; dx: number; dy: number}> = [
  {side:'north', dx:0, dy:-1}, {side:'east', dx:1, dy:0},
  {side:'south', dx:0, dy:1}, {side:'west', dx:-1, dy:0},
];
const finite = (n: unknown): n is number => typeof n === 'number' && Number.isFinite(n);

/**
 * A shoreline is drawn only where TWO actually observed adjacent Core chunks
 * disagree about this specific waterbody. Never infer land outside the window,
 * or invent a coastline from a missing neighbor.
 */
export function observedShoreEdges(window: TerrainWindow): ObservedShoreEdge[] {
  if (!window.available) return [];
  const byGrid = new Map(window.chunks.filter(c => finite(c.x) && finite(c.y))
    .map(c => [c.x + ':' + c.y, c]));
  const result: ObservedShoreEdge[] = [];
  for (const water of window.chunks) {
    const kind = water.waterKind === 'Ocean' ? 'sea'
      : water.waterKind === 'Lake' ? 'lake' : null;
    if (!kind) continue;
    for (const { side, dx, dy } of cardinal) {
      const neighbor = byGrid.get((water.x + dx) + ':' + (water.y + dy));
      if (!neighbor) continue;
      if ((kind === 'sea' && neighbor.waterKind === 'Ocean')
        || (kind === 'lake' && neighbor.waterKind === 'Lake')) continue;
      result.push({
        water, neighbor, side, kind,
        // Variance is visual edge irregularity, never a new water source.
        variation: stableNatureUnit(window.worldSeed ?? '0', water.x, water.y,
          side === 'north' ? 813 : side === 'east' ? 814 : side === 'south' ? 815 : 816) - .5,
      });
    }
  }
  return result;
}

/** Water-side local boundary endpoints, guaranteed on this Core water tile. */
export function waterTileEdge(side: ShoreSide): readonly [
  {u: number; v: number}, {u: number; v: number},
] {
  if (side === 'north') return [{u:0,v:0},{u:1,v:0}];
  if (side === 'east') return [{u:1,v:0},{u:1,v:1}];
  if (side === 'south') return [{u:1,v:1},{u:0,v:1}];
  return [{u:0,v:1},{u:0,v:0}];
}

/** Canvas size budgets: never turn a coastline into a broad blue road. */
export function shoreStrokeWidths(tile: number) {
  const base = Math.max(.75, Math.min(3.8, tile * .021));
  return { bank: Math.min(5.5, base + 1.3), water: base,
    glint: Math.max(.4, Math.min(1.15, base * .35)) };
}

/** Low-cost ground texture guided by observed coverage; not individual items. */
export interface GroundBrush {
  u: number;
  v: number;
  scale: number;
  kind: 'grass' | 'wetland' | 'gravel';
}
export function observedGroundBrushes(chunk: TerrainChunk, seed: string): GroundBrush[] {
  if (chunk.waterKind === 'Ocean' || chunk.waterKind === 'Lake') return [];
  const grass = Math.max(0, Math.min(1, Number(chunk.grassCoverage01) || 0));
  const wet = Math.max(0, Math.min(1, Number(chunk.wetlandCoverage01) || 0));
  const rock = Math.max(0, Math.min(1, Number(chunk.rockCoverage01) || 0));
  // Color variation comes from the real coverage, not tile-grid noise.
  const counts: Array<[GroundBrush['kind'],number]> = [
    ['grass',Math.floor(grass * 5)],['wetland',Math.floor(wet * 3)],
    ['gravel',Math.floor(rock * 3)],
  ];
  const result: GroundBrush[] = [];
  let salt=1100;
  for (const [kind,count] of counts) for (let i=0;i<count;i++) {
    result.push({
      u:.13 + stableNatureUnit(seed,chunk.x,chunk.y,salt++) * .74,
      v:.13 + stableNatureUnit(seed,chunk.x,chunk.y,salt++) * .74,
      scale:.7 + stableNatureUnit(seed,chunk.x,chunk.y,salt++) * .6,
      kind,
    });
  }
  return result;
}

/**
 * Difference in *observed* Core elevation across neighboring edge samples.
 * It controls shaded slope accents, not physics, collision or a claim of a cliff.
 */
export function observedSlopeContrast(samples: readonly number[]): number {
  if (samples.length === 0 || !samples.every(Number.isFinite)) return 0;
  const min = Math.min(...samples), max = Math.max(...samples);
  return Math.min(1, Math.max(0, (max - min - .045) * 5));
}
