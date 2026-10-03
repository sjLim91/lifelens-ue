import * as THREE from 'three';
import type { DynamicEnvironment } from '../runtime/core-types';
import { WORLD_PRESENTATION } from './world-presentation-config';

export const clamp01 = (value: unknown): number => {
  const number = Number(value);
  return Number.isFinite(number) ? Math.max(0, Math.min(1, number)) : 0;
};

export function presentationHash01(value: string): number {
  let hash = 2166136261;
  for (let i = 0; i < value.length; i++) {
    hash ^= value.charCodeAt(i); hash = Math.imul(hash, 16777619);
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967296;
}

/** Stateless view of current Core weather. No depth, melt clock or save state.
 * Reloading at the same observation reconstructs precisely the same cover.
 * Rain explicitly excludes snow even in a cold chunk. No accumulation history.
 */
export function snowPresentationCoverage(environment: DynamicEnvironment | null): number {
  if (!environment || environment.available === false) return 0;
  const type = environment.precipitationType ?? (environment.summary === 'Snow' ? 'Snow' : 'None');
  if (type !== 'Snow' || !Number.isFinite(environment.airTemperatureC)) return 0;
  const config = WORLD_PRESENTATION.weather;
  const intensity = clamp01(environment.precipitationIntensity01);
  const cold = clamp01((config.snowWarmLimitC - Number(environment.airTemperatureC)) / config.snowColdRangeC);
  return config.snowMaxCoverage * intensity * cold * (0.85 + clamp01(environment.surfaceWetness01) * 0.15);
}

interface SurfaceState {
  snow: number;
  originX: number;
  originZ: number;
}

/** Shared uniform modifier: adds snow only on upward faces of existing meshes.
 * It never creates terrain/vegetation or changes material identity/condition.
 * Origin corrects the rebased observer window so snow mottling remains world anchored.
 */
export class SurfaceSnowModifier {
  private readonly uniforms = {
    llSnow: { value: 0 },
    llOrigin: { value: new THREE.Vector2() },
    llSnowColor: { value: new THREE.Color(WORLD_PRESENTATION.weather.snowColor) },
  };

  install(material: THREE.MeshStandardMaterial): void {
    // One modifier per material, installed before its first GPU compilation.
    const previous = material.onBeforeCompile;
    material.onBeforeCompile = (shader, renderer) => {
      previous.call(material, shader, renderer);
      Object.assign(shader.uniforms, this.uniforms);
      shader.vertexShader = 'varying vec3 llSurfacePosition;\n' + shader.vertexShader;
      shader.vertexShader = shader.vertexShader.replace('#include <project_vertex>', `
        #include <project_vertex>
        vec4 llPosition = vec4(transformed, 1.0);
        #ifdef USE_INSTANCING
          llPosition = instanceMatrix * llPosition;
        #endif
        llSurfacePosition = (modelMatrix * llPosition).xyz;
      `);
      shader.fragmentShader = `uniform float llSnow;
        uniform vec2 llOrigin;
        uniform vec3 llSnowColor;
        varying vec3 llSurfacePosition;\n` + shader.fragmentShader;
      shader.fragmentShader = shader.fragmentShader.replace('#include <normal_fragment_maps>', `
        #include <normal_fragment_maps>
        vec3 llUpNormal = inverseTransformDirection(normal, viewMatrix);
        vec2 llGround = llSurfacePosition.xz + llOrigin;
        float llMottle = 0.84 + 0.16 * sin(llGround.x * 2.1) * cos(llGround.y * 1.7);
        float llCover = llSnow * smoothstep(0.35, 0.85, llUpNormal.y) * llMottle;
        diffuseColor.rgb = mix(diffuseColor.rgb, llSnowColor, llCover);
        roughnessFactor = mix(roughnessFactor, ${WORLD_PRESENTATION.weather.snowRoughness.toFixed(2)}, llCover);
      `);
    };
    material.customProgramCacheKey = () => 'lifelens-surface-snow-v1';
  }

  setState(state: Pick<SurfaceState, 'snow' | 'originX' | 'originZ'>): void {
    this.uniforms.llSnow.value = clamp01(state.snow);
    this.uniforms.llOrigin.value.set(state.originX, state.originZ);
  }
}

/** Existing terrain vertex palette remains untouched; only its shared material changes. */
export function applyGroundWetness(material: THREE.MeshStandardMaterial, wetness: number): void {
  const config = WORLD_PRESENTATION.weather, wet = clamp01(wetness);
  material.color.setScalar(1 - wet * config.wetGroundDarkening);
  material.roughness = THREE.MathUtils.lerp(config.dryGroundRoughness, config.wetGroundRoughness, wet);
}
