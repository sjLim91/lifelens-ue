import type { CivilizationWorldPayload, Resident, TerrainChunk, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { ResidentContinuity } from '../runtime/resident-continuity';
import { runtimeDiagnostics } from '../runtime/runtime-diagnostics';
import { visibleHumanTraces } from '../state/human-traces';
import { createTerrainElevationSampler } from './terrain-geometry';
import { shade, terrainColor } from './terrain-presentation';
import { createIsometricProjection, type IsometricView } from './isometric-projection';

export interface IsometricWorldDrawOptions {
  terrain: TerrainWindow | null;
  residents: Resident[];
  civilization: CivilizationWorldPayload;
  centerX: number;
  centerY: number;
  zoom: number;
  panX?: number;
  panZ?: number;
  selectedResidentId?: string | null;
  selectedHumanTraceId?: string | null;
  selectedSettlementId?: string | null;
}
export type IsometricMarkerKind = 'resident' | 'trace' | 'settlement';
export interface IsometricMarker {
  kind: IsometricMarkerKind;
  id: string;
  x: number;
  y: number;
  radius: number;
}
export interface IsometricPaintResult {
  tile: number;
  drawnChunks: number;
  residentMarkers: number;
  traceMarkers: number;
  settlementMarkers: number;
  markers: IsometricMarker[];
}
const empty = (): IsometricPaintResult =>
  ({ tile: 0, drawnChunks: 0, residentMarkers: 0, traceMarkers: 0, settlementMarkers: 0, markers: [] });
const valid = (n: unknown): n is number => typeof n === 'number' && Number.isFinite(n);
const waterFill = (chunk: TerrainChunk): string | null => {
  if (chunk.waterKind === 'Ocean') return '#316e89';
  if (chunk.waterKind === 'Coast') return '#538e9a';
  if (chunk.waterKind === 'Lake') return '#438ca1';
  return null;
};

/** Draw the observed Core window as a 2.5D surface, never as generated world state. */
export function paintIsometricWorld(
  ctx: CanvasRenderingContext2D,
  view: IsometricView,
  options: IsometricWorldDrawOptions,
  continuity: ResidentContinuity,
): IsometricPaintResult {
  const { width, height } = view;
  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = '#172a31';
  ctx.fillRect(0, 0, width, height);
  const terrain = options.terrain;
  if (!terrain?.available || terrain.chunks.length === 0) {
    ctx.fillStyle = '#e6ecdf'; ctx.textAlign = 'center'; ctx.font = '15px sans-serif';
    ctx.fillText('시뮬레이션 세계 정보를 불러오는 중…', width * .5, height * .5);
    return empty();
  }
  const projection = createIsometricProjection(view);
  const sample = createTerrainElevationSampler(terrain);
  const chunkSet = new Set(terrain.chunks.map(c => c.x + ':' + c.y));
  const markers: IsometricMarker[] = [];
  const { tile } = projection;
  let drawnChunks = 0;
  const chunks = [...terrain.chunks]
    .filter(chunk => valid(chunk.x) && valid(chunk.y) && valid(chunk.elevation01))
    .sort((a, b) => (a.x + a.y) - (b.x + b.y) || a.x - b.x);
  const point = (x: number, y: number, elevation: number) =>
    projection.project(x - options.centerX, y - options.centerY, elevation);
  const visible = (x: number, y: number, radius = tile) =>
    x + radius >= 0 && x - radius <= width
    && y + radius >= 0 && y - radius <= height;
  const polygon = (points: Array<{ x: number; y: number }>, fill: string) => {
    ctx.fillStyle = fill;
    ctx.beginPath(); ctx.moveTo(points[0].x, points[0].y);
    for (let i = 1; i < points.length; i++) ctx.lineTo(points[i].x, points[i].y);
    ctx.closePath(); ctx.fill();
  };
  // Even 17×17 Core height lattices are drawn from their actual values.
  // Adaptive tessellation bounds Canvas cost on low-end/mobile browsers.
  const subdivisionStep = tile < 18 ? 8 : tile < 38 ? 4 : 2;
  const seeds = terrain.worldSeed ?? '0';
  const hash = (x: number, y: number, i: number) => {
    let h = 2166136261;
    for (const c of seeds + ':' + x + ':' + y + ':' + i) h = Math.imul(h ^ c.charCodeAt(0), 16777619);
    return (h >>> 0) / 4294967296;
  };
  for (const chunk of chunks) {
    const center = point(chunk.x, chunk.y, chunk.elevation01);
    if (!visible(center.x, center.y, tile * 2)) continue;
    drawnChunks++;
    const base = waterFill(chunk) ?? terrainColor(chunk);
    for (let diagonal = 0; diagonal < 32; diagonal += subdivisionStep) {
      for (let ix = 0; ix < 16; ix += subdivisionStep) {
        const iy = diagonal - ix;
        if (iy < 0 || iy >= 16) continue;
        const u = ix / 16, v = iy / 16, d = subdivisionStep / 16;
        const a = sample(chunk.x, chunk.y, u, v);
        const b = sample(chunk.x, chunk.y, u + d, v);
        const c = sample(chunk.x, chunk.y, u + d, v + d);
        const e = sample(chunk.x, chunk.y, u, v + d);
        const p0 = point(chunk.x + u - .5, chunk.y + v - .5, a);
        const p1 = point(chunk.x + u + d - .5, chunk.y + v - .5, b);
        const p2 = point(chunk.x + u + d - .5, chunk.y + v + d - .5, c);
        const p3 = point(chunk.x + u - .5, chunk.y + v + d - .5, e);
        const slope = Math.max(-.13, Math.min(.13, (a + e - b - c) * .28 + (a + b - c - e) * .2));
        const variation = (hash(chunk.x, chunk.y, ix * 17 + iy) - .5) * .025;
        polygon([p0, p1, p2, p3], shade(base, .97 + slope + variation));
      }
    }
    if (chunk.x === options.centerX && chunk.y === options.centerY) {
      const corners = [[0, 0], [1, 0], [1, 1], [0, 1]].map(([u, v]) =>
        point(chunk.x + u - .5, chunk.y + v - .5, sample(chunk.x, chunk.y, u, v)));
      ctx.strokeStyle = 'rgba(244,235,195,.82)';
      ctx.lineWidth = Math.max(1.1, tile * .015);
      ctx.beginPath(); ctx.moveTo(corners[0].x, corners[0].y);
      for (let i = 1; i < 4; i++) ctx.lineTo(corners[i].x, corners[i].y);
      ctx.closePath(); ctx.stroke();
    }
  }

  // A river line is permitted ONLY when Core explicitly reports the
  // downstream cell within this observed window. No inferred water routes.
  for (const chunk of chunks) {
    if (!['River', 'Stream', 'Spring'].includes(chunk.waterKind)) continue;
    const from = point(chunk.x, chunk.y, sample(chunk.x, chunk.y, .5, .5) + .006);
    if (!visible(from.x, from.y, tile)) continue;
    const canFlow = chunk.hasDownstream === true
      && valid(chunk.downstreamChunkX) && valid(chunk.downstreamChunkY)
      && chunkSet.has(chunk.downstreamChunkX + ':' + chunk.downstreamChunkY);
    ctx.strokeStyle = chunk.waterKind === 'River' ? '#4fc2dc' : '#76c3d0';
    ctx.lineWidth = Math.max(1.3, tile * (chunk.waterKind === 'River' ? .11 : .06));
    ctx.lineCap = 'round';
    if (canFlow) {
      const target = chunks.find(c => c.x === chunk.downstreamChunkX && c.y === chunk.downstreamChunkY)!;
      const to = point(target.x, target.y, sample(target.x, target.y, .5, .5) + .006);
      ctx.beginPath(); ctx.moveTo(from.x, from.y); ctx.lineTo(to.x, to.y); ctx.stroke();
    } else {
      ctx.fillStyle = ctx.strokeStyle; ctx.beginPath();
      ctx.arc(from.x, from.y, Math.max(1.5, tile * .065), 0, Math.PI * 2); ctx.fill();
    }
  }

  // Purely decorative coverage indicators are deterministic and are never
  // claimed to be individual Core resources, harvested trees or smart objects.
  if (tile >= 11) for (const chunk of chunks) {
    if (chunk.waterKind === 'Ocean' || chunk.waterKind === 'Lake') continue;
    const forest = Math.max(0, Math.min(1, Number(chunk.forestCoverage01) || 0));
    const rock = Math.max(0, Math.min(1, Number(chunk.rockCoverage01) || 0));
    if (forest >= .18) {
      const count = Math.min(3, 1 + Math.floor(forest * 2.5));
      for (let i = 0; i < count; i++) {
        const u = .2 + hash(chunk.x, chunk.y, i + 100) * .6;
        const v = .2 + hash(chunk.x, chunk.y, i + 200) * .6;
        const p = point(chunk.x + u - .5, chunk.y + v - .5, sample(chunk.x, chunk.y, u, v));
        if (!visible(p.x, p.y, tile)) continue;
        const size = Math.min(20, tile * (.16 + forest * .1));
        ctx.fillStyle = '#284a35'; ctx.beginPath();
        ctx.moveTo(p.x, p.y - size);
        ctx.lineTo(p.x - size * .52, p.y + size * .15);
        ctx.lineTo(p.x + size * .52, p.y + size * .15);
        ctx.closePath(); ctx.fill();
        ctx.fillStyle = '#4b7754'; ctx.beginPath();
        ctx.moveTo(p.x, p.y - size * 1.22);
        ctx.lineTo(p.x - size * .4, p.y - size * .22);
        ctx.lineTo(p.x + size * .4, p.y - size * .22);
        ctx.closePath(); ctx.fill();
      }
    }
    if (rock > .36) {
      const p = point(chunk.x + .16, chunk.y - .17, sample(chunk.x, chunk.y, .66, .33));
      const size = Math.min(12, tile * .1);
      polygon([{x:p.x,y:p.y-size},{x:p.x+size,y:p.y},{x:p.x,y:p.y+size*.5},{x:p.x-size,y:p.y}], '#a7a393');
    }
  }

  const drawable: Array<{ depth: number; draw: () => void }> = [];
  const addMarker = (kind: IsometricMarkerKind, id: string, p: { x: number; y: number },
    radius: number, draw: () => void, depth: number) => {
    if (!visible(p.x, p.y, radius * 3)) return;
    markers.push({ kind, id, x: p.x, y: p.y, radius });
    drawable.push({ depth, draw });
  };
  const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const heightAtGrid = (x: number, y: number) => {
    const cx = Math.floor(x / span), cy = Math.floor(y / span);
    return chunkSet.has(cx + ':' + cy)
      ? sample(cx, cy, x / span - cx, y / span - cy) : null;
  };
  for (const trace of visibleHumanTraces(terrain)) {
    const h = heightAtGrid(trace.gridX, trace.gridY);
    if (h === null) continue;
    const p = projection.gridPoint(trace.gridX, trace.gridY, h);
    const selected = trace.id === options.selectedHumanTraceId;
    const radius = Math.max(4, Math.min(tile * .14, 11));
    addMarker('trace', trace.id, p, Math.max(radius * 1.5, 8), () => {
      ctx.fillStyle = trace.kind === 'Facility'
        ? trace.state === 'Ruined' ? '#735f55' : '#c7a875'
        : trace.kind === 'ResourceUse' ? '#d5b468' : '#897b65';
      ctx.fillRect(p.x - radius, p.y - radius * 1.5, radius * 2, radius * 1.4);
      ctx.fillStyle = trace.kind === 'Facility' ? '#715a45' : '#544b38';
      ctx.beginPath(); ctx.moveTo(p.x - radius * 1.2, p.y - radius * 1.5);
      ctx.lineTo(p.x, p.y - radius * 2.4);
      ctx.lineTo(p.x + radius * 1.2, p.y - radius * 1.5); ctx.closePath(); ctx.fill();
      if (selected) {
        ctx.strokeStyle = '#fff5b8'; ctx.lineWidth = 2;
        ctx.strokeRect(p.x - radius * 1.5, p.y - radius * 2.7, radius * 3, radius * 2.8);
      }
    }, trace.gridX + trace.gridY);
  }
  for (const settlement of options.civilization.available ? options.civilization.settlements ?? [] : []) {
    if (!settlement.active || !valid(settlement.gridX) || !valid(settlement.gridY)) continue;
    const h = heightAtGrid(settlement.gridX, settlement.gridY);
    if (h === null) continue;
    const p = projection.gridPoint(settlement.gridX, settlement.gridY, h);
    const selected = settlement.id === options.selectedSettlementId;
    addMarker('settlement', settlement.id, p, 13, () => {
      ctx.fillStyle = '#f0df9e'; ctx.beginPath(); ctx.arc(p.x, p.y - 10, 6, 0, Math.PI * 2);ctx.fill();
      ctx.strokeStyle = selected ? '#fff' : '#6a735c';ctx.lineWidth = selected ? 3 : 1;
      ctx.beginPath();ctx.arc(p.x, p.y - 10, 10, 0, Math.PI * 2);ctx.stroke();
    }, settlement.gridX + settlement.gridY);
  }
  for (const resident of options.residents) {
    const position = continuity.positionFor(resident);
    if (!position) continue;
    const h = heightAtGrid(position.x, position.y);
    if (h === null) continue;
    const p = projection.gridPoint(position.x, position.y, h);
    const selected = resident.id === options.selectedResidentId;
    const radius = Math.max(4, Math.min(10, tile * .12));
    addMarker('resident', resident.id, p, Math.max(9, radius * 1.7), () => {
      // M0 uses readable 2.5D observation markers. Full eight-direction
      // authored sprite animation remains the distinct M2 milestone.
      ctx.fillStyle = 'rgba(17,29,25,.42)';ctx.beginPath();
      ctx.ellipse(p.x,p.y,radius*.95,radius*.35,0,0,Math.PI*2);ctx.fill();
      ctx.fillStyle = resident.sex === 'Female' ? '#d6a48a' : '#97bb9c';
      ctx.beginPath();ctx.ellipse(p.x,p.y-radius*1.35,radius*.58,radius*1.13,0,0,Math.PI*2);ctx.fill();
      ctx.fillStyle = '#f1d7b2';ctx.beginPath();
      ctx.arc(p.x,p.y-radius*2.5,radius*.58,0,Math.PI*2);ctx.fill();
      if (selected) {
        ctx.strokeStyle = '#fff1a7';ctx.lineWidth = 2.5;ctx.beginPath();
        ctx.ellipse(p.x,p.y,radius*1.4,radius*.66,0,0,Math.PI*2);ctx.stroke();
      }
    }, position.x + position.y + 1);
  }
  drawable.sort((a, b) => a.depth - b.depth);
  for (const marker of drawable) marker.draw();
  return {
    tile, drawnChunks,
    residentMarkers: markers.filter(m => m.kind === 'resident').length,
    traceMarkers: markers.filter(m => m.kind === 'trace').length,
    settlementMarkers: markers.filter(m => m.kind === 'settlement').length,
    markers,
  };
}

/** Canvas 2D only. Shares the same live Core session as the 3D/legacy viewers. */
export class IsometricCanvasWorldRenderer {
  private readonly ctx: CanvasRenderingContext2D;
  private last: {
    view: IsometricView;
    result: IsometricPaintResult;
    sample: ReturnType<typeof createTerrainElevationSampler>;
    chunks: Set<string>;
  } | null = null;

  constructor(private readonly canvas: HTMLCanvasElement,
    private readonly continuity: ResidentContinuity) {
    const ctx = canvas.getContext('2d');
    if (!ctx) throw new Error('2.5D Canvas 2D context unavailable');
    this.ctx = ctx;
  }

  draw(options: IsometricWorldDrawOptions): void {
    const started = performance.now();
    const terrain = options.terrain;
    const xs = terrain?.chunks.map(c => c.x) ?? [];
    const ys = terrain?.chunks.map(c => c.y) ?? [];
    const extent = xs.length
      ? Math.max(Math.max(...xs) - Math.min(...xs) + 1,
        Math.max(...ys) - Math.min(...ys) + 1) : 1;
    const view: IsometricView = {
      width: this.canvas.width, height: this.canvas.height,
      extent, zoom: options.zoom, centerChunkX: options.centerX,
      centerChunkY: options.centerY, panX: options.panX, panZ: options.panZ,
    };
    const result = paintIsometricWorld(this.ctx, view, options, this.continuity);
    this.last = terrain?.available ? {
      view, result, sample: createTerrainElevationSampler(terrain),
      chunks: new Set(terrain.chunks.map(c => c.x + ':' + c.y)),
    } : null;
    runtimeDiagnostics.recordRender(performance.now() - started);
  }

  private canvasPoint(clientX: number, clientY: number) {
    const rect = this.canvas.getBoundingClientRect();
    if (!rect.width || !rect.height) return null;
    return {
      x: (clientX - rect.left) * this.canvas.width / rect.width,
      y: (clientY - rect.top) * this.canvas.height / rect.height,
    };
  }

  groundPointAtClient(clientX: number, clientY: number): { x: number; z: number } | null {
    const point = this.canvasPoint(clientX, clientY);
    if (!point || !this.last) return null;
    const { view, sample, chunks } = this.last;
    const projector = createIsometricProjection(view);
    // Iteratively invert visible ground relief; dragging/zooming uses Core
    // heights rather than assuming the viewport is an always-flat plane.
    let elevation = .5, position = projector.inverse(point.x, point.y);
    const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    for (let i = 0; i < 3; i++) {
      const gx = position.x / size + view.centerChunkX + .5;
      const gy = position.z / size + view.centerChunkY + .5;
      const cx = Math.floor(gx), cy = Math.floor(gy);
      if (!chunks.has(cx + ':' + cy)) break;
      elevation = sample(cx, cy, gx - cx, gy - cy);
      position = projector.inverse(point.x, point.y, elevation);
    }
    return position;
  }

  private pick(kind: IsometricMarkerKind, clientX: number, clientY: number): string | null {
    const point = this.canvasPoint(clientX, clientY);
    if (!point || !this.last) return null;
    // Nearest on-screen observed marker, never a guess outside the DTO.
    const hits = this.last.result.markers.filter(marker => marker.kind === kind
      && Math.hypot(marker.x - point.x, marker.y - point.y) <= marker.radius);
    hits.sort((a, b) => Math.hypot(a.x - point.x, a.y - point.y)
      - Math.hypot(b.x - point.x, b.y - point.y));
    return hits[0]?.id ?? null;
  }
  pickResident(x: number, y: number): string | null { return this.pick('resident', x, y); }
  pickHumanTrace(x: number, y: number): string | null { return this.pick('trace', x, y); }
  pickSettlement(x: number, y: number): string | null { return this.pick('settlement', x, y); }
}
