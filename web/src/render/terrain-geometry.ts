import * as THREE from 'three';
import type { TerrainChunk, TerrainWindow } from '../runtime/core-types';

export interface TerrainGeometryOptions {
  chunkWorldSize?: number;
  elevationScale?: number;
}

function average(values: Array<number | null>, fallback: number): number {
  const valid = values.filter((value): value is number => value !== null);
  if (valid.length === 0) return fallback;
  return valid.reduce((sum, value) => sum + value, 0) / valid.length;
}

export function createTerrainGeometryBuilder(
  window: TerrainWindow,
  options: TerrainGeometryOptions = {},
): (chunk: TerrainChunk) => THREE.BufferGeometry {
  const chunkWorldSize = options.chunkWorldSize ?? 8;
  const elevationScale = options.elevationScale ?? 48;
  const half = chunkWorldSize * 0.5;
  const elevations = new Map(
    window.chunks.map((chunk) => [
      `${chunk.x}:${chunk.y}`,
      Number(chunk.elevation01) || 0,
    ]),
  );

  const elevationAt = (x: number, y: number): number | null => (
    elevations.get(`${x}:${y}`) ?? null
  );

  return (chunk: TerrainChunk): THREE.BufferGeometry => {
    const center = Number(chunk.elevation01) || 0;
    const corner = (sx: number, sy: number): number => average(
      [
        elevationAt(chunk.x, chunk.y),
        elevationAt(chunk.x + sx, chunk.y),
        elevationAt(chunk.x, chunk.y + sy),
        elevationAt(chunk.x + sx, chunk.y + sy),
      ],
      center,
    ) * elevationScale;

    const h00 = corner(-1, -1);
    const h10 = corner(1, -1);
    const h11 = corner(1, 1);
    const h01 = corner(-1, 1);

    const positions = new Float32Array([
      -half, h00, -half,
       half, h10, -half,
       half, h11,  half,
      -half, h01,  half,
    ]);
    const indices = [0, 2, 1, 0, 3, 2];
    const uvs = new Float32Array([
      0, 0,
      1, 0,
      1, 1,
      0, 1,
    ]);

    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute(
      'position',
      new THREE.BufferAttribute(positions, 3),
    );
    geometry.setAttribute('uv', new THREE.BufferAttribute(uvs, 2));
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
