import { CharacterLayer, type ResidentPlacement } from '../character-layer';
import type {
  Resident,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import { ResidentContinuity } from '../runtime/resident-continuity';
import { runtimeDiagnostics } from '../runtime/runtime-diagnostics';
import {
  clamp01,
  presentationHash01,
  shade,
  terrainColor,
} from './terrain-presentation';
import { createWorldProjector } from './world-projection';

export interface LegacyCanvasWorldDrawOptions {
  terrain: TerrainWindow | null;
  residents: Resident[];
  centerX: number;
  centerY: number;
  zoom: number;
  angle: number;
}

export class LegacyCanvasWorldRenderer {
  private readonly ctx: CanvasRenderingContext2D;

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly characterLayer: CharacterLayer,
    private readonly residentContinuity: ResidentContinuity,
  ) {
    const context = canvas.getContext('2d');
    if (!context) throw new Error('Canvas 2D context unavailable');
    this.ctx = context;
  }

  draw(options: LegacyCanvasWorldDrawOptions): void {
    const { canvas, ctx, characterLayer, residentContinuity } = this;
    const {
      terrain,
      residents: residentSnapshot,
      centerX,
      centerY,
      zoom,
      angle,
    } = options;

    const renderStartedAt = performance.now();
      const width = canvas.width;
      const height = canvas.height;
      ctx.clearRect(0, 0, width, height);
      ctx.fillStyle = '#08100b';
      ctx.fillRect(0, 0, width, height);
    
      if (!terrain?.available || terrain.chunks.length === 0) {
        ctx.fillStyle = '#91a394';
        ctx.textAlign = 'center';
        ctx.font = `${Math.max(14, width / 55)}px system-ui`;
        ctx.fillText('LifeLensCore world truth loading…', width / 2, height / 2);
        runtimeDiagnostics.recordRender(performance.now() - renderStartedAt);
        return;
      }
    
      const xs = terrain.chunks.map((chunk) => chunk.x);
      const ys = terrain.chunks.map((chunk) => chunk.y);
      const minX = Math.min(...xs);
      const maxX = Math.max(...xs);
      const minY = Math.min(...ys);
      const maxY = Math.max(...ys);
      const extent = Math.max(maxX - minX + 1, maxY - minY + 1);
    
      const elevations = terrain.chunks.map((chunk) => Number(chunk.elevation01) || 0);
      const rawMinElevation = Math.min(...elevations);
      const rawMaxElevation = Math.max(...elevations);
      const reliefRange = Math.max(0.015, rawMaxElevation - rawMinElevation);
      const visualElevation = (elevation: number): number => {
        const normalized = clamp01((elevation - rawMinElevation) / reliefRange);
        const curved = normalized * normalized * (3 - (2 * normalized));
        return 0.16 + (curved * 0.72);
      };
    
      const projector = createWorldProjector({
        width,
        height,
        extent,
        zoom,
        angle,
      });
      const { tile, project } = projector;
      const cos = Math.cos(angle);
      const sin = Math.sin(angle);
    
      const sorted = [...terrain.chunks].sort((a, b) => {
        const da = (a.x - centerX) * sin + (a.y - centerY) * cos;
        const db = (b.x - centerX) * sin + (b.y - centerY) * cos;
        return da - db;
      });
      const chunkMap = new Map(terrain.chunks.map((chunk) => [`${chunk.x}:${chunk.y}`, chunk]));
      const sampleElevation = (x: number, y: number): number | null => {
        const sample = chunkMap.get(`${x}:${y}`);
        return sample ? (Number(sample.elevation01) || 0) : null;
      };
      const cornerElevation = (chunkX: number, chunkY: number, sx: number, sy: number): number => {
        const samples = [
          sampleElevation(chunkX, chunkY),
          sampleElevation(chunkX + sx, chunkY),
          sampleElevation(chunkX, chunkY + sy),
          sampleElevation(chunkX + sx, chunkY + sy),
        ].filter((value): value is number => value !== null);
        const average = samples.reduce((sum, value) => sum + value, 0) / Math.max(1, samples.length);
        return visualElevation(average);
      };
    
      for (const chunk of sorted) {
        const lx = chunk.x - centerX;
        const ly = chunk.y - centerY;
        const e0 = cornerElevation(chunk.x, chunk.y, -1, -1);
        const e1 = cornerElevation(chunk.x, chunk.y, 1, -1);
        const e2 = cornerElevation(chunk.x, chunk.y, 1, 1);
        const e3 = cornerElevation(chunk.x, chunk.y, -1, 1);
        const e = (e0 + e1 + e2 + e3) * 0.25;
        const p0 = project(lx - 0.5, ly - 0.5, e0);
        const p1 = project(lx + 0.5, ly - 0.5, e1);
        const p2 = project(lx + 0.5, ly + 0.5, e2);
        const p3 = project(lx - 0.5, ly + 0.5, e3);
        const base = terrainColor(chunk);
        const slopeLight = ((e0 + e3) - (e1 + e2)) * 0.34 + ((e0 + e1) - (e2 + e3)) * 0.22;
        const isOpenWater = chunk.waterKind === 'Ocean' || chunk.waterKind === 'Coast';
        const surfaceVariation = isOpenWater
          ? 0
          : (presentationHash01(
            terrain.worldSeed ?? '0',
            chunk.x,
            chunk.y,
            0,
            'ground',
          ) - 0.5) * 0.055;
        const light = isOpenWater
          ? 0.92
          : Math.max(
            0.72,
            Math.min(1.16, 0.91 + slopeLight * 0.72 + (e * 0.08) + surfaceVariation),
          );
    
        ctx.beginPath();
        ctx.moveTo(...p0);
        ctx.lineTo(...p1);
        ctx.lineTo(...p2);
        ctx.lineTo(...p3);
        ctx.closePath();
        ctx.fillStyle = shade(base, light);
        ctx.fill();
        if (zoom >= 1.45 && !isOpenWater) {
          ctx.strokeStyle = 'rgba(8,18,12,.012)';
          ctx.lineWidth = Math.max(0.22, canvas.width / 3200);
          ctx.stroke();
        }
    
        const flowLike = (value: WaterKind): boolean => (
          value === 'Spring'
          || value === 'Stream'
          || value === 'River'
        );
        const streamVisibilityThreshold = zoom < 0.78
          ? 0.7
          : zoom < 1.05
            ? 0.5
            : zoom < 1.35
              ? 0.34
              : 0.18;
        const shouldRenderFlow = (
          kind: WaterKind,
          availability: number,
        ): boolean => {
          if (kind === 'River') return true;
          if (kind === 'Stream') {
            return clamp01(availability) >= streamVisibilityThreshold;
          }
          if (kind === 'Spring') {
            return zoom >= 1.18 && clamp01(availability) >= 0.42;
          }
          return false;
        };
        if (
          shouldRenderFlow(
            chunk.waterKind,
            Number(chunk.waterAvailability) || 0,
          )
        ) {
          const [cx, cy] = project(lx, ly, e + 0.014);
          const currentElevation = Number(chunk.elevation01) || 0;
          const candidates = ([
            [1, 0], [-1, 0], [0, 1], [0, -1],
          ] as Array<[number, number]>)
            .map(([dx, dy]) => {
              const neighbor = chunkMap.get(`${chunk.x + dx}:${chunk.y + dy}`);
              if (!neighbor) return null;
              const connectsToFlow = (
                flowLike(neighbor.waterKind)
                && shouldRenderFlow(
                  neighbor.waterKind,
                  Number(neighbor.waterAvailability) || 0,
                )
              );
              const connectsToLake = (
                neighbor.waterKind === 'Lake'
                && chunk.waterKind !== 'Spring'
              );
              if (!connectsToFlow && !connectsToLake) return null;
              return {
                dx,
                dy,
                elevation: Number(neighbor.elevation01) || 0,
                kind: neighbor.waterKind,
              };
            })
            .filter((candidate): candidate is {
              dx: number;
              dy: number;
              elevation: number;
              kind: WaterKind;
            } => candidate !== null);

          const exits: Array<{ dx: number; dy: number }> = [];
          if (candidates.length > 0) {
            const downstream = [...candidates].sort((a, b) => (
              a.elevation - b.elevation
            ))[0];
            exits.push(downstream);

            if (chunk.waterKind !== 'Spring' && candidates.length > 1) {
              const remaining = candidates.filter((candidate) => candidate !== downstream);
              const upstream = [...remaining].sort((a, b) => {
                const aDelta = Math.abs(a.elevation - currentElevation);
                const bDelta = Math.abs(b.elevation - currentElevation);
                return bDelta - aDelta;
              })[0];
              if (upstream) exits.push(upstream);
            }
          }

          ctx.strokeStyle = chunk.waterKind === 'River'
            ? 'rgba(47,123,154,.88)'
            : 'rgba(72,146,172,.72)';
          ctx.lineWidth = Math.max(
            1.15,
            Math.min(
              tile * (chunk.waterKind === 'River' ? 0.11 : 0.055),
              chunk.waterKind === 'River' ? 6 : 3.4,
            ),
          );
          ctx.lineCap = 'round';
          ctx.lineJoin = 'round';

          for (const { dx, dy } of exits) {
            const [ex, ey] = project(
              lx + (dx * 0.52),
              ly + (dy * 0.52),
              e + 0.012,
            );
            const bend = (
              presentationHash01(
                terrain.worldSeed ?? '0',
                chunk.x,
                chunk.y,
                (dx + 2) * 7 + dy,
                'water-bend',
              ) - 0.5
            ) * tile * 0.16;
            ctx.beginPath();
            ctx.moveTo(cx, cy);
            ctx.quadraticCurveTo(
              (cx + ex) * 0.5 + bend,
              (cy + ey) * 0.5 - bend * 0.28,
              ex,
              ey,
            );
            ctx.stroke();
          }

          if (chunk.waterKind === 'Spring') {
            ctx.fillStyle = 'rgba(87,170,194,.9)';
            ctx.beginPath();
            ctx.arc(
              cx,
              cy,
              Math.max(2.1, Math.min(tile * 0.09, 5.5)),
              0,
              Math.PI * 2,
            );
            ctx.fill();
          }
        } else if (chunk.waterKind === 'Lake') {
          const points: Array<[number, number]> = [];
          for (let i = 0; i < 12; i += 1) {
            const a = (Math.PI * 2 * i) / 12;
            const jitter = 0.29 + presentationHash01(terrain.worldSeed ?? '0', chunk.x, chunk.y, i, 'lake-edge') * 0.13;
            points.push(project(lx + Math.cos(a) * jitter, ly + Math.sin(a) * jitter, e + 0.01));
          }
          ctx.fillStyle = 'rgba(45,113,137,.9)';
          ctx.beginPath();
          ctx.moveTo(...points[0]);
          for (let i = 1; i < points.length; i += 1) ctx.lineTo(...points[i]);
          ctx.closePath();
          ctx.fill();
        }
    
        if (chunk.waterKind !== 'Ocean') {
          const seed = terrain.worldSeed ?? '0';
          const forest = clamp01(chunk.forestCoverage01);
          const shrubs = clamp01(chunk.shrubCoverage01);
          const rocks = clamp01(chunk.rockCoverage01);
          const wetland = clamp01(chunk.wetlandCoverage01);
    
          if (zoom < 1.18 && forest >= 0.18) {
            const canopyCount = Math.min(3, 1 + Math.floor(forest * 2.4));
            for (let i = 0; i < canopyCount; i += 1) {
              const ox = (
                presentationHash01(seed, chunk.x, chunk.y, i, 'canopy-x') - 0.5
              ) * 0.54;
              const oy = (
                presentationHash01(seed, chunk.x, chunk.y, i, 'canopy-y') - 0.5
              ) * 0.54;
              const [fx, fy] = project(lx + ox, ly + oy, e + 0.008);
              const radius = Math.max(
                2.2,
                tile * (0.16 + forest * 0.13),
              );
              ctx.fillStyle = i % 2 === 0
                ? 'rgba(24,62,32,.62)'
                : 'rgba(34,75,39,.54)';
              ctx.beginPath();
              ctx.ellipse(
                fx,
                fy,
                radius * 1.25,
                radius * 0.72,
                -0.18,
                0,
                Math.PI * 2,
              );
              ctx.fill();
            }
          }

          const cluster = presentationHash01(seed, chunk.x, chunk.y, 0, 'forest-cluster');
          const treeCount = zoom < 1.18 || forest < 0.14 || cluster < 0.14
            ? 0
            : Math.min(
              10,
              2 + Math.floor(forest * 8) + (zoom >= 1.7 ? 1 : 0),
            );
          for (let i = 0; i < treeCount; i += 1) {
            const ox = (presentationHash01(seed, chunk.x, chunk.y, i, 'tree-x') - 0.5) * 0.78;
            const oy = (presentationHash01(seed, chunk.x, chunk.y, i, 'tree-y') - 0.5) * 0.78;
            const [tx, ty] = project(lx + ox, ly + oy, e + 0.012);
            const size = Math.max(
              4.8,
              Math.min(
                tile * 0.72,
                tile
                  * (0.38 + forest * 0.18)
                  * (0.82 + presentationHash01(seed, chunk.x, chunk.y, i, 'tree-size') * 0.42),
              ),
            );
            ctx.strokeStyle = '#4b3926';
            ctx.lineWidth = Math.max(0.8, size * 0.16);
            ctx.beginPath();
            ctx.moveTo(tx, ty + size * 0.42);
            ctx.lineTo(tx, ty - size * 0.58);
            ctx.stroke();
            ctx.fillStyle = presentationHash01(seed, chunk.x, chunk.y, i, 'tree-tone') > 0.48 ? '#183e20' : '#214b27';
            ctx.beginPath();
            ctx.moveTo(tx, ty - size * 1.15);
            ctx.lineTo(tx + size * 0.72, ty - size * 0.08);
            ctx.lineTo(tx - size * 0.72, ty - size * 0.08);
            ctx.closePath();
            ctx.fill();
            ctx.beginPath();
            ctx.moveTo(tx, ty - size * 0.76);
            ctx.lineTo(tx + size * 0.58, ty + size * 0.34);
            ctx.lineTo(tx - size * 0.58, ty + size * 0.34);
            ctx.closePath();
            ctx.fill();
          }
    
          if (shrubs >= 0.34) {
            const [sx, sy] = project(lx + 0.24, ly - 0.18, e + 0.008);
            ctx.fillStyle = '#52743a';
            ctx.beginPath();
            ctx.arc(sx, sy, Math.max(1.5, tile * (0.035 + shrubs * 0.03)), 0, Math.PI * 2);
            ctx.fill();
          }
    
          if (rocks >= 0.38) {
            const [rx, ry] = project(lx - 0.22, ly + 0.18, e + 0.008);
            const size = Math.max(1.5, tile * (0.03 + rocks * 0.035));
            ctx.fillStyle = '#817f77';
            ctx.beginPath();
            ctx.moveTo(rx, ry - size);
            ctx.lineTo(rx + size, ry);
            ctx.lineTo(rx, ry + size * 0.55);
            ctx.lineTo(rx - size, ry);
            ctx.closePath();
            ctx.fill();
          }
    
          if (wetland >= 0.48 && chunk.waterKind === 'None') {
            const [wx, wy] = project(lx, ly, e + 0.004);
            ctx.strokeStyle = 'rgba(70,135,116,.82)';
            ctx.lineWidth = Math.max(1, tile * 0.035);
            ctx.beginPath();
            ctx.ellipse(wx, wy, Math.max(2, tile * 0.18), Math.max(1, tile * 0.06), 0, 0, Math.PI * 2);
            ctx.stroke();
          }
        }
    
        if (chunk.x === centerX && chunk.y === centerY) {
          ctx.beginPath();
          ctx.moveTo(...p0);
          ctx.lineTo(...p1);
          ctx.lineTo(...p2);
          ctx.lineTo(...p3);
          ctx.closePath();
          ctx.strokeStyle = 'rgba(238,247,235,.95)';
          ctx.lineWidth = Math.max(2, canvas.width / 600);
          ctx.stroke();
        }
      }
    
      const grad = ctx.createLinearGradient(0, 0, 0, height);
      grad.addColorStop(0, 'rgba(8,15,10,.04)');
      grad.addColorStop(1, 'rgba(2,7,4,.42)');
      ctx.fillStyle = grad;
      ctx.fillRect(0, 0, width, height);
    
      const displayDpr = Math.min(window.devicePixelRatio || 1, 2);
      const tileCssPx = tile / displayDpr;
      // A Core chunk represents tens of metres. Keep humans at map scale instead
      // of making them billboard-sized; mature trees should read several times taller.
      const characterHeightCssPx = Math.max(
        3.2,
        Math.min(18, tileCssPx * 0.075),
      );
      const characterHeightDevicePx = characterHeightCssPx * displayDpr;
      const avatarRadius = Math.max(
        3 * displayDpr,
        characterHeightDevicePx * 0.3,
      );
      const residentRadius = Math.max(
        4 * displayDpr,
        characterHeightDevicePx * 0.38,
      );
      const fallbackElevation = visualElevation((rawMinElevation + rawMaxElevation) * 0.5);
      const projectedResidents = residentSnapshot.flatMap((resident, index) => {
        const position = residentContinuity.positionFor(resident);
        if (!position) return [];
        const chunkX = Math.floor(position.x / 32);
        const chunkY = Math.floor(position.y / 32);
        const residentChunk = chunkMap.get(`${chunkX}:${chunkY}`);
        const localX = ((position.x - (chunkX * 32)) / 32) - 0.5;
        const localY = ((position.y - (chunkY * 32)) / 32) - 0.5;
        const residentElevation = residentChunk
          ? visualElevation(Number(residentChunk.elevation01) || 0)
          : fallbackElevation;
        const [rawX, rawY] = project(
          (chunkX - centerX) + localX,
          (chunkY - centerY) + localY,
          residentElevation + 0.035,
        );
        return [{ resident, rawX, rawY, index, hasTerrain: Boolean(residentChunk) }];
      });
    
      const placedResidents: Array<{ x: number; y: number }> = [];
      const placedLabels: Array<{
        left: number;
        right: number;
        top: number;
        bottom: number;
      }> = [];
      const characterPlacements: ResidentPlacement[] = [];
      for (const projected of projectedResidents) {
        const anchorX = projected.rawX;
        const anchorY = projected.rawY;
        const nearby = placedResidents.filter((placed) => Math.hypot(placed.x - anchorX, placed.y - anchorY) < residentRadius * 3.0).length;
        const ring = 1 + Math.floor(nearby / 6);
        const slot = nearby % 6;
        const offsetAngle = (Math.PI * 2 * slot) / 6 + (projected.index * 0.37);
        const separation = nearby === 0 ? 0 : residentRadius * 1.8 * ring;
        const px = anchorX + Math.cos(offsetAngle) * separation;
        const py = anchorY + Math.sin(offsetAngle) * separation;
        placedResidents.push({ x: px, y: py });
    
        const displaced = Math.hypot(px - projected.rawX, py - projected.rawY) > residentRadius * 0.85;
        if (displaced) {
          ctx.strokeStyle = projected.hasTerrain ? 'rgba(222,235,218,.42)' : 'rgba(119,186,203,.68)';
          ctx.lineWidth = Math.max(1.5 * displayDpr, avatarRadius * 0.07);
          ctx.setLineDash([Math.max(3 * displayDpr, avatarRadius * 0.22), Math.max(3 * displayDpr, avatarRadius * 0.18)]);
          ctx.beginPath();
          ctx.moveTo(Math.max(0, Math.min(width, projected.rawX)), Math.max(0, Math.min(height, projected.rawY)));
          ctx.lineTo(px, py);
          ctx.stroke();
          ctx.setLineDash([]);
        }
    
        characterPlacements.push({
          id: projected.resident.id,
          name: projected.resident.name,
          sex: projected.resident.sex === 'Female' ? 'Female' : 'Male',
          activityLabel: projected.resident.activityLabel || 'Idle',
          x: px / displayDpr,
          y: py / displayDpr,
          index: projected.index,
          heightPx: characterHeightCssPx,
        });
    
        if (!characterLayer.ready) {
          const accentPalette = ['#75a88b', '#b39878', '#8298bd', '#b3849a', '#91a96b', '#9b8eb7'];
          const accentIndex = Math.floor(presentationHash01(terrain.worldSeed ?? '0', 0, 0, projected.index, `resident-${projected.resident.id}`) * accentPalette.length) % accentPalette.length;
          const accent = accentPalette[accentIndex];
    
          ctx.fillStyle = 'rgba(7,15,10,.92)';
          ctx.strokeStyle = accent;
          ctx.lineWidth = Math.max(2 * displayDpr, avatarRadius * 0.10);
          ctx.beginPath();
          ctx.arc(px, py, avatarRadius * 0.72, 0, Math.PI * 2);
          ctx.fill();
          ctx.stroke();
          ctx.fillStyle = accent;
          ctx.beginPath();
          ctx.arc(px, py - avatarRadius * 0.12, avatarRadius * 0.2, 0, Math.PI * 2);
          ctx.fill();
        }
    
        const fontSize = Math.max(
          7.1 * displayDpr,
          Math.min(9.8 * displayDpr, characterHeightDevicePx * 0.2),
        );
        ctx.font = `700 ${fontSize}px system-ui`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        const labelHeight = Math.max(14.5 * displayDpr, fontSize * 1.22);
        const labelWidth = Math.min(
          width * 0.24,
          ctx.measureText(projected.resident.name).width
            + Math.max(10 * displayDpr, avatarRadius * 0.42),
        );

        const headY = characterLayer.ready
          ? py - characterHeightDevicePx
          : py - avatarRadius * 1.45;
        const baseLabelY = headY - labelHeight * 0.62;
        const horizontalGap = Math.max(
          labelWidth * 0.72,
          characterHeightDevicePx * 0.66,
        );
        const verticalLift = Math.max(
          labelHeight * 0.95,
          characterHeightDevicePx * 0.24,
        );

        const candidates = [
          { x: px, y: baseLabelY },
          { x: px - horizontalGap, y: baseLabelY + labelHeight * 0.12 },
          { x: px + horizontalGap, y: baseLabelY + labelHeight * 0.12 },
          { x: px - horizontalGap * 0.82, y: baseLabelY - verticalLift },
          { x: px + horizontalGap * 0.82, y: baseLabelY - verticalLift },
          { x: px, y: baseLabelY - verticalLift },
          { x: px, y: baseLabelY + labelHeight * 1.08 },
        ];

        let labelX = px;
        let labelY = baseLabelY;
        let bestScore = Number.POSITIVE_INFINITY;

        for (const candidate of candidates) {
          const left = candidate.x - labelWidth / 2;
          const right = candidate.x + labelWidth / 2;
          const top = candidate.y - labelHeight / 2;
          const bottom = candidate.y + labelHeight / 2;

          if (
            left < 4 * displayDpr
            || right > width - 4 * displayDpr
            || top < 4 * displayDpr
            || bottom > height - 4 * displayDpr
          ) {
            continue;
          }

          let score = Math.abs(candidate.x - px) * 0.12
            + Math.abs(candidate.y - baseLabelY) * 0.08;

          for (const label of placedLabels) {
            const overlapWidth = Math.max(
              0,
              Math.min(right, label.right) - Math.max(left, label.left),
            );
            const overlapHeight = Math.max(
              0,
              Math.min(bottom, label.bottom) - Math.max(top, label.top),
            );
            score += overlapWidth * overlapHeight * 10;
          }

          const actorLeft = px - characterHeightDevicePx * 0.36;
          const actorRight = px + characterHeightDevicePx * 0.36;
          const actorTop = py - characterHeightDevicePx * 1.04;
          const actorBottom = py + characterHeightDevicePx * 0.08;
          const actorOverlapWidth = Math.max(
            0,
            Math.min(right, actorRight) - Math.max(left, actorLeft),
          );
          const actorOverlapHeight = Math.max(
            0,
            Math.min(bottom, actorBottom) - Math.max(top, actorTop),
          );
          score += actorOverlapWidth * actorOverlapHeight * 16;

          if (score < bestScore) {
            bestScore = score;
            labelX = candidate.x;
            labelY = candidate.y;
          }
        }

        const labelLeft = labelX - labelWidth / 2;
        const labelRight = labelX + labelWidth / 2;
        const labelTop = labelY - labelHeight / 2;
        const labelBottom = labelY + labelHeight / 2;

        placedLabels.push({
          left: labelLeft,
          right: labelRight,
          top: labelTop,
          bottom: labelBottom,
        });

        if (
          Math.abs(labelX - px) > labelWidth * 0.32
          || Math.abs(labelY - baseLabelY) > labelHeight * 0.55
        ) {
          ctx.strokeStyle = 'rgba(228,238,226,.28)';
          ctx.lineWidth = Math.max(0.75 * displayDpr, 1);
          ctx.beginPath();
          ctx.moveTo(labelX, labelY + labelHeight * 0.48);
          ctx.lineTo(px, headY + Math.max(1.5 * displayDpr, 2));
          ctx.stroke();
        }

        ctx.fillStyle = 'rgba(5,12,8,.8)';
        ctx.strokeStyle = 'rgba(221,236,220,.14)';
        ctx.lineWidth = Math.max(0.9 * displayDpr, avatarRadius * 0.025);
        ctx.beginPath();
        ctx.roundRect(
          labelLeft,
          labelTop,
          labelWidth,
          labelHeight,
          Math.max(4.5 * displayDpr, avatarRadius * 0.16),
        );
        ctx.fill();
        ctx.stroke();
        ctx.fillStyle = '#f4f8f2';
        ctx.fillText(projected.resident.name, labelX, labelY);
      }
      characterLayer.setResidents(characterPlacements);
      runtimeDiagnostics.recordRender(performance.now() - renderStartedAt);
  }
}
