import type {
  CivilizationWorldPayload,
  Resident,
  TerrainChunk,
  TerrainWindow,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

/**
 * Read-only, GPU-free cartographic presentation of the exact observed Core window.
 * Tree/rock/flower glyphs represent Core ecology coverage, not individual
 * resource nodes, traversable paths, discovered sites or new world facts.
 */
export interface IllustratedRegionView {
  width: number;
  height: number;
  centerX: number;
  centerY: number;
  zoom: number;
}

export interface IllustratedRegionProjection {
  tile: number;
  originX: number;
  originY: number;
}

export interface IllustratedRegionPaintInput {
  terrain: TerrainWindow | null;
  civilization: CivilizationWorldPayload;
  residents: readonly Resident[];
  view: IllustratedRegionView;
}

export interface IllustratedRegionPaintStats {
  drawnChunks: number;
  residentMarkers: number;
  settlementMarkers: number;
}

const clamp01 = (value: unknown): number => Number.isFinite(Number(value))
  ? Math.max(0, Math.min(1, Number(value))) : 0;

const stableUnit = (seed: string, x: number, y: number, i: number): number => {
  let hash = 2166136261 >>> 0;
  const input = seed + ':' + x + ':' + y + ':' + i;
  for (let j = 0; j < input.length; j += 1) {
    hash = Math.imul(hash ^ input.charCodeAt(j), 16777619) >>> 0;
  }
  return hash / 4294967296;
};

export function illustratedRegionProjection(
  terrain: TerrainWindow | null,
  view: IllustratedRegionView,
): IllustratedRegionProjection | null {
  const chunks = terrain?.available ? terrain.chunks.filter(c =>
    Number.isFinite(c.x) && Number.isFinite(c.y)) : [];
  if (!chunks.length || view.width <= 0 || view.height <= 0) return null;
  const xs = chunks.map(c => c.x), ys = chunks.map(c => c.y);
  const spanX = Math.max(...xs) - Math.min(...xs) + 1;
  const spanY = Math.max(...ys) - Math.min(...ys) + 1;
  const fit = Math.min(
    (view.width - 32) / spanX,
    (view.height - 32) / spanY,
  );
  const tile = Math.max(0.1, fit * Math.min(4, Math.max(1, view.zoom)));
  return {
    tile,
    originX: view.width / 2 - view.centerX * tile,
    originY: view.height / 2 - view.centerY * tile,
  };
}

/** Hit testing never claims a site outside the returned Core terrain window. */
export function illustratedRegionHit(
  terrain: TerrainWindow | null,
  view: IllustratedRegionView,
  px: number,
  py: number,
): { x: number; y: number } | null {
  const projection = illustratedRegionProjection(terrain, view);
  if (!projection || !Number.isFinite(px) || !Number.isFinite(py)
    || px < 0 || py < 0 || px > view.width || py > view.height) return null;
  const x = Math.floor((px - projection.originX) / projection.tile + 0.5);
  const y = Math.floor((py - projection.originY) / projection.tile + 0.5);
  return terrain?.chunks.some(c => c.x === x && c.y === y) ? { x, y } : null;
}

const surfaceFill = (chunk: TerrainChunk): string => {
  if (chunk.waterKind === 'Ocean') return '#397d96';
  if (chunk.waterKind === 'Coast') return '#c7b48b';
  if (chunk.waterKind === 'Lake') return '#549db1';
  if (chunk.waterKind === 'Wetland') return '#729d7b';
  const forest = clamp01(chunk.forestCoverage01);
  const rock = clamp01(chunk.rockCoverage01);
  const wet = clamp01(chunk.moisture01);
  const elevation = clamp01(chunk.elevation01);
  if (rock > 0.56 || (elevation > .78 && rock > .22)) return '#a6a68d';
  if (forest > 0.58) return '#588655';
  if (forest > 0.24) return '#82a572';
  if (chunk.waterKind === 'River' || chunk.waterKind === 'Stream') return '#85b49a';
  return wet < 0.3 ? '#b6b583' : '#a7c590';
};

function drawFir(ctx: CanvasRenderingContext2D, x: number, y: number, size: number, tone: string): void {
  ctx.fillStyle = 'rgba(28,48,28,.22)';
  ctx.beginPath();
  ctx.ellipse(x + size * .18, y + size * .24, size * .58, size * .23, 0, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#755c42';
  ctx.fillRect(x - size * .07, y - size * .12, size * .14, size * .46);
  ctx.fillStyle = tone;
  for (let layer = 0; layer < 3; layer += 1) {
    const upper = y - size * .78 + size * .29 * layer;
    const half = size * (.33 + .1 * layer);
    ctx.beginPath();
    ctx.moveTo(x, upper);
    ctx.lineTo(x + half, upper + size * .48);
    ctx.lineTo(x - half, upper + size * .48);
    ctx.closePath();
    ctx.fill();
  }
}

function drawRock(ctx: CanvasRenderingContext2D, x: number, y: number, size: number): void {
  ctx.fillStyle = 'rgba(44,50,39,.23)';
  ctx.beginPath();
  ctx.ellipse(x + size * .18, y + size * .32, size * .72, size * .26, 0, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#7f867b';
  ctx.beginPath();
  ctx.moveTo(x - size * .58, y + size * .27);
  ctx.lineTo(x - size * .47, y - size * .27);
  ctx.lineTo(x + size * .13, y - size * .51);
  ctx.lineTo(x + size * .57, y - size * .12);
  ctx.lineTo(x + size * .57, y + size * .3);
  ctx.closePath();
  ctx.fill();
  ctx.fillStyle = '#c0c4ac';
  ctx.beginPath();
  ctx.moveTo(x - size * .47, y - size * .27);
  ctx.lineTo(x + size * .13, y - size * .51);
  ctx.lineTo(x + size * .19, y - size * .07);
  ctx.lineTo(x - size * .2, y + size * .03);
  ctx.closePath();
  ctx.fill();
}

export function paintIllustratedRegion(
  ctx: CanvasRenderingContext2D,
  input: IllustratedRegionPaintInput,
): IllustratedRegionPaintStats {
  const { terrain, civilization, residents, view } = input;
  const empty = { drawnChunks: 0, residentMarkers: 0, settlementMarkers: 0 };
  const { width, height } = view;
  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = '#192b27';
  ctx.fillRect(0, 0, width, height);
  const projection = illustratedRegionProjection(terrain, view);
  if (!projection || !terrain) {
    ctx.font = '15px sans-serif';
    ctx.textAlign = 'center';
    ctx.fillStyle = '#e3e8d9';
    ctx.fillText('관찰 가능한 지역 정보가 없습니다.', width / 2, height / 2);
    return empty;
  }

  const { tile, originX, originY } = projection;
  const seed = String(terrain.worldSeed ?? '0');
  const inView = (x: number, y: number) =>
    x + tile > 0 && x - tile < width && y + tile > 0 && y - tile < height;
  const point = (x: number, y: number) => ({ x: originX + x * tile, y: originY + y * tile });
  let drawnChunks = 0;

  for (const chunk of terrain.chunks) {
    if (!Number.isFinite(chunk.x) || !Number.isFinite(chunk.y)) continue;
    const p = point(chunk.x, chunk.y);
    if (!inView(p.x, p.y)) continue;
    drawnChunks += 1;
    ctx.fillStyle = surfaceFill(chunk);
    ctx.fillRect(p.x - tile * .505, p.y - tile * .505, tile * 1.01, tile * 1.01);

    // All decoration positions are reproducible from the Core world seed and
    // observed chunk position. They are purely graphical indicators.
    const forest = clamp01(chunk.forestCoverage01);
    const rock = clamp01(chunk.rockCoverage01);
    if (tile < 8) continue;
    if (forest >= .18 && chunk.waterKind !== 'Ocean' && chunk.waterKind !== 'Lake') {
      const trees = Math.min(3, 1 + Math.floor(forest * 2.8));
      for (let i = 0; i < trees; i += 1) {
        const px = p.x + (stableUnit(seed, chunk.x, chunk.y, i * 7) - .5) * tile * .6;
        const py = p.y + (stableUnit(seed, chunk.x, chunk.y, i * 7 + 1) - .5) * tile * .55;
        const size = Math.min(19, tile * (.27 + forest * .22));
        drawFir(ctx, px, py, size, i % 3 === 0 ? '#31583c' : '#426e49');
      }
    }
    if ((rock > .32 || (clamp01(chunk.elevation01) > .78 && rock > .15))
      && chunk.waterKind !== 'Ocean' && chunk.waterKind !== 'Lake') {
      drawRock(ctx, p.x + tile * .23, p.y + tile * .19, Math.min(18, tile * (.16 + rock * .3)));
    }
    if (clamp01(chunk.grassCoverage01) > .55 && forest < .4 && rock < .4 && tile > 16) {
      const fx = p.x - tile * .18;
      const fy = p.y + tile * .18;
      ctx.fillStyle = stableUnit(seed, chunk.x, chunk.y, 9) > .5 ? '#d9b8d8' : '#e9d49e';
      for (let i = 0; i < 3; i += 1) {
        ctx.beginPath();
        ctx.arc(fx + i * tile * .075, fy + (i % 2) * tile * .055, Math.min(2, tile * .045), 0, Math.PI * 2);
        ctx.fill();
      }
    }
  }

  if (tile >= 12) {
    const observed = new Set(terrain.chunks.map(c => String(c.x) + ':' + String(c.y)));
    for (const chunk of terrain.chunks) {
      if (chunk.waterKind !== 'River' && chunk.waterKind !== 'Stream') continue;
      const p = point(chunk.x, chunk.y);
      if (!inView(p.x, p.y)) continue;
      ctx.strokeStyle = chunk.waterKind === 'River' ? '#4e9bb1' : '#71b4c4';
      ctx.lineWidth = tile * (chunk.waterKind === 'River' ? .17 : .09);
      ctx.lineCap = 'round';
      // Draw a connected flow segment only for an explicit Core downstream
      // edge whose destination is also present in this observation window.
      const hasEdge = chunk.hasDownstream === true
        && typeof chunk.downstreamChunkX === 'number'
        && typeof chunk.downstreamChunkY === 'number'
        && observed.has(String(chunk.downstreamChunkX) + ':' + String(chunk.downstreamChunkY));
      ctx.beginPath();
      if (hasEdge) {
        const downstream = point(chunk.downstreamChunkX!, chunk.downstreamChunkY!);
        ctx.moveTo(p.x, p.y);
        ctx.lineTo(downstream.x, downstream.y);
        ctx.stroke();
      } else {
        // A water glyph is not an invented direction or upstream source.
        ctx.fillStyle = ctx.strokeStyle;
        ctx.arc(p.x, p.y, tile * .12, 0, Math.PI * 2);
        ctx.fill();
      }
    }
  }

  let settlementMarkers = 0;
  for (const settlement of civilization.available ? civilization.settlements ?? [] : []) {
    if (!settlement.active || !Number.isFinite(settlement.gridX) || !Number.isFinite(settlement.gridY)) continue;
    const x = settlement.gridX / WORLD_GRID_CONTRACT.gridCellsPerChunk - .5;
    const y = settlement.gridY / WORLD_GRID_CONTRACT.gridCellsPerChunk - .5;
    const p = point(x, y);
    if (!inView(p.x, p.y)) continue;
    settlementMarkers += 1;
    ctx.fillStyle = '#f2e4c6';
    ctx.fillRect(p.x - 4, p.y - 1, 8, 7);
    ctx.fillStyle = '#805e47';
    ctx.beginPath();
    ctx.moveTo(p.x - 6, p.y - 1);
    ctx.lineTo(p.x, p.y - 7);
    ctx.lineTo(p.x + 6, p.y - 1);
    ctx.closePath();
    ctx.fill();
    if (tile >= 16) {
      ctx.font = 'bold 11px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillStyle = '#fff8e4';
      ctx.fillText('정착지', p.x, p.y - 11);
    }
  }

  let residentMarkers = 0;
  for (const resident of residents) {
    if (!resident.hasPosition || typeof resident.gridX !== 'number' || typeof resident.gridY !== 'number'
      || !Number.isFinite(resident.gridX) || !Number.isFinite(resident.gridY)) continue;
    const x = resident.gridX / WORLD_GRID_CONTRACT.gridCellsPerChunk - .5;
    const y = resident.gridY / WORLD_GRID_CONTRACT.gridCellsPerChunk - .5;
    const p = point(x, y);
    if (!inView(p.x, p.y)) continue;
    residentMarkers += 1;
    ctx.fillStyle = '#263c32';
    ctx.beginPath();
    ctx.arc(p.x, p.y, 4, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = '#f6dc96';
    ctx.beginPath();
    ctx.arc(p.x, p.y, 2.5, 0, Math.PI * 2);
    ctx.fill();
  }

  // The white outline shows the actual observer center, not a claimed
  // explored/discovered area.
  const observer = point(terrain.centerChunkX, terrain.centerChunkY);
  if (inView(observer.x, observer.y)) {
    ctx.strokeStyle = '#ffefb6';
    ctx.lineWidth = 1.8;
    ctx.strokeRect(observer.x - tile * .48, observer.y - tile * .48, tile * .96, tile * .96);
  }
  return { drawnChunks, residentMarkers, settlementMarkers };
}
