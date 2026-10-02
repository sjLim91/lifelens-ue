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
  storage: { maxPiles: 4, unitsPerPile: 4, width: 0.32, height: 0.26, depth: 0.4 },
  fire: { glowSize: 2.8, opacity: 0.22, textureSize: 32, color: 0xff983a },
  resource: { fullScale: 1.04 },
  load: { maxPieces: 3, spacing: 0.055 },
  paths: {
    maxMarks: 512, maxSamples: 256,
    widthGrid: 0.22, fadeMinutes: CORE_SIMULATION_MINUTES_PER_DAY,
    opacityPerVisit: 0.07, maxOpacity: 0.38, color: 0x796346,
  },
} as const;
