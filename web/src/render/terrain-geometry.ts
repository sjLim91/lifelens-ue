import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainColorSampler } from './terrain-surface';

export interface TerrainGeometryOptions {
  chunkWorldSize?: number;
  elevationScale?: number;
}

function average(values: Array<number | null>, fallback: number): number {
  const valid = values.filter((value): value is number => value !== null);
  if (valid.length === 0) return fallback;
  return valid.reduce((sum, value) => sum + value, 0) / valid.length;
}

function surfaceShade(
  seed: string,
  chunkX: number,
  chunkY: number,
  vertexX: number,
  vertexY: number,
): number {
  const px = (chunkX * 10 + vertexX) / 3;
  const py = (chunkY * 10 + vertexY) / 3;
  const ix = Math.floor(px), iy = Math.floor(py);
  const noise = (patchX: number, patchY: number): number => {
    const input = `${seed}:surface:${patchX}:${patchY}`;
    let hash = 2166136261 >>> 0;
    for (let index = 0; index < input.length; index += 1) {
      hash ^= input.charCodeAt(index);
      hash = Math.imul(hash, 16777619) >>> 0;
    }
    hash ^= hash >>> 16;
    const noise01 = (hash >>> 0) / 4294967295;
    return noise01;
  };
  const smooth = (t: number): number => t * t * (3 - 2 * t);
  const x = smooth(px - ix), y = smooth(py - iy);
  const north = noise(ix, iy) * (1 - x) + noise(ix + 1, iy) * x;
  const south = noise(ix, iy + 1) * (1 - x) + noise(ix + 1, iy + 1) * x;
  return 0.93 + (north * (1 - y) + south * y) * 0.1;
}

export function createTerrainElevationSampler(
  window: TerrainWindow,
): (
  chunkX: number,
  chunkY: number,
  localX01: number,
  localY01: number,
) => number {
  const elevations = new Map(
    window.chunks.map((chunk) => [
      `${chunk.x}:${chunk.y}`,
      Number(chunk.elevation01) || 0,
    ]),
  );

  const elevationAt = (x: number, y: number): number | null => (
    elevations.get(`${x}:${y}`) ?? null
  );

  return (
    chunkX: number,
    chunkY: number,
    localX01: number,
    localY01: number,
  ): number => {
    const center = elevationAt(chunkX, chunkY) ?? 0;
    const corner = (sx: number, sy: number): number => average(
      [
        elevationAt(chunkX, chunkY),
        elevationAt(chunkX + sx, chunkY),
        elevationAt(chunkX, chunkY + sy),
        elevationAt(chunkX + sx, chunkY + sy),
      ],
      center,
    );

    const h00 = corner(-1, -1);
    const h10 = corner(1, -1);
    const h11 = corner(1, 1);
    const h01 = corner(-1, 1);
    const tx = Math.max(0, Math.min(1, localX01));
    const ty = Math.max(0, Math.min(1, localY01));
    const north = h00 + ((h10 - h00) * tx);
    const south = h01 + ((h11 - h01) * tx);
    return north + ((south - north) * ty);
  };
}

export function createTerrainGeometryBuilder(
  window: TerrainWindow,
  options: TerrainGeometryOptions = {},
): (chunk: TerrainChunk) => THREE.BufferGeometry {
  const chunkWorldSize =
    options.chunkWorldSize ?? WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const elevationScale =
    options.elevationScale ?? WORLD_GRID_CONTRACT.elevationScale;
  const half = chunkWorldSize * 0.5;
  const sampleElevation = createTerrainElevationSampler(window);
  const sampleColor = createTerrainColorSampler(window);
  const color = new THREE.Color();
  const patches = new Map(window.chunks.map(chunk => [
    `${chunk.x}:${chunk.y}`,
    [sampleElevation(chunk.x, chunk.y, 0, 0), sampleElevation(chunk.x, chunk.y, 1, 0),
      sampleElevation(chunk.x, chunk.y, 0, 1), sampleElevation(chunk.x, chunk.y, 1, 1)],
  ]));
  const normal = new THREE.Vector3();
  const sampleNormal = (x: number, y: number): THREE.Vector3 => {
    const xs = Number.isInteger(x) ? [x - 1, x] : [Math.floor(x)];
    const ys = Number.isInteger(y) ? [y - 1, y] : [Math.floor(y)];
    let dx = 0, dy = 0, count = 0;
    for (const cy of ys) for (const cx of xs) {
      const heights = patches.get(`${cx}:${cy}`);
      if (!heights) continue;
      const [h00, h10, h01, h11] = heights;
      const tx = x - cx, ty = y - cy;
      dx += (h10 - h00) * (1 - ty) + (h11 - h01) * ty;
      dy += (h01 - h00) * (1 - tx) + (h11 - h10) * tx;
      count += 1;
    }
    const scale = elevationScale / (chunkWorldSize * Math.max(1, count));
    return normal.set(-dx * scale, 1, -dy * scale).normalize();
  };

  return (chunk: TerrainChunk): THREE.BufferGeometry => {
    const subdivisions = 10;
    const verticesPerSide = subdivisions + 1;
    const positions: number[] = [];
    const uvs: number[] = [];
    const colors: number[] = [];
    const normals: number[] = [];
    const indices: number[] = [];
    const worldSeed = window.worldSeed ?? '0';

    for (let y = 0; y <= subdivisions; y += 1) {
      const ty = y / subdivisions;
      const z = -half + (ty * chunkWorldSize);

      for (let x = 0; x <= subdivisions; x += 1) {
        const tx = x / subdivisions;
        const px = -half + (tx * chunkWorldSize);
        const elevation = sampleElevation(
          chunk.x,
          chunk.y,
          tx,
          ty,
        ) * elevationScale;

        positions.push(px, elevation, z);
        uvs.push(tx, ty);

        const shade = surfaceShade(
          worldSeed,
          chunk.x,
          chunk.y,
          x,
          y,
        );
        sampleColor(chunk.x, chunk.y, tx, ty, color).multiplyScalar(shade);
        colors.push(color.r, color.g, color.b);
        sampleNormal(chunk.x + tx, chunk.y + ty);
        normals.push(normal.x, normal.y, normal.z);
      }
    }

    for (let y = 0; y < subdivisions; y += 1) {
      for (let x = 0; x < subdivisions; x += 1) {
        const a = (y * verticesPerSide) + x;
        const b = a + 1;
        const d = ((y + 1) * verticesPerSide) + x;
        const c = d + 1;

        indices.push(a, c, b, a, d, c);
      }
    }

    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute(
      'position',
      new THREE.Float32BufferAttribute(positions, 3),
    );
    geometry.setAttribute(
      'uv',
      new THREE.Float32BufferAttribute(uvs, 2),
    );
    geometry.setAttribute(
      'color',
      new THREE.Float32BufferAttribute(colors, 3),
    );
    geometry.setIndex(indices);
    geometry.setAttribute('normal', new THREE.Float32BufferAttribute(normals, 3));
    geometry.computeBoundingBox();
    geometry.computeBoundingSphere();
    return geometry;
  };
}

export function buildChunkGeometry(
  chunk: TerrainChunk,
  window: TerrainWindow,
  options: TerrainGeometryOptions = {},
): THREE.BufferGeometry {
  return createTerrainGeometryBuilder(window, options)(chunk);
}
