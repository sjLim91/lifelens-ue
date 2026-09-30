import * as THREE from 'three';
import type {
  TerrainChunk,
  TerrainWindow,
  WaterKind,
} from '../runtime/core-types';
import {
  WORLD_GRID_CONTRACT,
  WORLD_UNITS_PER_GRID_CELL,
} from '../runtime/lifelens-contract';

const OPEN_WATER_KINDS = new Set<WaterKind>([
  'Ocean',
  'Coast',
]);

const STANDING_FRESH_WATER_KINDS = new Set<WaterKind>([
  'Lake',
  'Wetland',
]);

const FLOW_WATER_KINDS = new Set<WaterKind>([
  'Spring',
  'Stream',
  'River',
]);

type OpenWaterKind = 'Ocean' | 'Coast';
type StandingFreshWaterKind = 'Lake' | 'Wetland';
type FlowWaterKind = 'Spring' | 'Stream' | 'River';

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

interface FlowNode {
  key: string;
  chunk: TerrainChunk;
  x: number;
  y: number;
  z: number;
  width: number;
  color: THREE.Color;
}

interface FlowTarget {
  key: string | null;
  x: number;
  y: number;
  z: number;
  width: number;
  color: THREE.Color;
  score: number;
}

interface FlowEdge {
  source: FlowNode;
  target: FlowTarget;
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

const WATER_COLORS: Record<OpenWaterKind, number> = {
  Ocean: 0x174b67,
  Coast: 0x2f7487,
};

const STANDING_FRESH_WATER_COLORS: Record<StandingFreshWaterKind, number> = {
  Lake: 0x2e7188,
  Wetland: 0x4f8790,
};

const FLOW_COLORS: Record<FlowWaterKind, number> = {
  River: 0x3f8daa,
  Stream: 0x55a0ba,
  Spring: 0x63acc2,
};

const CARDINAL_DIRECTIONS = [
  [1, 0],
  [-1, 0],
  [0, 1],
  [0, -1],
] as const;

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
): kind is OpenWaterKind {
  return OPEN_WATER_KINDS.has(kind);
}

function isFlowWaterKind(kind: WaterKind): kind is FlowWaterKind {
  return FLOW_WATER_KINDS.has(kind);
}

function isStandingFreshWaterKind(
  kind: WaterKind,
): kind is StandingFreshWaterKind {
  return STANDING_FRESH_WATER_KINDS.has(kind);
}

function standingFreshWaterRadiusWorld(chunk: TerrainChunk): number {
  const availability = Math.max(
    0,
    Math.min(1, Number(chunk.waterAvailability) || 0),
  );
  const radiusCells = chunk.waterKind === 'Lake'
    ? 2.50 + 4.25 * availability
    : 3.00 + 4.00 * availability;
  return radiusCells * WORLD_UNITS_PER_GRID_CELL;
}

function flowWidth(chunk: TerrainChunk): number {
  const availability = Math.max(
    0,
    Math.min(1, Number(chunk.waterAvailability) || 0),
  );
  const flow = Math.max(
    0,
    Math.min(1, Number(chunk.flowPotential) || 0),
  );

  switch (chunk.waterKind) {
    case 'River':
      return (
        1.85
        + 2.75 * availability
        + 1.10 * flow
      ) * WORLD_UNITS_PER_GRID_CELL;
    case 'Stream':
      return (
        0.95
        + 1.35 * availability
        + 0.45 * flow
      ) * WORLD_UNITS_PER_GRID_CELL;
    case 'Spring':
      return (
        0.65
        + 0.85 * availability
      ) * WORLD_UNITS_PER_GRID_CELL;
    default:
      return WORLD_UNITS_PER_GRID_CELL;
  }
}

function shouldRenderFlowChunk(chunk: TerrainChunk): boolean {
  if (!isFlowWaterKind(chunk.waterKind)) return false;

  // Match Core's direct-use authority: any fresh surface flow with real
  // availability is a usable water source and therefore must be visible.
  // Presentation must never hide water that Core allows residents to drink.
  return (
    chunk.salinity === 'Fresh'
    && (Number(chunk.waterAvailability) || 0) > 0
  );
}

