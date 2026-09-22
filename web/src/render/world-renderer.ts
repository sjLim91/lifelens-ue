import * as THREE from 'three';
import type { TerrainWindow } from '../runtime/core-types';
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
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  }

  start(): void {
    if (this.animationFrame !== null) return;

    const render = (): void => {
      this.renderer.render(this.world.scene, this.world.camera);
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

  setTerrain(window: TerrainWindow): void {
    this.world.setTerrain(window);
  }

  dispose(): void {
    this.stop();
    this.world.dispose();
    this.renderer.dispose();
  }
}
