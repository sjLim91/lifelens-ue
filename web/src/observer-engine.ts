import { shadowObserver } from './cognition/shadow-observer';
import { residentAI } from './cognition/live-resident-ai';
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
import { WorldSession, type WorldSessionSnapshot } from './runtime/world-session';
import { observerActions } from './state/observer-actions';
import type { ObservationEvent } from './state/observation-feed';
import { observerStore } from './state/observer-store';
import {
  buildFastForwardSummary,
  captureFastForwardState,
  FAST_FORWARD_CHUNK_MINUTES,
  MAX_FAST_FORWARD_DAYS,
  MINUTES_PER_DAY,
} from './state/fast-forward';
import { humanTraceFocus, visibleHumanTraces } from './state/human-traces';
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

export function startObserverEngine(): () => void {
  let stopped = false;
  let ownedCore: LifeLensCoreBridge | null = null;
  const canvas = requireCanvas('#worldCanvas');
  const threeWorldCanvas = requireCanvas('#threeWorldCanvas');
  const characterCanvas = requireCanvas('#characterCanvas');
  const requestedRenderMode = readRenderMode();

  let effectiveRenderMode = requestedRenderMode;
  let threeWorldRenderer: WorldRenderer | null = null;
  let characterLayer: CharacterLayer | null = null;
  let legacyRenderer: LegacyCanvasWorldRenderer | null = null;
  let renderFailureBanner: HTMLElement | null = null;
  let startupRenderFailure: string | null = null;

  // A mobile WebGL context can be lost after a successful boot while the
  // WASM simulation continues. Preserve the active WorldSession and render
  // the SAME authoritative Core observations on a pure 2D canvas instead.
  function enterCanvasFailover(reason: string, error?: unknown): void {
    if (stopped || effectiveRenderMode === 'legacy-canvas') return;
    effectiveRenderMode = 'legacy-canvas';
    console.warn('LifeLens 3D rendering unavailable; using 2D Core observer', reason, error);
    const failedRenderer = threeWorldRenderer;
    threeWorldRenderer = null;
    try { failedRenderer?.dispose(); } catch (disposeError) {
      console.warn('Failed to dispose unavailable WebGL renderer', disposeError);
    }
    try {
      legacyRenderer = new LegacyCanvasWorldRenderer(
        canvas, { ready: false, setResidents: () => {} }, residentContinuity,
      );
      canvas.style.opacity = '1';
      threeWorldCanvas.style.opacity = '0';
      threeWorldCanvas.style.visibility = 'hidden';
      characterCanvas.style.opacity = '0';
      if (!renderFailureBanner) {
        renderFailureBanner = document.createElement('div');
        renderFailureBanner.className = 'render-fallback-banner';
        renderFailureBanner.setAttribute('role', 'status');
        canvas.parentElement?.appendChild(renderFailureBanner);
      }
      renderFailureBanner.textContent =
        '3D 그래픽 오류 · 2D 비상 관찰 모드로 전환했습니다. 주민과 시뮬레이션은 계속 진행됩니다.';
      resizeCanvas();
    } catch (fallbackError) {
      console.error('LifeLens 2D failover unavailable', fallbackError);
    }
  }

  if (requestedRenderMode === 'three-world') {
    try {
      threeWorldRenderer = new WorldRenderer(
        threeWorldCanvas, enterCanvasFailover,
      );
      threeWorldRenderer.start();
    } catch (error) {
      effectiveRenderMode = 'legacy-canvas';
      startupRenderFailure = error instanceof Error ? error.message : String(error);
      console.warn('LifeLens Three World unavailable; using 2D fallback', error);
    }
  }

  if (effectiveRenderMode === 'legacy-canvas') {
    try {
      characterLayer = new CharacterLayer(characterCanvas, () => drawWorld());
    } catch (error) {
      // A broken GPU cannot be the prerequisite for a Canvas 2D rescue.
      startupRenderFailure ??= error instanceof Error ? error.message : String(error);
      console.warn('Legacy WebGL avatars unavailable; using 2D resident markers', error);
    }
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
  if (effectiveRenderMode === 'legacy-canvas') {
    legacyRenderer = new LegacyCanvasWorldRenderer(
      canvas, characterLayer ?? { ready: false, setResidents: () => {} },
      residentContinuity,
    );
    if (startupRenderFailure) {
      renderFailureBanner = document.createElement('div');
      renderFailureBanner.className = 'render-fallback-banner';
      renderFailureBanner.setAttribute('role', 'status');
      renderFailureBanner.textContent =
        '3D 그래픽 장치를 사용할 수 없어 2D 관찰 모드로 실행합니다.';
      canvas.parentElement?.appendChild(renderFailureBanner);
    }
  }
  let followResidents = false;
  let simulationClock: SimulationClock | null = null;
  let fastForwardRunning = false;
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
  const cameraInput = new CameraInput(canvas, {
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
      if (residentId) selectResident(residentId);
      else {
        const traceId = threeWorldRenderer.pickHumanTrace(clientX, clientY);
        if (traceId) selectHumanTrace(traceId);
        else selectSettlement(threeWorldRenderer.pickSettlement(clientX, clientY));
      }
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
    try {
      threeWorldRenderer?.resize(rect.width, rect.height);
    } catch (error) {
      enterCanvasFailover('3D 화면 크기 조정에 실패했습니다.', error);
    }
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
  
  function refresh(requireFreshActivity = false): WorldSessionSnapshot | null {
    if (!worldSession) return null;
  
    const queryStartedAt = performance.now();
    const snapshot = worldSession.refresh(requireFreshActivity);
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
      earthScenario: snapshot.earthScenario,
      earthSolar: snapshot.earthSolar,
      residents: residentSnapshot,
      terrain,
      environment: snapshot.environment,
      socialEvents: snapshot.socialEvents,
      civilization: snapshot.civilization,
      worldObjects: snapshot.worldObjects,
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
    shadowObserver.observe(snapshot.overview, residentSnapshot, selectedResidentId);
    residentAI.observe(snapshot.overview, residentSnapshot);
    try {
    threeWorldRenderer?.setSelectedResident(selectedResidentId);
    if (snapshot.terrainStaticChanged) {
      threeWorldRenderer?.setTerrain(terrain);
    } else {
      threeWorldRenderer?.setHumanTraces(terrain);
    }
    threeWorldRenderer?.setAuthoritativeSpatialTargets(
      snapshot.civilization,
      snapshot.worldObjects,
      terrain,
    );
    threeWorldRenderer?.setSelectedHumanTrace(observerStore.getSnapshot().selectedHumanTraceId);
    threeWorldRenderer?.setSelectedSettlement(observerStore.getSnapshot().selectedSettlementId);
    threeWorldRenderer?.setResidents(
      residentSnapshot,
      terrain,
      centerX,
      centerY,
    );
    threeWorldRenderer?.setSimulationMinute(
      Number(snapshot.overview.minute) || 0,
    );
    threeWorldRenderer?.setSocialEvents(
      snapshot.socialEvents,
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
    } catch (error) {
      enterCanvasFailover('3D 세계 갱신에 실패했습니다.', error);
    }
    drawWorld();
    return snapshot;
  }
  
  function createWorld(seed: string): void {
    if (!worldSession || fastForwardRunning) return;
    const requestedSeed = seed.trim();
    const effectiveSeed = requestedSeed || generateWorldSeed();
    worldSession.createWorld(effectiveSeed);
    resetAfterWorldCreated();
  }

  function createEarthWorld(latitude: number, longitude: number): boolean {
    if (!worldSession || fastForwardRunning
      || !Number.isFinite(latitude) || !Number.isFinite(longitude))
      return false;
    const seed = generateWorldSeed();
    if (!worldSession.createEarthWorld(seed, latitude, longitude))
      return false;
    resetAfterWorldCreated();
    return true;
  }

  function resetAfterWorldCreated(): void {
    shadowObserver.resetWorld();
    residentAI.resetWorld();
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
    const newWorldSpeed = observerStore.getSnapshot().simulationSpeed;
    simulationClock?.setSpeed(newWorldSpeed);
    threeWorldRenderer?.setSimulationSpeed(newWorldSpeed);
    threeWorldRenderer?.resetSocialEvents();
    threeWorldRenderer?.setSelectedResident(null);
    threeWorldRenderer?.setSelectedHumanTrace(null);
    threeWorldRenderer?.setSelectedSettlement(null);
    refresh();
    // Building the initial terrain is presentation work, not elapsed life.
    // Start the new world's wall-clock baseline after that synchronous work.
    simulationClock?.resetAccumulator();
  }

  function selectResident(residentId: string | null): void {
    observerStore.selectResident(residentId);
    threeWorldRenderer?.setSelectedHumanTrace(null);
    threeWorldRenderer?.setSelectedSettlement(null);
    threeWorldRenderer?.setSelectedResident(
      observerStore.getSnapshot().selectedResidentId,
    );
  }

  function selectHumanTrace(id: string | null, focus = false): void {
    observerStore.selectHumanTrace(id);
    const selectedId = observerStore.getSnapshot().selectedHumanTraceId;
    threeWorldRenderer?.setSelectedResident(null);
    threeWorldRenderer?.setSelectedHumanTrace(selectedId);
    threeWorldRenderer?.setSelectedSettlement(observerStore.getSnapshot().selectedSettlementId);
    const trace = visibleHumanTraces(terrain).find(candidate => candidate.id === selectedId);
    if (!focus || !trace || !worldSession) return;
    const target = humanTraceFocus(trace);
    localPanX = target.panX;
    localPanZ = target.panZ;
    worldSession.moveObserver(target.centerChunkX - centerX, target.centerChunkY - centerY);
    refresh();
  }

  function selectSettlement(id: string | null, focus = false): void {
    observerStore.selectSettlement(id);
    const selectedId = observerStore.getSnapshot().selectedSettlementId;
    threeWorldRenderer?.setSelectedResident(null);
    threeWorldRenderer?.setSelectedHumanTrace(null);
    threeWorldRenderer?.setSelectedSettlement(selectedId);
    const settlement = observerStore.getSnapshot().civilization.settlements?.find(entry => entry.id === selectedId);
    if (!focus || !settlement || !worldSession) return;
    const target = humanTraceFocus(settlement);
    localPanX = target.panX;
    localPanZ = target.panZ;
    worldSession.moveObserver(target.centerChunkX - centerX, target.centerChunkY - centerY);
    refresh();
  }

  function focusObservation(event: ObservationEvent): void {
    if (!worldSession) return;

    const preferredResidentId = event.residentId
      && residentSnapshot.some((resident) => resident.id === event.residentId)
      ? event.residentId
      : event.targetResidentId
        && residentSnapshot.some(
          (resident) => resident.id === event.targetResidentId,
        )
        ? event.targetResidentId
        : null;

    const fallbackResident = preferredResidentId
      ? residentSnapshot.find(
          (resident) => resident.id === preferredResidentId,
        )
      : undefined;
    const gridX = event.focusGridX !== undefined
      && Number.isFinite(event.focusGridX)
      ? event.focusGridX
      : fallbackResident?.hasPosition
        ? fallbackResident.gridX
        : undefined;
    const gridY = event.focusGridY !== undefined
      && Number.isFinite(event.focusGridY)
      ? event.focusGridY
      : fallbackResident?.hasPosition
        ? fallbackResident.gridY
        : undefined;

    observerStore.focusObservation(event.id, preferredResidentId);
    threeWorldRenderer?.setSelectedHumanTrace(null);
    threeWorldRenderer?.setSelectedResident(preferredResidentId);
    threeWorldRenderer?.setSelectedSettlement(null);

    if (gridX === undefined || gridY === undefined) return;
    const target = humanTraceFocus({ gridX, gridY });
    followResidents = false;
    localPanX = target.panX;
    localPanZ = target.panZ;
    worldSession.moveObserver(
      target.centerChunkX - centerX,
      target.centerChunkY - centerY,
    );
    refresh();
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
    if (fastForwardRunning) return;
    const canonicalSpeed = normalizeSimulationSpeed(speed);
    simulationClock?.setSpeed(canonicalSpeed);
    threeWorldRenderer?.setSimulationSpeed(canonicalSpeed);
    observerStore.update({ simulationSpeed: canonicalSpeed });
    refreshSafely();
  }

  function yieldToBrowser(): Promise<void> {
    return new Promise((resolve) => window.setTimeout(resolve, 0));
  }

  async function fastForwardDays(days: number): Promise<void> {
    if (!worldSession || fastForwardRunning) return;

    const requestedDays = Math.floor(Number(days));
    if (
      !Number.isFinite(requestedDays)
      || requestedDays < 1
      || requestedDays > MAX_FAST_FORWARD_DAYS
    ) {
      observerStore.failFastForward(
        `건너뛸 일수는 1일부터 ${MAX_FAST_FORWARD_DAYS}일까지 입력해 주세요.`,
      );
      return;
    }

    const totalMinutes = requestedDays * MINUTES_PER_DAY;
    fastForwardRunning = true;
    shadowObserver.suspend(true);
    residentAI.suspend(true);
    simulationClock?.stop();
    threeWorldRenderer?.breakFootTrafficContinuity();

    try {
      const beforeSnapshot = refresh(true);
      if (!beforeSnapshot) throw new Error('고속 진행 전 세계 기록을 읽지 못했습니다.');
      const before = captureFastForwardState({
        world: beforeSnapshot.overview,
        residents: beforeSnapshot.residentDetails,
        civilization: beforeSnapshot.civilization,
        worldObjects: beforeSnapshot.worldObjects,
      });

      observerStore.beginFastForward(requestedDays, totalMinutes);

      let completedMinutes = 0;
      while (completedMinutes < totalMinutes) {
        const chunkMinutes = Math.min(
          FAST_FORWARD_CHUNK_MINUTES,
          totalMinutes - completedMinutes,
        );
        worldSession.runMinutes(chunkMinutes);
        completedMinutes += chunkMinutes;
        observerStore.updateFastForwardProgress(completedMinutes);
        await yieldToBrowser();
      }

      worldSession.recenterToResidents();
      threeWorldRenderer?.breakFootTrafficContinuity();
      const afterSnapshot = refresh(true);
      if (!afterSnapshot) throw new Error('고속 진행 후 세계 기록을 읽지 못했습니다.');
      const after = captureFastForwardState({
        world: afterSnapshot.overview,
        residents: afterSnapshot.residentDetails,
        civilization: afterSnapshot.civilization,
        worldObjects: afterSnapshot.worldObjects,
      });

      observerStore.completeFastForward(
        buildFastForwardSummary(requestedDays, before, after),
      );
    } catch (error) {
      const message = error instanceof Error
        ? error.message
        : '고속 진행 중 알 수 없는 오류가 발생했습니다.';
      runtimeDiagnostics.recordCoreFailure();
      observerStore.failFastForward(message);
      console.error('LifeLens day fast-forward failed', error);
    } finally {
      fastForwardRunning = false;
      if (!stopped) {
        shadowObserver.suspend(false);
        residentAI.suspend(false);
      } else ownedCore?.dispose();
      threeWorldRenderer?.breakFootTrafficContinuity();
      simulationClock?.resetAccumulator();
      if (!stopped) simulationClock?.start();
    }
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
  
  const unbindActions = observerActions.bind({
    createWorld,
    createEarthWorld,
    setSimulationSpeed,
    fastForwardDays,
    clearFastForwardResult: () => observerStore.clearFastForwardResult(),
    moveObserver: move,
    recenterObserver,
    recoverRendering: () => enterCanvasFailover('화면이 보이지 않아 수동 복구를 요청했습니다.'),
    selectResident,
    selectHumanTrace,
    selectSettlement,
    focusObservation,
    clearObservationFocus: () => observerStore.clearObservationFocus(),
  });
  
  const resizeObserver = new ResizeObserver(resizeCanvas);
  resizeObserver.observe(canvas);
  
  async function boot(): Promise<void> {
    observerStore.setRuntime('loading');
  
    try {
      const core = await LifeLensCoreBridge.connect();
      if (stopped) { core.dispose(); return; }
      ownedCore = core;
      shadowObserver.attach(core);
      residentAI.attach(core);
      // Every world has resident agency from boot. Discover a local language
      // inference engine automatically; no user-toggle or network cloud key.
      residentAI.startAutomatically();
      worldSession = new WorldSession(core, residentContinuity);
      observerStore.setRuntime('ready');
      createWorld('');
      startSimulationClock();
    } catch (error) {
      if (stopped) return;
      const message = error instanceof Error ? error.message : String(error);
      console.error('LifeLensCore boot failed', error);
      observerStore.setRuntime('error', message);
      drawWorld();
    }
  }
  
  void boot();
  return () => {
    stopped = true;
    shadowObserver.dispose();
    residentAI.dispose();
    simulationClock?.stop();
    unbindActions();
    resizeObserver.disconnect();
    cameraInput.dispose();
    threeWorldRenderer?.dispose();
    renderFailureBanner?.remove();
    // Yielded fast-forward work retains its Core until it finishes.
    if (!fastForwardRunning) ownedCore?.dispose();
    worldSession = null;
  };
  }

