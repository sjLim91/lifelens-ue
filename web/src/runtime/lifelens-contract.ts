export const SIMULATION_TIME_CONTRACT = {
  simulationMinutesPerDay: 24 * 60,
  realMinutesPerSimulationDayAt1x: 8,
  speeds: [0, 1, 4, 16, 64] as const,
  defaultSpeed: 1,
  tickIntervalMs: 125,
  refreshIntervalMs: 500,
  maxCatchupMs: 10_000,
  maxAdvanceMinutesPerTick: 240,
} as const;

export type SimulationSpeed =
  (typeof SIMULATION_TIME_CONTRACT.speeds)[number];

export const REAL_MS_PER_SIMULATION_MINUTE_AT_1X = (
  SIMULATION_TIME_CONTRACT.realMinutesPerSimulationDayAt1x
  * 60
  * 1000
) / SIMULATION_TIME_CONTRACT.simulationMinutesPerDay;

export function normalizeSimulationSpeed(speed: number): SimulationSpeed {
  return SIMULATION_TIME_CONTRACT.speeds.includes(
    speed as SimulationSpeed,
  )
    ? speed as SimulationSpeed
    : SIMULATION_TIME_CONTRACT.defaultSpeed;
}

export const SIMULATION_SPEED_PRESETS = [
  { speed: 0, label: '⏸', title: '일시정지' },
  { speed: 1, label: '1×', title: '관찰' },
  { speed: 4, label: '4×', title: '빠르게' },
  { speed: 16, label: '16×', title: '더 빠르게' },
  { speed: 64, label: '64×', title: '고속 관찰' },
] as const satisfies ReadonlyArray<{
  speed: SimulationSpeed;
  label: string;
  title: string;
}>;

export const WORLD_GRID_CONTRACT = {
  gridCellsPerChunk: 32,
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
} as const;

export function simulationTimeHint(): string {
  return (
    `1× = 현실 ${SIMULATION_TIME_CONTRACT.realMinutesPerSimulationDayAt1x}분에 `
    + 'LifeLens 1일 · 시간 점프 없이 관찰 배속만 조절'
  );
}
