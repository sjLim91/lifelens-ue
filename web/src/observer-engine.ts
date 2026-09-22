import { CharacterLayer, type ResidentPlacement } from './character-layer';
import { LifeLensCoreBridge } from './runtime/core-bridge';
import { runtimeDiagnostics } from './runtime/runtime-diagnostics';
import { SimulationClock } from './runtime/simulation-clock';
import type { Resident, TerrainWindow } from './runtime/core-types';
import { ResidentContinuity } from './runtime/resident-continuity';
import { observerStore } from './state/observer-store';
import { CameraInput } from './input/camera-input';

import { LegacyCanvasWorldRenderer } from './render/legacy-canvas-world-renderer';

const $ = <T extends Element>(selector: string): T => {
  const node = document.querySelector<T>(selector);
  if (!node) throw new Error(`Missing UI element: ${selector}`);
  return node;
};

const canvas = $<HTMLCanvasElement>('#worldCanvas');
const characterCanvas = $<HTMLCanvasElement>('#characterCanvas');
const characterLayer = new CharacterLayer(characterCanvas, () => drawWorld());

const ui = {
  seed: $<HTMLInputElement>('#seedInput'),
  seedError: $<HTMLElement>('#seedError'),
  newWorld: $<HTMLButtonElement>('#newWorld'),
  step10: $<HTMLButtonElement>('#step10'),
  step60: $<HTMLButtonElement>('#step60'),
  left: $<HTMLButtonElement>('#left'),
  right: $<HTMLButtonElement>('#right'),
  up: $<HTMLButtonElement>('#up'),
  down: $<HTMLButtonElement>('#down'),
};

let runtime: LifeLensCoreBridge | null = null;
let centerX = 0;
let centerY = 0;
let terrain: TerrainWindow | null = null;
let stableTerrain: TerrainWindow | null = null;
let stableTerrainCenterKey: string | null = null;
let residentSnapshot: Resident[] = [];
const residentContinuity = new ResidentContinuity(10000);
const legacyRenderer = new LegacyCanvasWorldRenderer(
  canvas,
  characterLayer,
  residentContinuity,
);
let followResidents = true;
let simulationClock: SimulationClock | null = null;
let angle = -0.68;
let zoom = 1;
const cameraInput = new CameraInput(canvas, {
  initialAngle: angle,
  initialZoom: zoom,
  onChange(next) {
    angle = next.angle;
    zoom = next.zoom;
    observerStore.updateCamera({ angle, zoom });
    drawWorld();
  },
});

function controls(enabled: boolean): void {
  [ui.step10, ui.step60, ui.left, ui.right, ui.up, ui.down].forEach((button) => {
    button.disabled = !enabled;
  });
}

function resizeCanvas(): void {
  const rect = canvas.getBoundingClientRect();
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  canvas.width = Math.max(1, Math.round(rect.width * dpr));
  canvas.height = Math.max(1, Math.round(rect.height * dpr));
  characterLayer.resize(rect.width, rect.height, dpr);
  drawWorld();
}

function drawWorld(): void {
  legacyRenderer.draw({
    terrain,
    residents: residentSnapshot,
    centerX,
    centerY,
    zoom,
    angle,
  });
}

