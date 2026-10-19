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
    llResidentFocus: { value: Array.from({ length: 4 }, () => new THREE.Vector3()) },
    llResidentFocusCount: { value: 0 },
  };
  setResidents(targets: readonly THREE.Vector3[]): void {
    this.uniforms.llResidentFocusCount.value = Math.min(4, targets.length);
    for (let i = 0; i < this.uniforms.llResidentFocusCount.value; i++)
      this.uniforms.llResidentFocus.value[i].copy(targets[i]);
  }
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
uniform vec3 llResidentFocus[4];
uniform int llResidentFocusCount;
varying vec3 vLlFoliageWorld;`);
  shader.fragmentShader = shader.fragmentShader.replace('#include <dithering_fragment>',
    `#include <dithering_fragment>
float residentFade = 0.0;
for (int i = 0; i < 4; i++) {
  if (i >= llResidentFocusCount) break;
  vec3 residentRay = llResidentFocus[i] - llClarityCamera;
  float residentT = dot(vLlFoliageWorld-llClarityCamera, residentRay)
    / max(dot(residentRay, residentRay), .0001);
  float residentSeparation = length(vLlFoliageWorld - llClarityCamera - residentRay * residentT);
  if (residentT > .02 && residentT < 1.01)
    residentFade = max(residentFade, (1.0-smoothstep(.4, .85, residentSeparation)) * .96);
}
if (llClarityStrength > 0.0 || residentFade > 0.0) {
  vec3 ray = llClarityFocus - llClarityCamera;
  float rayLength2 = max(dot(ray, ray), 0.0001);
  float t = clamp(dot(vLlFoliageWorld-llClarityCamera, ray)/rayLength2, 0.0, 1.0);
  vec3 nearest = llClarityCamera + ray * t;
  float separation = length(vLlFoliageWorld - nearest);
  // Only the foliage obstructing the sightline, never the entire tree.
  float fade = residentFade;
  if (t > .06 && t < .98 && separation < llClarityRadius) {
    fade = max(fade, (1.0-smoothstep(llClarityRadius*.62, llClarityRadius, separation)) * llClarityStrength);
  }
  {
    vec3 cell = floor(vLlFoliageWorld*18.0);
    float stablePattern = fract(sin(dot(cell, vec3(12.9898,78.233,37.719)))
      * 43758.5453);
    if (stablePattern < fade) discard;
  }
}`);
}

