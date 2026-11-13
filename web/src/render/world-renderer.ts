import * as THREE from 'three';
import type {
  CivilizationWorldPayload,
  DynamicEnvironment,
  RecentSocialEventsPayload,
  Resident,
  TerrainWindow,
  WorldObjectsPayload,
} from '../runtime/core-types';
import { WorldScene, type WorldSceneCameraState } from './world-scene';
import { useMobilePerformanceProfile } from '../runtime/device-performance';
import { RenderFramePacer } from './render-frame-pacer';
import { renderPerformance } from './render-performance';

export class WorldRenderer {
  private readonly renderer: THREE.WebGLRenderer;
  private readonly world = new WorldScene();
  private animationFrame: number | null = null;
  private failed = false;
  private readonly mobilePerformance = useMobilePerformanceProfile();
  private readonly onVisibilityChange = (): void => renderPerformance.breakContinuity();
  private readonly onWebGLContextLost = (event: Event): void => {
    event.preventDefault();
    this.fail('3D 그래픽 장치 연결이 끊어졌습니다.');
  };

  constructor(
    canvas: HTMLCanvasElement,
    private readonly onRenderFailure?: (reason: string, error?: unknown) => void,
  ) {
    // A tall Android WebView at DPR 2 consumes multiple large color/depth
    // buffers, plus reflected water and resident rigs. Bound the mobile
    // framebuffer before the GPU silently blanks the canvas.
    const compactGpu = this.mobilePerformance;
    this.renderer = new THREE.WebGLRenderer({
      canvas,
      antialias: !compactGpu,
      alpha: false,
      powerPreference: compactGpu ? 'low-power' : 'high-performance',
    });
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, compactGpu ? 1.35 : 2));
    // Count all passes (including water reflection), with one reset per frame.
    this.renderer.info.autoReset = false;
    renderPerformance.begin(this.performanceSurface());
    canvas.addEventListener('webglcontextlost', this.onWebGLContextLost);
    document.addEventListener('visibilitychange', this.onVisibilityChange);
  }

  private fail(reason: string, error?: unknown): void {
    if (this.failed) return;
    this.failed = true;
    this.stop();
    this.onRenderFailure?.(reason, error);
  }

  start(): void {
    if (this.animationFrame !== null) return;

    const frames = new RenderFramePacer(this.mobilePerformance ? 30 : null, performance.now());
    renderPerformance.resume();
    const render = (frameTime: number): void => {
      const deltaSeconds = frames.advance(frameTime);
      if (deltaSeconds === null) {
        this.animationFrame = requestAnimationFrame(render);
        return;
      }
      try {
        // Some mobile GPUs silently lose their WebGL context without throwing
        // in the render loop. Do not keep showing an empty canvas.
        if (this.renderer.getContext().isContextLost()) {
          this.fail('3D 그래픽 장치 연결이 끊어졌습니다.');
          return;
        }
        const started = performance.now();
        this.renderer.info.reset();
        this.world.update(deltaSeconds);
        this.world.updateReflections(this.renderer);
        this.renderer.render(this.world.scene, this.world.camera);
        const info = this.renderer.info;
        if (document.hidden) renderPerformance.breakContinuity();
        else renderPerformance.record(frameTime, performance.now() - started, {
          calls: info.render.calls, triangles: info.render.triangles,
          geometries: info.memory.geometries, textures: info.memory.textures,
          programs: info.programs?.length ?? 0,
        });
        this.animationFrame = requestAnimationFrame(render);
      } catch (error) {
        this.fail('3D 렌더링 도중 오류가 발생했습니다.', error);
      }
    };

    this.animationFrame = requestAnimationFrame(render);
  }

  stop(): void {
    renderPerformance.stop();
    if (this.animationFrame === null) return;
    cancelAnimationFrame(this.animationFrame);
    this.animationFrame = null;
  }

  resize(width: number, height: number): void {
    const safeWidth = Math.max(1, width);
    const safeHeight = Math.max(1, height);
    this.renderer.setSize(safeWidth, safeHeight, false);
    this.world.resize(safeWidth, safeHeight);
    renderPerformance.setSurface(this.performanceSurface());
  }

  private performanceSurface() {
    return { width: this.renderer.domElement.width, height: this.renderer.domElement.height,
      pixelRatio: this.renderer.getPixelRatio(), mobile: this.mobilePerformance };
  }

  setCamera(state: WorldSceneCameraState, immediate = false): void {
    this.world.setCamera(state, immediate);
  }

  groundPointAtClient(clientX: number, clientY: number): { x: number; z: number } | null {
    const rect = this.renderer.domElement.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return null;
    return this.world.groundPoint(
      (clientX - rect.left) / rect.width * 2 - 1,
      1 - (clientY - rect.top) / rect.height * 2,
    );
  }

  setSimulationMinute(minute: number): void {
    this.world.setSimulationMinute(minute);
  }

  setSocialEvents(payload: RecentSocialEventsPayload, minute: number): void {
    this.world.setSocialEvents(payload, minute);
  }

  resetSocialEvents(): void { this.world.resetSocialEvents(); }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.world.setEnvironment(environment);
  }

  setSimulationSpeed(speed: number): void {
    this.world.setSimulationSpeed(speed);
  }

  breakFootTrafficContinuity(): void { this.world.breakFootTrafficContinuity(); }

  pickResident(clientX: number, clientY: number): string | null {
    const rect = this.renderer.domElement.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return null;

    const normalizedX = ((clientX - rect.left) / rect.width) * 2 - 1;
    const normalizedY = -(((clientY - rect.top) / rect.height) * 2 - 1);
    return this.world.pickResident(normalizedX, normalizedY);
  }

  setSelectedResident(residentId: string | null): void {
    this.world.setSelectedResident(residentId);
  }

  pickHumanTrace(clientX: number, clientY: number): string | null {
    const rect = this.renderer.domElement.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return null;
    return this.world.pickHumanTrace(
      ((clientX - rect.left) / rect.width) * 2 - 1,
      -(((clientY - rect.top) / rect.height) * 2 - 1),
    );
  }

  setSelectedHumanTrace(id: string | null): void {
    this.world.setSelectedHumanTrace(id);
  }

  setSelectedSettlement(id: string | null): void { this.world.setSelectedSettlement(id); }

  pickSettlement(clientX: number, clientY: number): string | null {
    const rect = this.renderer.domElement.getBoundingClientRect();
    if (rect.width <= 0 || rect.height <= 0) return null;
    return this.world.pickSettlement(((clientX - rect.left) / rect.width) * 2 - 1,
      -(((clientY - rect.top) / rect.height) * 2 - 1));
  }

  setAuthoritativeSpatialTargets(
    civilization: CivilizationWorldPayload,
    worldObjects: WorldObjectsPayload,
    terrain: TerrainWindow,
  ): void {
    this.world.setAuthoritativeSpatialTargets(
      civilization,
      worldObjects,
      terrain,
    );
  }

  setResidents(
    residents: Resident[],
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    this.world.setResidents(
      residents,
      terrain,
      centerX,
      centerY,
    );
  }

  setTerrain(window: TerrainWindow): void {
    this.world.setTerrain(window);
  }

  setHumanTraces(window: TerrainWindow): void {
    this.world.setHumanTraces(window);
  }

  dispose(): void {
    this.failed = true;
    this.stop();
    document.removeEventListener('visibilitychange', this.onVisibilityChange);
    this.renderer.domElement.removeEventListener(
      'webglcontextlost', this.onWebGLContextLost,
    );
    this.world.dispose();
    this.renderer.dispose();
  }
}


