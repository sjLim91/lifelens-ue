import * as THREE from 'three';
import type {
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

const FLOW_KINDS = new Set<WaterKind>([
  'Spring',
  'Stream',
  'River',
  'Lake',
  'Wetland',
]);

export function createWaterGeometryBuilder(
  window: TerrainWindow,
  chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk,
): (chunk: TerrainChunk) => THREE.BufferGeometry {
  const map = new Map(
    window.chunks.map((entry) => [
      `${entry.x}:${entry.y}`,
      entry,
    ]),
  );

  const connectedDirections = (
    chunk: TerrainChunk,
  ): Array<[number, number]> => (
    ([
      [1, 0],
      [-1, 0],
      [0, 1],
      [0, -1],
    ] as Array<[number, number]>).filter(([dx, dy]) => {
      const neighbor = map.get(`${chunk.x + dx}:${chunk.y + dy}`);
      return neighbor ? FLOW_KINDS.has(neighbor.waterKind) : false;
    })
  );

  return (chunk: TerrainChunk): THREE.BufferGeometry => {
    if (
      chunk.waterKind === 'Ocean'
      || chunk.waterKind === 'Coast'
      || chunk.waterKind === 'Lake'
      || chunk.waterKind === 'Wetland'
    ) {
      const plane = new THREE.PlaneGeometry(
        chunkWorldSize * 0.96,
        chunkWorldSize * 0.96,
      );
      plane.rotateX(-Math.PI / 2);
      return plane;
    }

    const connected = connectedDirections(chunk);
    const directions = connected.length > 0
      ? connected
      : ([[1, 0], [-1, 0]] as Array<[number, number]>);

    const width = chunk.waterKind === 'River'
      ? chunkWorldSize * 0.22
      : chunk.waterKind === 'Stream'
        ? chunkWorldSize * 0.11
        : chunkWorldSize * 0.08;
    const halfWidth = width * 0.5;
    const halfLength = chunkWorldSize * 0.52;

    const positions: number[] = [];
    const indices: number[] = [];

    const addQuad = (
      ax: number,
      az: number,
      bx: number,
      bz: number,
      cx: number,
      cz: number,
      dx: number,
      dz: number,
    ): void => {
      const base = positions.length / 3;
      positions.push(
        ax, 0, az,
        bx, 0, bz,
        cx, 0, cz,
        dx, 0, dz,
      );
      indices.push(
        base, base + 2, base + 1,
        base, base + 3, base + 2,
      );
    };

    addQuad(
      -halfWidth, -halfWidth,
       halfWidth, -halfWidth,
       halfWidth,  halfWidth,
      -halfWidth,  halfWidth,
    );

    for (const [dx, dy] of directions) {
      const endX = dx * halfLength;
      const endZ = dy * halfLength;
      const sideX = -dy * halfWidth;
      const sideZ = dx * halfWidth;

      addQuad(
        -sideX, -sideZ,
         sideX,  sideZ,
         endX + sideX, endZ + sideZ,
         endX - sideX, endZ - sideZ,
      );
    }

    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute(
      'position',
      new THREE.Float32BufferAttribute(positions, 3),
    );
    geometry.setIndex(indices);
    geometry.computeVertexNormals();
    geometry.computeBoundingBox();
    geometry.computeBoundingSphere();
    return geometry;
  };
}

export function buildWaterGeometry(
  chunk: TerrainChunk,
  window: TerrainWindow,
  chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk,
): THREE.BufferGeometry {
  return createWaterGeometryBuilder(window, chunkWorldSize)(chunk);
}
