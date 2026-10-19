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

export class WorldRenderer {
  private readonly renderer: THREE.WebGLRenderer;
  private readonly world = new WorldScene();
  private animationFrame: number | null = null;
  private failed = false;
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
    const compactGpu = window.matchMedia?.('(max-width: 800px) and (pointer: coarse)').matches ?? false;
    this.renderer = new THREE.WebGLRenderer({
      canvas,
      antialias: !compactGpu,
      alpha: false,
      powerPreference: compactGpu ? 'low-power' : 'high-performance',
    });
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, compactGpu ? 1.35 : 2));
    canvas.addEventListener('webglcontextlost', this.onWebGLContextLost);
  }

  private fail(reason: string, error?: unknown): void {
    if (this.failed) return;
    this.failed = true;
    this.stop();
    this.onRenderFailure?.(reason, error);
  }

  start(): void {
    if (this.animationFrame !== null) return;

    let lastFrame = performance.now();
    const render = (frameTime: number): void => {
      const deltaSeconds = Math.min(
        0.05,
        Math.max(0, (frameTime - lastFrame) / 1000),
      );
      lastFrame = frameTime;
      try {
        // Some mobile GPUs silently lose their WebGL context without throwing
        // in the render loop. Do not keep showing an empty canvas.
        if (this.renderer.getContext().isContextLost()) {
          this.fail('3D 그래픽 장치 연결이 끊어졌습니다.');
          return;
        }
        this.world.update(deltaSeconds);
        this.world.updateReflections(this.renderer);
        this.renderer.render(this.world.scene, this.world.camera);
        this.animationFrame = requestAnimationFrame(render);
      } catch (error) {
        this.fail('3D 렌더링 도중 오류가 발생했습니다.', error);
      }
    };

    this.animationFrame = requestAnimationFrame(render);
  }

  stop(): void {
    if (this.animationFrame === null) return;
    cancelAnimationFrame(this.animationFrame);
    this.animationFrame = null;
  }

  resize(width: number, height: number): void {
    const safeWidth = Math.max(1, width);
    const safeHeight = Math.max(1, height);
    this.renderer.setSize(safeWidth, safeHeight, false);
    this.world.resize(safeWidth, safeHeight);
  }

  setCamera(state: WorldSceneCameraState): void {
    this.world.setCamera(state);
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
    this.renderer.domElement.removeEventListener(
      'webglcontextlost', this.onWebGLContextLost,
    );
    this.world.dispose();
    this.renderer.dispose();
  }
}

