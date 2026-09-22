import * as THREE from 'three';
import type {
  DynamicEnvironment,
  Resident,
  TerrainWindow,
  WorldPresentationSnapshot,
} from '../runtime/core-types';
import { runtimeDiagnostics } from '../runtime/runtime-diagnostics';
import { WorldScene, type WorldSceneCameraState } from './world-scene';

export class WorldRenderer {
  private readonly renderer: THREE.WebGLRenderer;
  private readonly world = new WorldScene();
  private animationFrame: number | null = null;

  constructor(canvas: HTMLCanvasElement) {
    this.renderer = new THREE.WebGLRenderer({
      canvas,
      antialias: true,
      alpha: false,
      powerPreference: 'high-performance',
    });
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = 1.08;
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
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
      const renderStartedAt = performance.now();
      this.world.update(deltaSeconds);
      this.renderer.render(this.world.scene, this.world.camera);
      const info = this.renderer.info;
      runtimeDiagnostics.recordThreeRender(
        performance.now() - renderStartedAt,
        info.render.calls,
        info.render.triangles,
        info.memory.geometries,
        info.memory.textures,
      );
      this.animationFrame = requestAnimationFrame(render);
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

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.world.setEnvironment(environment);
  }

  setSimulationSpeed(speed: number): void {
    this.world.setSimulationSpeed(speed);
  }

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

  setWorldPresentation(
    snapshot: WorldPresentationSnapshot | null,
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    this.world.setWorldPresentation(
      snapshot,
      terrain,
      centerX,
      centerY,
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

  dispose(): void {
    this.stop();
    this.world.dispose();
    this.renderer.dispose();
  }
}
