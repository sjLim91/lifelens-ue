import { CharacterLayer } from './character-layer';
import { LifeLensCoreBridge } from './runtime/core-bridge';
import { runtimeDiagnostics } from './runtime/runtime-diagnostics';
import { SimulationClock } from './runtime/simulation-clock';
import {
  OBSERVER_CAMERA_CONTRACT,
  OBSERVER_RUNTIME_CONTRACT,
  WORLD_GRID_CONTRACT,
  normalizeSimulationSpeed,
} from './runtime/lifelens-contract';
import type { Resident, TerrainWindow } from './runtime/core-types';
import { ResidentContinuity } from './runtime/resident-continuity';
import { WorldSession } from './runtime/world-session';
import { observerActions } from './state/observer-actions';
import { observerStore } from './state/observer-store';
import { CameraInput, cameraPanDelta } from './input/camera-input';

import { LegacyCanvasWorldRenderer } from './render/legacy-canvas-world-renderer';
import { readRenderMode } from './render/render-mode';
import { WorldRenderer } from './render/world-renderer';

function generateWorldSeed(): string {
  const words = new Uint32Array(2);
  globalThis.crypto.getRandomValues(words);
  const value = (BigInt(words[0]) << 32n) | BigInt(words[1]);
  return (value === 0n ? 1n : value).toString();
}

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
  const residentContinuity = new ResidentContinuity(
    OBSERVER_RUNTIME_CONTRACT.residentContinuityGraceMs,
  );
  const legacyRenderer = characterLayer
    ? new LegacyCanvasWorldRenderer(
      canvas,
      characterLayer,
      residentContinuity,
    )
    : null;
  let followResidents = false;
  let simulationClock: SimulationClock | null = null;
  let angle: number = OBSERVER_CAMERA_CONTRACT.defaultAngleRadians;
  let elevation: number = OBSERVER_CAMERA_CONTRACT.defaultElevationRadians;
  const compactViewport = window.matchMedia(
    `(max-width: ${OBSERVER_CAMERA_CONTRACT.compactViewportMaxWidthPx}px)`,
  ).matches;
  let zoom: number = compactViewport
    ? OBSERVER_CAMERA_CONTRACT.defaultMobileZoom
    : OBSERVER_CAMERA_CONTRACT.defaultDesktopZoom;
  let localPanX = 0;
  let localPanZ = 0;
  new CameraInput(canvas, {
    initialAngle: angle,
    initialElevation: elevation,
    initialZoom: zoom,
    onChange(next) {
      angle = next.angle;
      elevation = next.elevation;
      zoom = next.zoom;
      observerStore.updateCamera({ angle, elevation, zoom });
      threeWorldRenderer?.setCamera({
        centerChunkX: centerX,
        centerChunkY: centerY,
        angle,
        elevation,
        zoom,
        panX: localPanX,
        panZ: localPanZ,
      });
      drawWorld();
    },
    onPan(deltaX, deltaY) {
      if (!worldSession) return;

      if (followResidents) {
        worldSession.moveObserver(0, 0);
        followResidents = false;
      }

      const worldUnitsPerPixel =
        OBSERVER_CAMERA_CONTRACT.panWorldUnitsPerPixelAtZoom1
        / Math.max(OBSERVER_CAMERA_CONTRACT.minZoom, zoom);
      const pan = cameraPanDelta(angle, deltaX, deltaY);
      localPanX += pan.x * worldUnitsPerPixel;
      localPanZ += pan.z * worldUnitsPerPixel;

      const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
      const stepX = Math.trunc(localPanX / chunkWorldSize);
      const stepY = Math.trunc(localPanZ / chunkWorldSize);

      if (stepX !== 0 || stepY !== 0) {
        localPanX -= stepX * chunkWorldSize;
        localPanZ -= stepY * chunkWorldSize;
        worldSession.moveObserver(stepX, stepY);
        refresh();
        return;
      }

      observerStore.updateCamera({
        centerChunkX: centerX,
        centerChunkY: centerY,
        angle,
        elevation,
        zoom,
        followResidents: false,
      });
      threeWorldRenderer?.setCamera({
        centerChunkX: centerX,
        centerChunkY: centerY,
        angle,
        elevation,
        zoom,
        panX: localPanX,
        panZ: localPanZ,
      });
      drawWorld();
    },
    onTap(clientX, clientY) {
      if (!threeWorldRenderer) return;
      const residentId = threeWorldRenderer.pickResident(clientX, clientY);
      selectResident(residentId);
    },
  });
  
  function resizeCanvas(): void {
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(
      window.devicePixelRatio || 1,
      OBSERVER_CAMERA_CONTRACT.maxRenderDevicePixelRatio,
    );
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
      environment: snapshot.environment,
    });
    observerStore.updateCamera({
      centerChunkX: centerX,
      centerChunkY: centerY,
      angle,
      elevation,
      zoom,
      followResidents,
    });
    const selectedResidentId = observerStore.getSnapshot().selectedResidentId;
    threeWorldRenderer?.setSelectedResident(selectedResidentId);
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
    threeWorldRenderer?.setEnvironment(snapshot.environment);
    threeWorldRenderer?.setCamera({
      centerChunkX: centerX,
      centerChunkY: centerY,
      angle,
      elevation,
      zoom,
      panX: localPanX,
      panZ: localPanZ,
    });
    drawWorld();
  }
  
  function createWorld(seed: string): void {
    if (!worldSession) return;
    const requestedSeed = seed.trim();
    const effectiveSeed = requestedSeed || generateWorldSeed();
    worldSession.createWorld(effectiveSeed);
    centerX = 0;
    centerY = 0;
    followResidents = false;
    localPanX = 0;
    localPanZ = 0;
    residentSnapshot = [];
    terrain = null;
    characterLayer?.clearResidents();
    simulationClock?.resetAccumulator();
    observerStore.resetWorld();
    threeWorldRenderer?.setSelectedResident(null);
    refresh();
  }

  function selectResident(residentId: string | null): void {
    observerStore.selectResident(residentId);
    threeWorldRenderer?.setSelectedResident(
      observerStore.getSnapshot().selectedResidentId,
    );
  }
  
  function move(dx: number, dy: number): void {
    localPanX = 0;
    localPanZ = 0;
    worldSession?.moveObserver(dx, dy);
    refresh();
  }

  function recenterObserver(): void {
    if (!worldSession) return;
    localPanX = 0;
    localPanZ = 0;
    worldSession.recenterToResidents();
    refresh();
  }
  
  function setSimulationSpeed(speed: number): void {
    const canonicalSpeed = normalizeSimulationSpeed(speed);
    simulationClock?.setSpeed(canonicalSpeed);
    threeWorldRenderer?.setSimulationSpeed(canonicalSpeed);
    observerStore.update({ simulationSpeed: canonicalSpeed });
    refreshSafely();
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
    const initialSpeed = observerStore.getSnapshot().simulationSpeed;
    threeWorldRenderer?.setSimulationSpeed(initialSpeed);
    simulationClock = new SimulationClock({
      initialSpeed,
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
    setSimulationSpeed,
    moveObserver: move,
    recenterObserver,
    selectResident,
  });
  
  new ResizeObserver(resizeCanvas).observe(canvas);
  
  async function boot(): Promise<void> {
    observerStore.setRuntime('loading');
  
    try {
      const core = await LifeLensCoreBridge.connect();
      worldSession = new WorldSession(core, residentContinuity);
      observerStore.setRuntime('ready');
      createWorld('');
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
