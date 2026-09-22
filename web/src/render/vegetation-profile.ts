import {
  OBSERVER_CAMERA_CONTRACT,
  WORLD_GRID_CONTRACT,
} from '../runtime/lifelens-contract';

const TREE_HEIGHT_WORLD_UNITS =
  WORLD_GRID_CONTRACT.worldUnitsPerChunk * 0.92;

export const VEGETATION_PRESENTATION_CONTRACT = {
  maxTrees: 3072,
  minForestCoverage01: 0.12,
  desktopMaxTreesPerChunk: 9,
  mobileMaxTreesPerChunk: 6,
  placementSpanChunkRatio: 0.8,
  branchCountPerTree: 4,
  crownLobeCountPerTree: 5,
  treeHeightWorldUnits: TREE_HEIGHT_WORLD_UNITS,
  trunkHeightWorldUnits: TREE_HEIGHT_WORLD_UNITS * 0.46,
  trunkRadiusWorldUnits: TREE_HEIGHT_WORLD_UNITS * 0.035,
  branchLengthWorldUnits: TREE_HEIGHT_WORLD_UNITS * 0.2,
  branchRadiusWorldUnits: TREE_HEIGHT_WORLD_UNITS * 0.012,
  crownRadiusWorldUnits: TREE_HEIGHT_WORLD_UNITS * 0.21,
} as const;

export const TREE_CROWN_LOBE_LAYOUT = [
  { x: 0, y: 0.72, z: 0, radius: 1.06 },
  { x: 0.54, y: 0.61, z: 0.1, radius: 0.82 },
  { x: -0.48, y: 0.58, z: 0.18, radius: 0.86 },
  { x: 0.08, y: 0.6, z: 0.5, radius: 0.8 },
  { x: -0.1, y: 0.64, z: -0.48, radius: 0.78 },
] as const;

export const TREE_FOLIAGE_PALETTE = [
  0x2f6638,
  0x396f3d,
  0x285b34,
  0x487843,
] as const;

export const TREE_BARK_PALETTE = [
  0x4f3c2a,
  0x5b4530,
  0x443428,
] as const;

export function useMobileVegetationProfile(): boolean {
  if (typeof window === 'undefined') return false;

  const compactViewport =
    Math.min(window.innerWidth, window.innerHeight)
    <= OBSERVER_CAMERA_CONTRACT.compactViewportMaxWidthPx;
  const coarsePointer = window.matchMedia?.('(pointer: coarse)').matches ?? false;

  return compactViewport || coarsePointer;
}
