import * as THREE from 'three';
import type {
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

const OPEN_WATER_KINDS = new Set<WaterKind>([
  'Ocean',
  'Coast',
  'Lake',
]);

const CONNECTED_WATER_KINDS = new Set<WaterKind>([
  'Spring',
  'Stream',
  'River',
  'Lake',
  'Coast',
  'Ocean',
]);

interface OpenWaterNode {
  chunk: TerrainChunk;
  levelWorldY: number;
  color: THREE.Color;
}

interface SurfacePoint {
  x: number;
  y: number;
  z: number;
  color: THREE.Color;
}

type PointToken =
  | 'a'
  | 'b'
  | 'c'
  | 'd'
  | 'ab'
  | 'bc'
  | 'cd'
  | 'da';

const WATER_COLORS: Record<'Ocean' | 'Coast' | 'Lake', number> = {
  Ocean: 0x174b67,
  Coast: 0x2f7487,
  Lake: 0x2e7188,
};

const MARCHING_POLYGONS: Record<number, PointToken[][]> = {
  0: [],
  1: [['a', 'ab', 'da']],
  2: [['b', 'bc', 'ab']],
  3: [['a', 'b', 'bc', 'da']],
  4: [['c', 'cd', 'bc']],
  5: [
    ['a', 'ab', 'da'],
    ['c', 'cd', 'bc'],
  ],
  6: [['b', 'c', 'cd', 'ab']],
  7: [['a', 'b', 'c', 'cd', 'da']],
  8: [['d', 'da', 'cd']],
  9: [['a', 'ab', 'cd', 'd']],
  10: [
    ['b', 'bc', 'ab'],
    ['d', 'da', 'cd'],
  ],
  11: [['a', 'b', 'bc', 'cd', 'd']],
  12: [['d', 'da', 'bc', 'c']],
  13: [['a', 'ab', 'bc', 'c', 'd']],
  14: [['b', 'c', 'd', 'da', 'ab']],
  15: [['a', 'b', 'c', 'd']],
};

function key(x: number, y: number): string {
  return `${x}:${y}`;
}

export function isOpenWaterSurfaceKind(
  kind: WaterKind,
): kind is 'Ocean' | 'Coast' | 'Lake' {
  return OPEN_WATER_KINDS.has(kind);
}

function buildOpenWaterNodes(
  window: TerrainWindow,
): Map<string, OpenWaterNode> {
  const chunks = new Map(
    window.chunks.map((chunk) => [key(chunk.x, chunk.y), chunk]),
  );
  const openChunks = window.chunks.filter(
    (chunk) => isOpenWaterSurfaceKind(chunk.waterKind),
  );
  const componentByKey = new Map<string, number>();
  const components: TerrainChunk[][] = [];

  for (const start of openChunks) {
    const startKey = key(start.x, start.y);
    if (componentByKey.has(startKey)) continue;

    const componentIndex = components.length;
    const component: TerrainChunk[] = [];
    const queue: TerrainChunk[] = [start];
    componentByKey.set(startKey, componentIndex);

    while (queue.length > 0) {
      const current = queue.shift();
      if (!current) break;
      component.push(current);

      for (const [dx, dy] of (
        [[1, 0], [-1, 0], [0, 1], [0, -1]] as Array<[number, number]>
      )) {
        const neighbor = chunks.get(
          key(current.x + dx, current.y + dy),
        );
        if (
          !neighbor
          || !isOpenWaterSurfaceKind(neighbor.waterKind)
        ) {
          continue;
        }

        const neighborKey = key(neighbor.x, neighbor.y);
        if (componentByKey.has(neighborKey)) continue;
        componentByKey.set(neighborKey, componentIndex);
        queue.push(neighbor);
      }
    }

    components.push(component);
  }

  const levelByComponent = components.map((component) => {
    const ocean = component.filter(
      (chunk) => chunk.waterKind === 'Ocean',
    );
    const levelSources = ocean.length > 0 ? ocean : component;
    const averageElevation = levelSources.reduce(
      (sum, chunk) => sum + (Number(chunk.elevation01) || 0),
      0,
    ) / Math.max(1, levelSources.length);

    return (
      averageElevation * WORLD_GRID_CONTRACT.elevationScale
      + 0.08
    );
  });

  const nodes = new Map<string, OpenWaterNode>();
  for (const chunk of openChunks) {
    if (!isOpenWaterSurfaceKind(chunk.waterKind)) continue;
    const componentIndex = componentByKey.get(key(chunk.x, chunk.y)) ?? 0;
    nodes.set(key(chunk.x, chunk.y), {
      chunk,
      levelWorldY: levelByComponent[componentIndex] ?? 0.08,
      color: new THREE.Color(WATER_COLORS[chunk.waterKind]),
    });
  }
  return nodes;
}

function mixPoint(
  first: OpenWaterNode | undefined,
  second: OpenWaterNode | undefined,
  x: number,
  z: number,
): SurfacePoint {
  const sources = [first, second].filter(
    (entry): entry is OpenWaterNode => Boolean(entry),
  );
  const levelWorldY = sources.reduce(
    (sum, entry) => sum + entry.levelWorldY,
    0,
  ) / Math.max(1, sources.length);

  const color = new THREE.Color(0x2e7188);
  if (sources.length > 0) {
    color.copy(sources[0].color);
    for (let index = 1; index < sources.length; index += 1) {
      color.lerp(sources[index].color, 1 / (index + 1));
    }
  }

  return {
    x,
    y: levelWorldY,
    z,
    color,
  };
}

export function buildOpenWaterSurfaceGeometry(
  window: TerrainWindow,
  chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk,
): THREE.BufferGeometry {
  const nodes = buildOpenWaterNodes(window);
  const geometry = new THREE.BufferGeometry();

  if (nodes.size === 0) {
    geometry.setAttribute(
      'position',
      new THREE.Float32BufferAttribute([], 3),
    );
    geometry.setAttribute(
      'normal',
      new THREE.Float32BufferAttribute([], 3),
    );
    geometry.setAttribute(
      'color',
      new THREE.Float32BufferAttribute([], 3),
    );
    return geometry;
  }

  const coordinates = [...nodes.values()].map((entry) => entry.chunk);
  const minX = Math.min(...coordinates.map((chunk) => chunk.x));
  const maxX = Math.max(...coordinates.map((chunk) => chunk.x));
  const minY = Math.min(...coordinates.map((chunk) => chunk.y));
  const maxY = Math.max(...coordinates.map((chunk) => chunk.y));

  const positions: number[] = [];
  const normals: number[] = [];
  const colors: number[] = [];

  const worldX = (gridX: number): number => (
    (gridX - window.centerChunkX) * chunkWorldSize
  );
  const worldZ = (gridY: number): number => (
    (gridY - window.centerChunkY) * chunkWorldSize
  );

  for (let y = minY - 1; y <= maxY; y += 1) {
    for (let x = minX - 1; x <= maxX; x += 1) {
      const aNode = nodes.get(key(x, y));
      const bNode = nodes.get(key(x + 1, y));
      const cNode = nodes.get(key(x + 1, y + 1));
      const dNode = nodes.get(key(x, y + 1));
      const mask =
        (aNode ? 1 : 0)
        | (bNode ? 2 : 0)
        | (cNode ? 4 : 0)
        | (dNode ? 8 : 0);

      if (mask === 0) continue;

      const points: Record<PointToken, SurfacePoint> = {
        a: mixPoint(aNode, undefined, worldX(x), worldZ(y)),
        b: mixPoint(bNode, undefined, worldX(x + 1), worldZ(y)),
        c: mixPoint(
          cNode,
          undefined,
          worldX(x + 1),
          worldZ(y + 1),
        ),
        d: mixPoint(dNode, undefined, worldX(x), worldZ(y + 1)),
        ab: mixPoint(
          aNode,
          bNode,
          worldX(x + 0.5),
          worldZ(y),
        ),
        bc: mixPoint(
          bNode,
          cNode,
          worldX(x + 1),
          worldZ(y + 0.5),
        ),
        cd: mixPoint(
          cNode,
          dNode,
          worldX(x + 0.5),
          worldZ(y + 1),
        ),
        da: mixPoint(
          dNode,
          aNode,
          worldX(x),
          worldZ(y + 0.5),
        ),
      };

      for (const polygon of MARCHING_POLYGONS[mask]) {
        for (let index = 1; index < polygon.length - 1; index += 1) {
          const triangle = [
            points[polygon[0]],
            points[polygon[index]],
            points[polygon[index + 1]],
          ];
          for (const point of triangle) {
            positions.push(point.x, point.y, point.z);
            normals.push(0, 1, 0);
            colors.push(point.color.r, point.color.g, point.color.b);
          }
        }
      }
    }
  }

  geometry.setAttribute(
    'position',
    new THREE.Float32BufferAttribute(positions, 3),
  );
  geometry.setAttribute(
    'normal',
    new THREE.Float32BufferAttribute(normals, 3),
  );
  geometry.setAttribute(
    'color',
    new THREE.Float32BufferAttribute(colors, 3),
  );
  geometry.computeBoundingBox();
  geometry.computeBoundingSphere();
  return geometry;
}

export function createWaterGeometryBuilder(
  window: TerrainWindow,
  chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk,
): (chunk: TerrainChunk) => THREE.BufferGeometry {
  const map = new Map(
    window.chunks.map((entry) => [
      key(entry.x, entry.y),
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
      const neighbor = map.get(key(chunk.x + dx, chunk.y + dy));
      return neighbor
        ? CONNECTED_WATER_KINDS.has(neighbor.waterKind)
        : false;
    })
  );

  return (chunk: TerrainChunk): THREE.BufferGeometry => {
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
  if (isOpenWaterSurfaceKind(chunk.waterKind)) {
    return buildOpenWaterSurfaceGeometry(window, chunkWorldSize);
  }
  return createWaterGeometryBuilder(window, chunkWorldSize)(chunk);
}
