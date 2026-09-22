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
import { CameraInput } from './input/camera-input';

import { LegacyCanvasWorldRenderer } from './render/legacy-canvas-world-renderer';
import { readRenderMode } from './render/render-mode';
import { WorldRenderer } from './render/world-renderer';
import { createTerrainElevationSampler } from './render/terrain-geometry';

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
  let localPanY = 0;
  let localPanZ = 0;
  let autoFrameActivity = true;
  let autoFrameZoom = true;
  new CameraInput(canvas, {
    initialAngle: angle,
    initialElevation: elevation,
    initialZoom: zoom,
    onChange(next) {
      if (Math.abs(next.zoom - zoom) > 0.0001) {
        autoFrameZoom = false;
      }
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
        panY: localPanY,
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
      autoFrameActivity = false;

      const worldUnitsPerPixel =
        OBSERVER_CAMERA_CONTRACT.panWorldUnitsPerPixelAtZoom1
        / Math.max(OBSERVER_CAMERA_CONTRACT.minZoom, zoom);
      const sin = Math.sin(angle);
      const cos = Math.cos(angle);

      localPanX += (
        (sin * deltaX) - (cos * deltaY)
      ) * worldUnitsPerPixel;
      localPanZ += (
        (-cos * deltaX) - (sin * deltaY)
      ) * worldUnitsPerPixel;

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
        panY: localPanY,
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

  function gridToLocalWorld(gridX: number, gridY: number): {
    x: number;
    y: number;
    z: number;
  } {
    const cells = WORLD_GRID_CONTRACT.gridCellsPerChunk;
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const chunkX = Math.floor(gridX / cells);
    const chunkY = Math.floor(gridY / cells);
    const localX = (gridX - chunkX * cells) / cells;
    const localY = (gridY - chunkY * cells) / cells;
    const elevation = terrain
      ? createTerrainElevationSampler(terrain)(
          chunkX,
          chunkY,
          localX,
          localY,
        ) * WORLD_GRID_CONTRACT.elevationScale
      : 0;
    return {
      x: (chunkX - centerX + localX - 0.5) * chunkWorldSize,
      y: elevation,
      z: (chunkY - centerY + localY - 0.5) * chunkWorldSize,
    };
  }

  function updateActivityFrame(
    residents: Resident[],
    facilities: Array<{ gridX: number; gridY: number; state?: string }>,
  ): void {
    if (!autoFrameActivity) return;

    const selectedId = observerStore.getSnapshot().selectedResidentId;
    const selected = selectedId
      ? residents.find((resident) => resident.id === selectedId)
      : undefined;

    if (
      selected?.hasPosition
      && typeof selected.gridX === 'number'
      && typeof selected.gridY === 'number'
    ) {
      const point = gridToLocalWorld(selected.gridX, selected.gridY);
      localPanX = point.x;
      localPanY = point.y;
      localPanZ = point.z;
      if (autoFrameZoom) {
        zoom = compactViewport ? 2.25 : 1.9;
      }
      return;
    }

    const points: Array<{ x: number; y: number; z: number; weight: number }> = [];
    for (const resident of residents) {
      if (
        !resident.hasPosition
        || typeof resident.gridX !== 'number'
        || typeof resident.gridY !== 'number'
      ) {
        continue;
      }
      const point = gridToLocalWorld(resident.gridX, resident.gridY);
      points.push({ ...point, weight: 2 });
    }
    for (const facility of facilities) {
      if (
        facility.state === 'Ruined'
        || typeof facility.gridX !== 'number'
        || typeof facility.gridY !== 'number'
      ) {
        continue;
      }
      const point = gridToLocalWorld(facility.gridX, facility.gridY);
      points.push({ ...point, weight: 1 });
    }

    if (points.length === 0) return;
    let weightedX = 0;
    let weightedY = 0;
    let weightedZ = 0;
    let weightTotal = 0;
    for (const point of points) {
      weightedX += point.x * point.weight;
      weightedY += point.y * point.weight;
      weightedZ += point.z * point.weight;
      weightTotal += point.weight;
    }
    localPanX = weightedX / Math.max(1, weightTotal);
    localPanY = weightedY / Math.max(1, weightTotal);
    localPanZ = weightedZ / Math.max(1, weightTotal);

    if (autoFrameZoom) {
      let maxRadius = 0;
      for (const point of points) {
        maxRadius = Math.max(
          maxRadius,
          Math.hypot(
            point.x - localPanX,
            point.z - localPanZ,
          ),
        );
      }
      const targetZoom = 86 / Math.max(30, maxRadius + 26);
      zoom = Math.max(
        OBSERVER_CAMERA_CONTRACT.minZoom,
        Math.min(
          compactViewport ? 2.25 : 2.15,
          targetZoom,
        ),
      );
    }
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
      presentation: snapshot.presentation,
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
    updateActivityFrame(
      residentSnapshot,
      snapshot.presentation?.facilities ?? [],
    );
    threeWorldRenderer?.setSelectedResident(selectedResidentId);
    threeWorldRenderer?.setTerrain(terrain);
    threeWorldRenderer?.setResidents(
      residentSnapshot,
      terrain,
      centerX,
      centerY,
    );
    threeWorldRenderer?.setWorldPresentation(
      snapshot.presentation,
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
    autoFrameActivity = true;
    autoFrameZoom = true;
    localPanX = 0;
    localPanY = 0;
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
    autoFrameActivity = residentId !== null;
    if (residentId !== null) autoFrameZoom = true;
    if (residentId !== null) {
      const selected = residentSnapshot.find(
        (resident) => resident.id === residentId,
      );
      if (
        selected?.hasPosition
        && typeof selected.gridX === 'number'
        && typeof selected.gridY === 'number'
      ) {
        const point = gridToLocalWorld(selected.gridX, selected.gridY);
        localPanX = point.x;
        localPanY = point.y;
        localPanZ = point.z;
      }
    }
    threeWorldRenderer?.setSelectedResident(
      observerStore.getSnapshot().selectedResidentId,
    );
    threeWorldRenderer?.setCamera({
      centerChunkX: centerX,
      centerChunkY: centerY,
      angle,
      elevation,
      zoom,
      panX: localPanX,
      panZ: localPanZ,
    });
  }
  
  function move(dx: number, dy: number): void {
    localPanX = 0;
    localPanY = 0;
    localPanZ = 0;
    worldSession?.moveObserver(dx, dy);
    refresh();
  }

  function recenterObserver(): void {
    if (!worldSession) return;
    autoFrameActivity = true;
    autoFrameZoom = true;
    localPanX = 0;
    localPanY = 0;
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