export function createVisibleWaterFootprintTester(
  window: TerrainWindow,
  chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk,
): (worldX: number, worldZ: number) => boolean {
  const chunks = new Map(
    window.chunks.map((chunk) => [key(chunk.x, chunk.y), chunk]),
  );
  const halfChunk = chunkWorldSize * 0.5;

  const segmentDistance = (
    px: number,
    pz: number,
    ax: number,
    az: number,
    bx: number,
    bz: number,
  ): { distanceSquared: number; t: number } => {
    const abx = bx - ax;
    const abz = bz - az;
    const ab2 = abx * abx + abz * abz;
    const t = ab2 > 0
      ? THREE.MathUtils.clamp(
        ((px - ax) * abx + (pz - az) * abz) / ab2,
        0,
        1,
      )
      : 0;
    const dx = px - (ax + abx * t);
    const dz = pz - (az + abz * t);
    return {
      distanceSquared: dx * dx + dz * dz,
      t,
    };
  };

  return (worldX: number, worldZ: number): boolean => {
    const nearestChunkX =
      Math.round(worldX / chunkWorldSize) + window.centerChunkX;
    const nearestChunkY =
      Math.round(worldZ / chunkWorldSize) + window.centerChunkY;

    for (let dy = -1; dy <= 1; dy += 1) {
      for (let dx = -1; dx <= 1; dx += 1) {
        const chunk = chunks.get(
          key(nearestChunkX + dx, nearestChunkY + dy),
        );
        if (!chunk) continue;

        const centerX =
          (chunk.x - window.centerChunkX) * chunkWorldSize;
        const centerZ =
          (chunk.y - window.centerChunkY) * chunkWorldSize;

        if (isOpenWaterSurfaceKind(chunk.waterKind)) {
          if (
            Math.abs(worldX - centerX) <= halfChunk
            && Math.abs(worldZ - centerZ) <= halfChunk
          ) {
            return true;
          }
          continue;
        }

        if (
          isStandingFreshWaterKind(chunk.waterKind)
          && chunk.salinity === 'Fresh'
          && (Number(chunk.waterAvailability) || 0) > 0
        ) {
          const radius = standingFreshWaterRadiusWorld(chunk);
          const ddx = worldX - centerX;
          const ddz = worldZ - centerZ;
          if (ddx * ddx + ddz * ddz <= radius * radius) {
            return true;
          }
          continue;
        }

        if (
          !isFlowWaterKind(chunk.waterKind)
          || !shouldRenderFlowChunk(chunk)
        ) {
          continue;
        }

        const sourceWidth = flowWidth(chunk);
        if (
          chunk.waterKind === 'Spring'
          || chunk.hasDownstream !== true
          || !Number.isFinite(chunk.downstreamChunkX)
          || !Number.isFinite(chunk.downstreamChunkY)
        ) {
          // Core treats a flow source without a usable downstream segment as
          // a localized circular footprint around the hydrology center.
          const radius = sourceWidth * 0.5;
          const ddx = worldX - centerX;
          const ddz = worldZ - centerZ;
          if (ddx * ddx + ddz * ddz <= radius * radius) {
            return true;
          }
          continue;
        }

        const downstreamX = Number(chunk.downstreamChunkX);
        const downstreamY = Number(chunk.downstreamChunkY);
        const targetX =
          (downstreamX - window.centerChunkX) * chunkWorldSize;
        const targetZ =
          (downstreamY - window.centerChunkY) * chunkWorldSize;
        const targetChunk = chunks.get(key(downstreamX, downstreamY));

        let targetWidth = sourceWidth;
        if (
          targetChunk
          && isFlowWaterKind(targetChunk.waterKind)
          && shouldRenderFlowChunk(targetChunk)
        ) {
          targetWidth = flowWidth(targetChunk);
        } else if (
          targetChunk
          && isOpenWaterSurfaceKind(targetChunk.waterKind)
        ) {
          targetWidth = Math.max(
            sourceWidth,
            chunkWorldSize * 0.09,
          );
        }

        const distance = segmentDistance(
          worldX,
          worldZ,
          centerX,
          centerZ,
          targetX,
          targetZ,
        );
        const halfWidth = THREE.MathUtils.lerp(
          sourceWidth,
          targetWidth,
          distance.t,
        ) * 0.5;
        if (
          distance.distanceSquared
          <= halfWidth * halfWidth
        ) {
          return true;
        }
      }
    }
    return false;
  };
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

      for (const [dx, dy] of CARDINAL_DIRECTIONS) {
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
            points[polygon[index + 1]],
            points[polygon[index]],
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

function buildFlowNodes(
  window: TerrainWindow,
  chunkWorldSize: number,
): Map<string, FlowNode> {
  const nodes = new Map<string, FlowNode>();

  for (const chunk of window.chunks) {
    if (
      !isFlowWaterKind(chunk.waterKind)
      || !shouldRenderFlowChunk(chunk)
    ) {
      continue;
    }
    const nodeKey = key(chunk.x, chunk.y);
    nodes.set(nodeKey, {
      key: nodeKey,
      chunk,
      x: (chunk.x - window.centerChunkX) * chunkWorldSize,
      y: (
        (Number(chunk.elevation01) || 0)
        * WORLD_GRID_CONTRACT.elevationScale
      ) + 0.09,
      z: (chunk.y - window.centerChunkY) * chunkWorldSize,
      width: flowWidth(chunk),
      color: new THREE.Color(FLOW_COLORS[chunk.waterKind]),
    });
  }

  return nodes;
}

function authoritativeFlowTarget(
  node: FlowNode,
  chunks: Map<string, TerrainChunk>,
  nodes: Map<string, FlowNode>,
  openWaterNodes: Map<string, OpenWaterNode>,
  chunkWorldSize: number,
): FlowTarget | null {
  if (
    node.chunk.waterKind === 'Spring'
    || node.chunk.hasDownstream !== true
    || !Number.isFinite(node.chunk.downstreamChunkX)
    || !Number.isFinite(node.chunk.downstreamChunkY)
  ) {
    return null;
  }

  const downstreamX = Number(node.chunk.downstreamChunkX);
  const downstreamY = Number(node.chunk.downstreamChunkY);
  const downstreamKey = key(downstreamX, downstreamY);
  const flowNeighbor = nodes.get(downstreamKey);

  if (flowNeighbor) {
    return {
      key: downstreamKey,
      x: flowNeighbor.x,
      y: flowNeighbor.y,
      z: flowNeighbor.z,
      width: flowNeighbor.width,
      color: flowNeighbor.color,
      score: 1,
    };
  }

  const openWater = openWaterNodes.get(downstreamKey);
  if (openWater) {
    const dx = downstreamX - node.chunk.x;
    const dy = downstreamY - node.chunk.y;
    return {
      key: null,
      x: node.x + dx * chunkWorldSize * 0.58,
      y: openWater.levelWorldY,
      z: node.z + dy * chunkWorldSize * 0.58,
      width: Math.max(node.width, chunkWorldSize * 0.09),
      color: openWater.color,
      score: 1,
    };
  }

  const downstreamChunk = chunks.get(downstreamKey);
  if (!downstreamChunk) {
    return null;
  }

  // The Core can route a channel into a downstream land chunk that does not
  // itself meet the visible Stream/River classification threshold. Do not
  // invent a browser-side continuation across that land. Hiding the segment
  // is safer than exposing a disconnected debug-like stroke.
  return null;
}

function selectFlowEdges(
  window: TerrainWindow,
  chunkWorldSize: number,
): {
  nodes: Map<string, FlowNode>;
  edges: FlowEdge[];
  participatingKeys: Set<string>;
} {
  const chunks = new Map(
    window.chunks.map((chunk) => [key(chunk.x, chunk.y), chunk]),
  );
  const nodes = buildFlowNodes(window, chunkWorldSize);
  const openWaterNodes = buildOpenWaterNodes(window);
  const participatingKeys = new Set<string>();
  const edges: FlowEdge[] = [];

  for (const node of nodes.values()) {
    const target = authoritativeFlowTarget(
      node,
      chunks,
      nodes,
      openWaterNodes,
      chunkWorldSize,
    );
    if (!target) continue;

    participatingKeys.add(node.key);
    if (target.key) participatingKeys.add(target.key);
    edges.push({ source: node, target });
  }

  return { nodes, edges, participatingKeys };
}

function appendDisc(
  positions: number[],
  normals: number[],
  colors: number[],
  indices: number[],
  node: FlowNode,
  radius: number,
): void {
  const segments = 10;
  const base = positions.length / 3;
  positions.push(node.x, node.y + 0.004, node.z);
  normals.push(0, 1, 0);
  colors.push(node.color.r, node.color.g, node.color.b);

  for (let index = 0; index <= segments; index += 1) {
    const angle = (index / segments) * Math.PI * 2;
    positions.push(
      node.x + Math.cos(angle) * radius,
      node.y + 0.004,
      node.z + Math.sin(angle) * radius,
    );
    normals.push(0, 1, 0);
    colors.push(node.color.r, node.color.g, node.color.b);
  }

  for (let index = 0; index < segments; index += 1) {
    indices.push(base, base + index + 2, base + index + 1);
  }
}

function appendAuthoritativeRibbon(
  positions: number[],
  normals: number[],
  colors: number[],
  indices: number[],
  edge: FlowEdge,
  chunkWorldSize: number,
): void {
  const { source, target } = edge;
  const start = new THREE.Vector3(source.x, source.y, source.z);
  const end = new THREE.Vector3(target.x, target.y, target.z);
  const direction = end.clone().sub(start);
  const planarLength = Math.hypot(direction.x, direction.z);

  const tangent = new THREE.Vector3(
    direction.x,
    0,
    direction.z,
  );
  if (tangent.lengthSq() < 0.00001) return;
  tangent.normalize();

  // Core ground traversal and Water resource access use the straight segment
  // from a water chunk center to its authoritative downstream chunk center.
  // Keep the rendered centerline on that exact segment. Presentation-only
  // meanders made residents appear to gather water from dry ground because
  // the visible river drifted away from the Core water footprint.
  const side = new THREE.Vector3(-tangent.z, 0, tangent.x);
  const segments = planarLength > chunkWorldSize * 0.8 ? 8 : 6;
  const base = positions.length / 3;

  for (let index = 0; index <= segments; index += 1) {
    const t = index / segments;
    const point = start.clone().lerp(end, t);
    point.y += 0.004;

    const width = THREE.MathUtils.lerp(
      source.width,
      target.width,
      t,
    );
    const halfWidth = width * 0.5;
    const color = source.color.clone().lerp(target.color, t);

    for (const sign of [-1, 1]) {
      positions.push(
        point.x + side.x * halfWidth * sign,
        point.y,
        point.z + side.z * halfWidth * sign,
      );
      normals.push(0, 1, 0);
      colors.push(color.r, color.g, color.b);
    }
  }

  for (let index = 0; index < segments; index += 1) {
    const left = base + index * 2;
    const right = left + 1;
    const nextLeft = left + 2;
    const nextRight = left + 3;

    indices.push(
      left, right, nextLeft,
      right, nextRight, nextLeft,
    );
  }
}

export function buildFlowWaterSurfaceGeometry(
  window: TerrainWindow,
  chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk,
): THREE.BufferGeometry {
  const {
    nodes,
    edges,
  } = selectFlowEdges(window, chunkWorldSize);

  const positions: number[] = [];
  const normals: number[] = [];
  const colors: number[] = [];
  const indices: number[] = [];

  for (const edge of edges) {
    appendAuthoritativeRibbon(
      positions,
      normals,
      colors,
      indices,
      edge,
      chunkWorldSize,
    );
  }

  for (const node of nodes.values()) {
    // Every Core-usable fresh flow source gets a visible local footprint.
    // Connected sources also receive their ribbon; isolated/weak sources keep
    // this cap so residents can never drink from visually dry ground.
    appendDisc(
      positions,
      normals,
      colors,
      indices,
      node,
      node.width * 0.5,
    );
  }


  for (const chunk of window.chunks) {
    if (
      !isStandingFreshWaterKind(chunk.waterKind)
      || chunk.salinity !== 'Fresh'
      || (Number(chunk.waterAvailability) || 0) <= 0
    ) {
      continue;
    }

    appendDisc(
      positions,
      normals,
      colors,
      indices,
      {
        key: key(chunk.x, chunk.y),
        chunk,
        x: (chunk.x - window.centerChunkX) * chunkWorldSize,
        y: (
          (Number(chunk.elevation01) || 0)
          * WORLD_GRID_CONTRACT.elevationScale
        ) + 0.09,
        z: (chunk.y - window.centerChunkY) * chunkWorldSize,
        width: standingFreshWaterRadiusWorld(chunk) * 2,
        color: new THREE.Color(
          STANDING_FRESH_WATER_COLORS[chunk.waterKind],
        ),
      },
      standingFreshWaterRadiusWorld(chunk),
    );
  }
  const geometry = new THREE.BufferGeometry();
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
  geometry.setIndex(indices);
  geometry.computeBoundingBox();
  geometry.computeBoundingSphere();
  return geometry;
}

export function createWaterGeometryBuilder(
  window: TerrainWindow,
  chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk,
): (chunk: TerrainChunk) => THREE.BufferGeometry {
  const flowGeometry = buildFlowWaterSurfaceGeometry(
    window,
    chunkWorldSize,
  );

  return (chunk: TerrainChunk): THREE.BufferGeometry => {
    if (isOpenWaterSurfaceKind(chunk.waterKind)) {
      return buildOpenWaterSurfaceGeometry(window, chunkWorldSize);
    }
    if (
      isFlowWaterKind(chunk.waterKind)
      || isStandingFreshWaterKind(chunk.waterKind)
    ) {
      return flowGeometry.clone();
    }
    return new THREE.BufferGeometry();
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
  if (
    isFlowWaterKind(chunk.waterKind)
    || isStandingFreshWaterKind(chunk.waterKind)
  ) {
    return buildFlowWaterSurfaceGeometry(window, chunkWorldSize);
  }
  return new THREE.BufferGeometry();
}
