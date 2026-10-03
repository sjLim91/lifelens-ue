import { CORE_SIMULATION_MINUTES_PER_DAY } from '../runtime/generated-core-contract';
/** Presentation budgets and dimensions only; never simulation thresholds. */
export const WORLD_PRESENTATION = {
  conditionBands: 8,
  settlement: { maxVisibleAnchors: 64, focusSegments: 48, focusRadiusGrid: 8,
    hitRadiusGrid: 3, focusColor: 0xc5bf98, focusOpacity: 0.3, groundLift: 0.12,
    maxRelationsInPopup: 6, maxRecentEvents: 3 },
  shelter: { wallProgress: 0.9, sideX: 1.4, wallY: 1.15, wallThickness: 0.08, wallHeight: 2, wallDepth: 1.8 },
  cultivation: { rows: [-0.72, 0, 0.72], length: 3.2, ridgeHeight: 0.08, ridgeWidth: 0.1 },
  weatheredColor: 0x898574,
  wearTint: 0.65,
  damageTilt: 0.12,
  storage: { maxPiles: 4, unitsPerPile: 4, maxInventoryRows: 12, width: 0.32, height: 0.26, depth: 0.4 },
  fire: {
    glowSize: 2.8, opacity: 0.22, textureSize: 32, color: 0xff983a,
    smokeMaxParticles: 192, smokePerFacility: 8, smokeOpacity: 0.22,
    smokeHeight: 3.2, smokeSize: 0.75, smokeDrift: 1.3,
    scorchMaxInstances: 64, scorchRadius: 1.05, scorchOpacity: 0.46,
  },
  weather: {
    wetGroundDarkening: 0.3, dryGroundRoughness: 0.92, wetGroundRoughness: 0.55,
    wetRockDarkening: 0.22, dryRockRoughness: 0.94, wetRockRoughness: 0.52,
    wetWoodDarkening: 0.18, wetStoneDarkening: 0.22,
    wetGrassDarkening: 0.12, wetShrubDarkening: 0.1,
    snowMaxCoverage: 0.72, snowColor: 0xe7edf0, snowRoughness: 0.93,
    snowWarmLimitC: 2, snowColdRangeC: 8,
    puddleMaxInstances: 128, puddleSamplesPerAxis: 3, puddleSegments: 16,
    puddleWetnessThreshold: 0.45, puddleMaxOpacity: 0.52,
    puddleRadius: 0.7, puddleRoughness: 0.28, puddleColor: 0x52615f,
    puddleMaxSlope: 0.12, puddleMinDepression: 0.005, surfaceLift: 0.035,
  },
  residue: {
    maxResidueVisuals: 64, maxClustersPerResidue: 8, unitsPerCluster: 2,
    segments: 24, soilColor: 0x514431, detailColor: 0x292a21,
    humanWasteOpacity: 0.78, detailOpacity: 0.68,
    // Footprint is a restrained fraction of Core exposure radius, not that radius itself.
    footprintRadiusRatio: 0.8,
  },
  naturalResources: {
    patchRadiusWorldUnits: 3.2, slotsPerNode: 3,
    innerRadiusRatio: 0.3, radiusSpreadRatio: 0.45, youngScale: 0.3,
    treeAccessClearanceWorldUnits: 0.9, groundAccessClearanceWorldUnits: 0.28,
    clayHeightRatio: 0.08, clayWidthMultiplier: 2.4,
    colors: { Clay: 0x8d5f48, CopperOre: 0x8e684b, TinOre: 0x858a8c, Flint: 0x4d514f },
  },
  load: { maxPieces: 3, spacing: 0.055 },
  paths: {
    maxMarks: 512, maxSamples: 256,
    widthGrid: 0.22, fadeMinutes: CORE_SIMULATION_MINUTES_PER_DAY,
    opacityPerVisit: 0.07, maxOpacity: 0.38, color: 0x796346,
    wetMudColor: 0x332c22, wetMudOpacityMultiplier: 1.55,
    wetMudWidthMultiplier: 1.18, dryRoughness: 0.96, wetRoughness: 0.64,
  },
} as const;


