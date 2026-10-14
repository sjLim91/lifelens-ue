import * as THREE from 'three';

/**
 * Makes the selected resident (or a close camera target) readable through
 * dense foliage. This is presentation-only: no vegetation is removed from
 * Core, no resource amounts or collision/pathfinding truth change.
 *
 * Dithered cutout, not material.opacity: it avoids the incorrect depth sorting
 * and full-screen transparent overdraw that dense instanced forests cause.
 */
export class ObserverFoliageClarity {
  readonly uniforms = {
    llClarityCamera: { value: new THREE.Vector3() },
    llClarityFocus: { value: new THREE.Vector3() },
    llClarityRadius: { value: 0 },
    llClarityStrength: { value: 0 },
  };
  set(camera: THREE.Camera, target: THREE.Vector3 | null, selected: boolean): void {
    if (!target) {
      this.uniforms.llClarityStrength.value = 0;
      return;
    }
    camera.getWorldPosition(this.uniforms.llClarityCamera.value);
    this.uniforms.llClarityFocus.value.copy(target);
    const distance = this.uniforms.llClarityCamera.value.distanceTo(target);
    // Do not clear a broad hole through the forest when observing Earth from
    // far above. Selection takes precedence over camera-distance thresholds.
    this.uniforms.llClarityStrength.value =
      selected ? 0.92 : distance < 34 ? 0.78 : 0;
    this.uniforms.llClarityRadius.value =
      selected ? 1.35 : THREE.MathUtils.clamp(distance * 0.045, .85, 1.45);
  }
}

/** Compose AFTER existing wind/tint shader changes, never overwrite them. */
export function injectFoliageClarity(
  shader: THREE.WebGLProgramParametersWithUniforms,
  clarity: ObserverFoliageClarity,
): void {
  Object.assign(shader.uniforms, clarity.uniforms);
  shader.vertexShader = shader.vertexShader.replace('#include <common>',
    `#include <common>
varying vec3 vLlFoliageWorld;`);
  shader.vertexShader = shader.vertexShader.replace('#include <project_vertex>',
    `#include <project_vertex>
vec4 llClarityVertex = vec4(transformed, 1.0);
#ifdef USE_INSTANCING
llClarityVertex = instanceMatrix * llClarityVertex;
#endif
vLlFoliageWorld = (modelMatrix * llClarityVertex).xyz;`);
  shader.fragmentShader = shader.fragmentShader.replace('#include <common>',
    `#include <common>
uniform vec3 llClarityCamera;
uniform vec3 llClarityFocus;
uniform float llClarityRadius;
uniform float llClarityStrength;
varying vec3 vLlFoliageWorld;`);
  shader.fragmentShader = shader.fragmentShader.replace('#include <dithering_fragment>',
    `#include <dithering_fragment>
if (llClarityStrength > 0.0) {
  vec3 ray = llClarityFocus - llClarityCamera;
  float rayLength2 = max(dot(ray, ray), 0.0001);
  float t = clamp(dot(vLlFoliageWorld-llClarityCamera, ray)/rayLength2, 0.0, 1.0);
  vec3 nearest = llClarityCamera + ray * t;
  float separation = length(vLlFoliageWorld - nearest);
  // Only the foliage obstructing the sightline, never the entire tree.
  if (t > .06 && t < .98 && separation < llClarityRadius) {
    float fade = (1.0-smoothstep(llClarityRadius*.62, llClarityRadius, separation));
    vec3 cell = floor(vLlFoliageWorld*18.0);
    float stablePattern = fract(sin(dot(cell, vec3(12.9898,78.233,37.719)))
      * 43758.5453);
    if (stablePattern < fade * llClarityStrength) discard;
  }
}`);
}
