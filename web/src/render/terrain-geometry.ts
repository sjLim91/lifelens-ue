import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

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
  const patchX = Math.floor((chunkX * 10 + vertexX) / 2);
  const patchY = Math.floor((chunkY * 10 + vertexY) / 2);
  const input = `${seed}:surface:${patchX}:${patchY}`;
  let hash = 2166136261 >>> 0;
  for (let index = 0; index < input.length; index += 1) {
    hash ^= input.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  const noise01 = (hash >>> 0) / 4294967295;
  return 0.88 + noise01 * 0.16;
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

  return (chunk: TerrainChunk): THREE.BufferGeometry => {
    const subdivisions = 10;
    const verticesPerSide = subdivisions + 1;
    const positions: number[] = [];
    const uvs: number[] = [];
    const colors: number[] = [];
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
        colors.push(shade, shade, shade);
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
    geometry.computeVertexNormals();
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
