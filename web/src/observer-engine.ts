import { CharacterLayer } from './character-layer';
import { LifeLensCoreBridge } from './runtime/core-bridge';
import { runtimeDiagnostics } from './runtime/runtime-diagnostics';
import { SimulationClock } from './runtime/simulation-clock';
import type { Resident, TerrainWindow } from './runtime/core-types';
import { ResidentContinuity } from './runtime/resident-continuity';
import { WorldSession } from './runtime/world-session';
import { observerActions } from './state/observer-actions';
import { observerStore } from './state/observer-store';
import { CameraInput } from './input/camera-input';

import { LegacyCanvasWorldRenderer } from './render/legacy-canvas-world-renderer';
import { readRenderMode } from './render/render-mode';
import { WorldRenderer } from './render/world-renderer';

function requireCanvas(selector: string): HTMLCanvasElement {
  const element = document.querySelector<HTMLCanvasElement>(selector);
  if (!element) throw new Error(`Missing canvas: ${selector}`);
  return element;
}

export function startObserverEngine(): void {
  const canvas = requireCanvas('#worldCanvas');
  const threeWorldCanvas = requireCanvas('#threeWorldCanvas');
  const characterCanvas = requireCanvas('#characterCanvas');
  const requestedRenderMode = readRenderMode();

  let effectiveRenderMode = requestedRenderMode;
  let threeWorldRenderer: WorldRenderer | null = null;
  let characterLayer: CharacterLayer | null = null;

  if (requestedRenderMode === 'three-world') {
    try {
      threeWorldRenderer = new WorldRenderer(threeWorldCanvas);
      threeWorldRenderer.start();
    } catch (error) {
      effectiveRenderMode = 'legacy-canvas';
      console.warn(
        'LifeLens Three World unavailable; falling back to Legacy renderer',
        error,
      );
    }
  }

  if (effectiveRenderMode === 'legacy-canvas') {
    characterLayer = new CharacterLayer(characterCanvas, () => drawWorld());
  }

  canvas.style.opacity = effectiveRenderMode === 'legacy-canvas' ? '1' : '0';
  threeWorldCanvas.style.opacity = effectiveRenderMode === 'three-world' ? '1' : '0';
  characterCanvas.style.opacity = effectiveRenderMode === 'legacy-canvas' ? '1' : '0';
  
  let worldSession: WorldSession | null = null;
  let centerX = 0;
  let centerY = 0;
  let terrain: TerrainWindow | null = null;
  let residentSnapshot: Resident[] = [];
  const residentContinuity = new ResidentContinuity(10000);
  const legacyRenderer = characterLayer
    ? new LegacyCanvasWorldRenderer(
      canvas,
      characterLayer,
      residentContinuity,
    )
    : null;
  let followResidents = true;
  let simulationClock: SimulationClock | null = null;
  let angle = -0.68;
  const compactViewport = window.matchMedia('(max-width: 800px)').matches;
  let zoom = compactViewport ? 2.15 : 1.25;
  new CameraInput(canvas, {
    initialAngle: angle,
    initialZoom: zoom,
    onChange(next) {
      angle = next.angle;
      zoom = next.zoom;
      observerStore.updateCamera({ angle, zoom });
      threeWorldRenderer?.setCamera({
        centerChunkX: centerX,
        centerChunkY: centerY,
        angle,
        zoom,
      });
      drawWorld();
    },
  });
  
  function resizeCanvas(): void {
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    canvas.width = Math.max(1, Math.round(rect.width * dpr));
    canvas.height = Math.max(1, Math.round(rect.height * dpr));
    characterLayer?.resize(rect.width, rect.height, dpr);
    threeWorldRenderer?.resize(rect.width, rect.height);
    drawWorld();
  }
  
  function drawWorld(): void {
    legacyRenderer?.draw({
      terrain,
      residents: residentSnapshot,
      centerX,
      centerY,
      zoom,
      angle,
    });
  }
  
  function refresh(): void {
    if (!worldSession) return;
  
    const queryStartedAt = performance.now();
    const snapshot = worldSession.refresh();
    centerX = snapshot.centerX;
    centerY = snapshot.centerY;
    followResidents = snapshot.followResidents;
    residentSnapshot = snapshot.residents;
    terrain = snapshot.terrain;
  
    runtimeDiagnostics.recordCoreQuery(
      performance.now() - queryStartedAt,
      residentSnapshot.length,
      terrain.chunks.length,
    );
    observerStore.update({
      world: snapshot.overview,
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
    threeWorldRenderer?.setTerrain(terrain);
    threeWorldRenderer?.setResidents(
      residentSnapshot,
      terrain,
      centerX,
      centerY,
    );
    threeWorldRenderer?.setSimulationMinute(
      Number(snapshot.overview.minute) || 0,
    );
    threeWorldRenderer?.setCamera({
      centerChunkX: centerX,
      centerChunkY: centerY,
      angle,
      zoom,
    });
    drawWorld();
  }
  
  function createWorld(seed: string): void {
    if (!worldSession) return;
    worldSession.createWorld(seed);
    centerX = 0;
    centerY = 0;
    followResidents = true;
    residentSnapshot = [];
    terrain = null;
    characterLayer?.clearResidents();
    simulationClock?.resetAccumulator();
    observerStore.resetWorld();
    refresh();
  }
  
  function move(dx: number, dy: number): void {
    worldSession?.moveObserver(dx, dy);
    refresh();
  }
  
  function stepMinutes(minutes: number): void {
    worldSession?.runMinutes(minutes);
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
        worldSession?.runMinutes(minutes);
      },
      onRefresh: refreshSafely,
      onError(phase, error) {
        if (phase === 'refresh') runtimeDiagnostics.recordCoreFailure();
        console.warn(`LifeLens simulation ${phase} failed; clock remains active`, error);
      },
    });
    simulationClock.start();
  }
  
  observerActions.bind({
    createWorld,
    stepMinutes,
    moveObserver: move,
  });
  
  new ResizeObserver(resizeCanvas).observe(canvas);
  
  async function boot(): Promise<void> {
    observerStore.setRuntime('loading');
  
    try {
      const core = await LifeLensCoreBridge.connect();
      worldSession = new WorldSession(core, residentContinuity);
      observerStore.setRuntime('ready');
      createWorld('42');
      startSimulationClock();
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      console.error('LifeLensCore boot failed', error);
      observerStore.setRuntime('error', message);
      drawWorld();
    }
  }
  
  void boot();
  }
