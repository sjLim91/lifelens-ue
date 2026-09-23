import {
  CORE_SIMULATION_MINUTES_PER_DAY,
  CORE_WORLD_CHUNK_SPAN_GRID_CELLS,
} from './generated-core-contract';

export const SIMULATION_SPEED_MODES = [
  { speed: 0, label: '⏸', title: '일시정지' },
  { speed: 1, label: '1×', title: '관찰' },
  { speed: 4, label: '4×', title: '빠르게' },
  { speed: 16, label: '16×', title: '더 빠르게' },
  { speed: 64, label: '64×', title: '고속 관찰' },
] as const;

export type SimulationSpeed =
  (typeof SIMULATION_SPEED_MODES)[number]['speed'];

export const SIMULATION_TIME_CONTRACT = {
  simulationMinutesPerDay: CORE_SIMULATION_MINUTES_PER_DAY,
  realMinutesPerSimulationDayAt1x: 8,
  defaultSpeed: SIMULATION_SPEED_MODES[1].speed,
  tickIntervalMs: 125,
  refreshIntervalMs: 500,
  maxCatchupMs: 10_000,
  maxAdvanceMinutesPerTick: 240,
} as const;

export const REAL_MS_PER_SIMULATION_MINUTE_AT_1X = (
  SIMULATION_TIME_CONTRACT.realMinutesPerSimulationDayAt1x
  * 60
  * 1000
) / SIMULATION_TIME_CONTRACT.simulationMinutesPerDay;

export function normalizeSimulationSpeed(speed: number): SimulationSpeed {
  const mode = SIMULATION_SPEED_MODES.find(
    (candidate) => candidate.speed === speed,
  );
  return mode?.speed ?? SIMULATION_TIME_CONTRACT.defaultSpeed;
}

export const WORLD_GRID_CONTRACT = {
  gridCellsPerChunk: CORE_WORLD_CHUNK_SPAN_GRID_CELLS,
  worldUnitsPerChunk: 8,
  elevationScale: 48,
} as const;

export const WORLD_UNITS_PER_GRID_CELL =
  WORLD_GRID_CONTRACT.worldUnitsPerChunk
  / WORLD_GRID_CONTRACT.gridCellsPerChunk;

export const RESIDENT_VISUAL_SPEED_WORLD_UNITS_PER_SECOND_AT_1X =
  WORLD_UNITS_PER_GRID_CELL
  / (REAL_MS_PER_SIMULATION_MINUTE_AT_1X / 1000);

export const OBSERVER_CAMERA_CONTRACT = {
  defaultAngleRadians: -0.68,
  defaultElevationRadians: 0.67,
  defaultDesktopZoom: 1.25,
  defaultMobileZoom: 2.15,
  minZoom: 0.55,
  maxZoom: 2.7,
  minElevationRadians: 0.28,
  maxElevationRadians: 1.18,
  rotateSensitivity: 0.006,
  panWorldUnitsPerPixelAtZoom1: 0.18,
  compactViewportMaxWidthPx: 800,
  tapMoveThresholdPx: 8,
  pointerMotionEpsilonPx: 0.01,
  wheelZoomSensitivity: 0.001,
  maxRenderDevicePixelRatio: 2,
} as const;

export const OBSERVER_RUNTIME_CONTRACT = {
  residentContinuityGraceMs: 10_000,
} as const;

export const RESIDENT_PRESENTATION_CONTRACT = {
  movementEpsilonWorldUnits: 0.01,
  maxAnimationDeltaSeconds: 0.05,
  movementSampleSeconds:
    SIMULATION_TIME_CONTRACT.refreshIntervalMs / 1000,
  targetArrivalPaddingSeconds: 0.04,
  walkStopGraceSeconds:
    (SIMULATION_TIME_CONTRACT.refreshIntervalMs / 1000) * 0.45,
  animationCrossFadeSeconds: 0.14,
  turnResponsivenessPerSecond: 12,
  modelForwardYawOffsetRadians: 0,
} as const;

export function simulationTimeHint(): string {
  return (
    `1× = 현실 ${SIMULATION_TIME_CONTRACT.realMinutesPerSimulationDayAt1x}분에 `
    + 'LifeLens 1일 · 시간 점프 없이 관찰 배속만 조절'
  );
}
