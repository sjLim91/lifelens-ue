import { useMobileVegetationProfile } from './vegetation-profile';
import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainWindow,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import {
  buildFlowWaterSurfaceGeometry,
  buildOpenWaterSurfaceGeometry,
} from './water-geometry';

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

export class WaterLayer {
  readonly group = new THREE.Group();

  private readonly openWaterMaterial = new THREE.MeshPhysicalMaterial({
    color: 0xffffff,
    vertexColors: true,
    transparent: false,
    opacity: 1,
    roughness: 0.56,
    metalness: 0,
    ior: 1.333,
    clearcoat: 1,
    envMapIntensity: 1.1,
    depthWrite: true,
    depthTest: true,
    dithering: true,
    side: THREE.FrontSide,
  });
  private readonly openWaterMesh = new THREE.Mesh(
    new THREE.BufferGeometry(),
    this.openWaterMaterial,
  );

  private readonly flowWaterMaterial = new THREE.MeshPhysicalMaterial({
    color: 0xffffff,
    vertexColors: true,
    transparent: false,
    opacity: 1,
    roughness: 0.6,
    metalness: 0,
    ior: 1.333,
    clearcoat: 1,
    envMapIntensity: 1.1,
    depthWrite: true,
    depthTest: true,
    dithering: true,
    side: THREE.FrontSide,
  });
  private readonly flowWaterMesh = new THREE.Mesh(
    new THREE.BufferGeometry(),
    this.flowWaterMaterial,
  );

  private surfaceSignature = '';
  private readonly mobile = useMobileVegetationProfile();
  private readonly reflectionTarget = new THREE.WebGLCubeRenderTarget(this.mobile ? 64 : 128, { type: THREE.HalfFloatType });
  private readonly reflectionCamera = new THREE.CubeCamera(0.1, 180, this.reflectionTarget);
  private reflectionFrame = 0;
  private reflectionDirty = true;
  private readonly reflectionPosition = new THREE.Vector3();
  private reflectionGenerator: THREE.PMREMGenerator | null = null;
  private reflectionMap: THREE.WebGLRenderTarget | null = null;
  private time = 0;
  private readonly waveUniforms = { llWaterTime: { value: 0 }, llWaterWind: { value: 0 }, llWaterOrigin: { value: new THREE.Vector2() } };
  private wind01 = 0;
  private rain01 = 0;
  private daylight01 = 1;

  constructor() {
    for (const material of [this.openWaterMaterial, this.flowWaterMaterial]) {
      material.onBeforeCompile = shader => {
        Object.assign(shader.uniforms, this.waveUniforms);
        shader.vertexShader = 'uniform vec2 llWaterOrigin; varying vec3 llWaterPosition;\n' + shader.vertexShader;
        shader.vertexShader = shader.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nllWaterPosition = position + vec3(llWaterOrigin.x, 0.0, llWaterOrigin.y);');
        shader.fragmentShader = 'uniform float llWaterTime; uniform float llWaterWind; varying vec3 llWaterPosition;\n' + shader.fragmentShader;
        shader.fragmentShader = shader.fragmentShader.replace('#include <normal_fragment_maps>', `
          #include <normal_fragment_maps>
          float a = sin(llWaterPosition.x * 5.1 + llWaterPosition.z * 3.4 + llWaterTime * 1.4);
          float b = cos(llWaterPosition.z * 6.3 - llWaterPosition.x * 2.3 + llWaterTime * 1.1);
          normal = normalize(normal + vec3(a, b, 0.0) * (0.015 + llWaterWind * 0.055));
        `);
      };
      material.customProgramCacheKey = () => 'lifelens-water-wave-v1';
    }
    for (const mesh of [this.openWaterMesh, this.flowWaterMesh]) {
      mesh.renderOrder = 1;
      mesh.castShadow = false;
      mesh.receiveShadow = false;
      this.group.add(mesh);
    }
  }

