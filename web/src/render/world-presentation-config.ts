import { PRODUCTION_MATERIALS } from './production-material-presentation';
import { CORE_SIMULATION_MINUTES_PER_DAY } from '../runtime/generated-core-contract';
import { SOCIAL_EVENT_PRESENTATION_CONTRACT } from '../runtime/lifelens-contract';
/** Presentation budgets and dimensions only; never simulation thresholds. */
export const WORLD_PRESENTATION = {
  conditionBands: 8,
  residentLife: {
    scaleSettleSeconds: 0.9, elderLeanRadians: 0.055, parentingLeanRadians: 0.1,
    childTouchRadius: 0.22, touchRadiusCap: 0.32,
    headScale: { Baby: 1.3, Toddler: 1.18, Child: 1.08, Teen: 1.02, Adult: 1, Elderly: 1 },
    bodyWidth: { Baby: 0.9, Toddler: 0.9, Child: 0.87, Teen: 0.95, Adult: 1, Elderly: 0.96 },
    gait: { Baby: 0.4, Toddler: 1.15, Child: 1.1, Teen: 1.03, Adult: 1, Elderly: 0.88 },
    belly: { FirstTrimester: 0, SecondTrimester: 0.035, ThirdTrimester: 0.065, Due: 0.075 },
  },
  lifeEvents: {
    maxActive: 8, maxSeen: 128, maxResidentCursors: 384,
    durationSeconds: 3.5, minorDurationSeconds: 2.2,
    pairMaxDistance: 4.5, replayWindowMinutes: SOCIAL_EVENT_PRESENTATION_CONTRACT.replayWindowMinutes, coalesceMinuteWindow: 1,
    segmentsPerCue: 48, ringSegments: 16, radius: 0.32, height: 0.28,
    strokeWidth: 0.025, opacity: 0.65,
    colors: { birth: 0xc6d9bd, growth: 0xc3caa4, pregnancy: 0xcbbba7,
      relationship: 0xc6b8cb, separation: 0xac9d99, loss: 0x9aaba9, household: 0xb8baa0 },
  },
  construction: {
    progressSteps: 40, revealSpan: 0.18, finishingStart: 0.82,
    earlyWorkEnd: 0.25, framingEnd: 0.65,
    freshDurability: 0.75, wornDurability: 0.45, severeDurability: 0.2,
    wearAmounts: [0, 0.3, 0.65, 0.95],
    pileSlots: 4, pileSize: 0.48, pileSpacing: 0.36,
    stakeHeight: 0.48, wearShapeLoss: 0.32, wearTilt: 0.22,
    maxActiveSites: 8, activityPeriodSeconds: 1.6,
    activityHeight: 0.45, activityRadius: 0.2, activityOffset: 1.6,
    workColor: 0xbca789, repairColor: 0xdfcc94, activityOpacity: 0.6,
  },
  settlement: { maxVisibleAnchors: 100, focusSegments: 48, focusRadiusGrid: 10,
    hitRadiusGrid: 8.5, focusColor: 0xc5bf98, focusOpacity: 0.3, groundLift: 0.12,
    maxRelationsInPopup: 6, maxRecentEvents: 3,
    anchorSegments: 24, anchorRadiusGrid: 8, anchorOpacity: 0.5, inactiveStrength: 0.28,
    maxVisibleRoutes: 100, routeSegments: 64, routeColor: 0xb9c5ac, routeOpacity: 0.44,
    routeEvidenceScale: 8, routeBaseStrength: 0.45, routeEvidenceStrength: 0.55, routeDashFraction: 0.55,
    anchorLodDistance: 80, anchorLodMaxScale: 3, anchorLodSteps: 8,
    nearCameraDistance: 10, farCameraDistance: 36, nearOpacityScale: 0.65,
    migrationColor: 0xc4b9a0, migrationOpacity: 0.5, migrationLengthGrid: 12, migrationSegments: 8 },
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
    snowMaxCoverage: 0.5, snowColor: 0xe7edf0, snowRoughness: 0.93,
    snowWarmLimitC: 2, snowColdRangeC: 8, snowVisibleIntensityFloor: 0.2, snowColdCoverageFloor: 0.4,
    puddleMaxInstances: 128, puddleSamplesPerAxis: 3, puddleSegments: 16,
    puddleWetnessThreshold: 0.45, puddleMaxOpacity: 0.52,
    puddleRadius: 0.7, puddleRoughness: 0.28, puddleColor: 0x50584c,
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
    colors: { Clay: 0x8d5f48, CopperOre: 0x8e684b, TinOre: 0x858a8c, IronOre: PRODUCTION_MATERIALS.IronOre.color, Flint: 0x4d514f },
  },
  production: { targetGridTolerance: 1.5, interactionDistance: 0.65,
    gatherReachDistance: 4.1, maxEquipmentSlots: 3, loadScale: 1.15, workSurfaceHeightRatio: .68,
    maxProcessingStocks: 256, processingSlots: 4, processingUnitsPerSlot: 4, processingSteps: 4,
    processingPileSize: 0.3, processingPileSpacing: 0.42 },
  load: { maxPieces: 3, spacing: 0.055 },
  paths: {
    maxMarks: 512, maxSamples: 256,
    widthGrid: 0.22, fadeMinutes: CORE_SIMULATION_MINUTES_PER_DAY,
    opacityPerVisit: 0.07, maxOpacity: 0.38, color: 0x796346,
    wetMudColor: 0x332c22, wetMudOpacityMultiplier: 1.55,
    wetMudWidthMultiplier: 1.18, dryRoughness: 0.96, wetRoughness: 0.64,
  },
} as const;