function refresh(): void {
  if (!runtime) return;
  const queryStartedAt = performance.now();
  const overview = runtime.worldOverview();
  const residentPayload = runtime.residents();
  const expectedLiving = Math.max(0, Number(overview.livingResidents) || 0);
  residentSnapshot = residentContinuity.stabilize(
    residentPayload,
    expectedLiving,
  );
  const visiblePositions = residentSnapshot
    .map((resident) => residentContinuity.positionFor(resident))
    .filter((position): position is { x: number; y: number } => (
      position !== undefined
    ));
  if (followResidents && visiblePositions.length > 0) {
    const chunkXs = visiblePositions.map((position) => Math.floor(position.x / 32));
    const chunkYs = visiblePositions.map((position) => Math.floor(position.y / 32));
    const minResidentX = Math.min(...chunkXs);
    const maxResidentX = Math.max(...chunkXs);
    const minResidentY = Math.min(...chunkYs);
    const maxResidentY = Math.max(...chunkYs);
    const outsideTrackingEnvelope = chunkXs.some((x) => Math.abs(x - centerX) > 6)
      || chunkYs.some((y) => Math.abs(y - centerY) > 6);
    if (outsideTrackingEnvelope) {
      centerX = Math.round((minResidentX + maxResidentX) * 0.5);
      centerY = Math.round((minResidentY + maxResidentY) * 0.5);
    }
  }
  const residentRadius = visiblePositions.reduce((radius, position) => {
    const chunkX = Math.floor(position.x / 32);
    const chunkY = Math.floor(position.y / 32);
    return Math.max(radius, Math.abs(chunkX - centerX), Math.abs(chunkY - centerY));
  }, 8);
  const queryRadius = Math.max(8, Math.min(16, residentRadius + 2));
  const terrainCandidate = runtime.terrainWindow(centerX, centerY, queryRadius);
  const terrainKey = `${centerX}:${centerY}`;
  const terrainCandidateValid = terrainCandidate.available === true
    && Array.isArray(terrainCandidate.chunks)
    && terrainCandidate.chunks.length > 0;
  if (terrainCandidateValid) {
    terrain = terrainCandidate;
    stableTerrain = terrainCandidate;
    stableTerrainCenterKey = terrainKey;
  } else if (stableTerrain && stableTerrainCenterKey === terrainKey) {
    terrain = stableTerrain;
  } else {
    terrain = {
      ...terrainCandidate,
      available: false,
      chunks: Array.isArray(terrainCandidate.chunks) ? terrainCandidate.chunks : [],
    };
  }

  runtimeDiagnostics.recordCoreQuery(
    performance.now() - queryStartedAt,
    residentSnapshot.length,
    terrain.chunks.length,
  );
  observerStore.update({
    world: overview,
    residents: residentSnapshot,
    terrain,
  });
  observerStore.updateCamera({
    centerChunkX: centerX,
    centerChunkY: centerY,
    angle,
    zoom,
    followResidents,
  });
  drawWorld();
}

function createWorld(): void {
  if (!runtime) return;
  const seed = ui.seed.value.trim();
  if (!seed) {
    ui.seedError.classList.remove('hidden');
    return;
  }
  ui.seedError.classList.add('hidden');
  runtime.createWorld(seed, '', 3);
  centerX = 0;
  centerY = 0;
  followResidents = true;
  residentSnapshot = [];
  terrain = null;
  stableTerrain = null;
  stableTerrainCenterKey = null;
  residentContinuity.reset();
  characterLayer.clearResidents();
  simulationClock?.resetAccumulator();
  observerStore.resetWorld();
  refresh();
}

function move(dx: number, dy: number): void {
  followResidents = false;
  centerX += dx;
  centerY += dy;
  observerStore.updateCamera({
    centerChunkX: centerX,
    centerChunkY: centerY,
    followResidents,
  });
  refresh();
}

function refreshSafely(): void {
  try {
    refresh();
  } catch (error) {
    runtimeDiagnostics.recordCoreFailure();
    console.warn('LifeLens observer refresh failed; simulation clock remains active', error);
  }
}

function startSimulationClock(): void {
  simulationClock?.stop();
  simulationClock = new SimulationClock({
    onAdvance(minutes) {
      runtime?.runMinutes(minutes);
    },
    onRefresh: refreshSafely,
    onError(phase, error) {
      if (phase === 'refresh') runtimeDiagnostics.recordCoreFailure();
      console.warn(`LifeLens simulation ${phase} failed; clock remains active`, error);
    },
  });
  simulationClock.start();
}

ui.newWorld.addEventListener('click', createWorld);
ui.step10.addEventListener('click', () => {
  runtime?.runMinutes(10);
  refresh();
});
ui.step60.addEventListener('click', () => {
  runtime?.runMinutes(60);
  refresh();
});
ui.left.addEventListener('click', () => move(-1, 0));
ui.right.addEventListener('click', () => move(1, 0));
ui.up.addEventListener('click', () => move(0, 1));
ui.down.addEventListener('click', () => move(0, -1));

new ResizeObserver(resizeCanvas).observe(canvas);

async function boot(): Promise<void> {
  controls(false);
  observerStore.setRuntime('loading');

  try {
    runtime = await LifeLensCoreBridge.connect();
    controls(true);
    observerStore.setRuntime('ready');
    createWorld();
    startSimulationClock();
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    console.error('LifeLensCore boot failed', error);
    observerStore.setRuntime('error', message);
    controls(false);
    drawWorld();
  }
}

boot();