  setTerrain(window: TerrainWindow): void {
    const signature = [
      window.worldSeed ?? '0',
      window.centerChunkX,
      window.centerChunkY,
      ...window.chunks.map((chunk) => (
        `${chunk.x}:${chunk.y}:${chunk.waterKind}:${Number(
          chunk.elevation01,
        ).toFixed(5)}`
      )).sort(),
    ].join('|');

    if (signature === this.surfaceSignature) {
      return;
    }

    this.waveUniforms.llWaterOrigin.value.set(window.centerChunkX * WORLD_GRID_CONTRACT.worldUnitsPerChunk, window.centerChunkY * WORLD_GRID_CONTRACT.worldUnitsPerChunk);
    const previousOpen = this.openWaterMesh.geometry;
    const previousFlow = this.flowWaterMesh.geometry;

    this.openWaterMesh.geometry = buildOpenWaterSurfaceGeometry(
      window,
      WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    );
    this.flowWaterMesh.geometry = buildFlowWaterSurfaceGeometry(
      window,
      WORLD_GRID_CONTRACT.worldUnitsPerChunk,
    );

    previousOpen.dispose();
    previousFlow.dispose();
    this.surfaceSignature = signature;
    this.reflectionDirty = true;
    this.applyAllSurfaceStates();
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    if (!environment || environment.available === false) {
      this.wind01 = 0;
      this.rain01 = 0;
    } else {
      this.wind01 = clamp01(environment.windIntensity01);
      const rainy =
        environment.precipitationType === 'Rain'
        || environment.summary === 'Rain'
        || environment.summary === 'Storm';
      this.rain01 = rainy
        ? clamp01(environment.precipitationIntensity01)
        : 0;
    }

    this.applyAllSurfaceStates();
  }

  setSimulationMinute(minuteValue: number): void {
    const minute = Math.max(0, Number(minuteValue) || 0) % 1440;
    const dayAngle =
      ((minute / 1440) * Math.PI * 2) - (Math.PI * 0.5);
    const solar = Math.sin(dayAngle);
    this.daylight01 = clamp01((solar + 0.22) / 1.22);
    this.time = Math.max(0, Number(minuteValue) || 0) * 0.04;
    this.waveUniforms.llWaterTime.value = this.time;
    this.applyAllSurfaceStates();
  }

  update(_deltaSeconds: number): void {
    this.waveUniforms.llWaterWind.value = this.wind01;
  }

  /** Actual scene radiance, bounded probe resolution and refresh frequency.
   * No water is drawn during capture, avoiding recursive reflections. */
  updateReflection(renderer: THREE.WebGLRenderer, scene: THREE.Scene, camera: THREE.Camera): void {
    if (!this.openWaterMesh.geometry.getAttribute('position')?.count
      && !this.flowWaterMesh.geometry.getAttribute('position')?.count) return;
    this.reflectionFrame++;
    const moved = camera.position.distanceToSquared(this.reflectionPosition) > 64;
    if (!this.reflectionDirty && !moved && this.reflectionFrame % (this.mobile ? 240 : 120) !== 0) return;
    const previousTarget = renderer.getRenderTarget();
    const visible = this.group.visible;
    const xr = renderer.xr.enabled;
    this.group.visible = false; renderer.xr.enabled = false;
    this.reflectionCamera.position.copy(camera.position);
    this.reflectionCamera.position.y = Math.max(0.2, camera.position.y * 0.35);
    try {
      this.reflectionCamera.update(renderer, scene);
      this.reflectionGenerator ??= new THREE.PMREMGenerator(renderer);
      const next = this.reflectionGenerator.fromCubemap(this.reflectionTarget.texture);
      this.reflectionMap?.dispose(); this.reflectionMap = next;
      for (const material of [this.openWaterMaterial, this.flowWaterMaterial]) {
        const first = material.envMap === null;
        material.envMap = next.texture; if (first) material.needsUpdate = true;
      }
      this.reflectionPosition.copy(camera.position); this.reflectionDirty = false;
    } finally {
      this.group.visible = visible; renderer.xr.enabled = xr;
      renderer.setRenderTarget(previousTarget);
    }
  }

  dispose(): void {
    this.reflectionTarget.dispose(); this.reflectionMap?.dispose(); this.reflectionGenerator?.dispose();
    this.openWaterMesh.geometry.dispose();
    this.flowWaterMesh.geometry.dispose();
    this.openWaterMaterial.dispose();
    this.flowWaterMaterial.dispose();
  }

  private applyAllSurfaceStates(): void {
    const openBrightness = 0.68 + this.daylight01 * 0.32;
    this.openWaterMaterial.color.setScalar(openBrightness);
    this.openWaterMaterial.roughness = THREE.MathUtils.clamp(
      0.12 + this.wind01 * 0.28 + this.rain01 * 0.18,
      0.12,
      0.6,
    );

    const flowBrightness = 0.72 + this.daylight01 * 0.28;
    this.flowWaterMaterial.color.setScalar(flowBrightness);
    this.flowWaterMaterial.roughness = THREE.MathUtils.clamp(
      0.18 + this.wind01 * 0.25 + this.rain01 * 0.18,
      0.18,
      0.65,
    );
  }
}

