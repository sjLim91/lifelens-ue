import * as THREE from 'three';
import type {
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';

const FLOW_KINDS = new Set<WaterKind>([
  'Spring',
  'Stream',
  'River',
  'Lake',
  'Wetland',
]);

function connectedDirections(
  chunk: TerrainChunk,
  window: TerrainWindow,
): Array<[number, number]> {
  const map = new Map(
    window.chunks.map((entry) => [
      `${entry.x}:${entry.y}`,
      entry,
    ]),
  );

  return ([
    [1, 0],
    [-1, 0],
    [0, 1],
    [0, -1],
  ] as Array<[number, number]>).filter(([dx, dy]) => {
    const neighbor = map.get(`${chunk.x + dx}:${chunk.y + dy}`);
    return neighbor ? FLOW_KINDS.has(neighbor.waterKind) : false;
  });
}

export function buildWaterGeometry(
  chunk: TerrainChunk,
  window: TerrainWindow,
  chunkWorldSize = 8,
): THREE.BufferGeometry {
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

  const connected = connectedDirections(chunk, window);
  const directions = connected.length > 0
    ? connected
    : ([[1, 0], [-1, 0]] as Array<[number, number]>);

  const width = chunk.waterKind === 'River'
    ? chunkWorldSize * 0.22
    : chunk.waterKind === 'Stream'
      ? chunkWorldSize * 0.11
      : chunkWorldSize * 0.08;

  const shape = new THREE.Shape();
  shape.moveTo(-width * 0.5, -width * 0.5);
  shape.lineTo(width * 0.5, -width * 0.5);
  shape.lineTo(width * 0.5, width * 0.5);
  shape.lineTo(-width * 0.5, width * 0.5);
  shape.closePath();

  for (const [dx, dy] of directions) {
    const half = chunkWorldSize * 0.52;
    const ex = dx * half;
    const ey = dy * half;
    const sideX = -dy * width * 0.5;
    const sideY = dx * width * 0.5;

    shape.lineTo(ex + sideX, ey + sideY);
    shape.lineTo(ex - sideX, ey - sideY);
    shape.lineTo(width * 0.5, width * 0.5);
  }

  const geometry = new THREE.ShapeGeometry(shape);
  geometry.rotateX(-Math.PI / 2);
  return geometry;
}
