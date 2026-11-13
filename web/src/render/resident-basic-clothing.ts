import * as THREE from 'three';
import type { ResidentAppearanceProfile } from './resident-appearance';

const CLOTH = [0x536c62, 0x796950, 0x6b657a, 0x54717c, 0x8a6657] as const;
/** Neutral observer clothing, not crafted equipment or a Core inventory item.
 * The body surface and its actual skin weights carry it through every pose,
 * growth and pregnancy deformation, with no extra draw call/mesh/skeleton.
 */
export function installResidentBasicClothing(body: THREE.SkinnedMesh, profile: ResidentAppearanceProfile): void {
  const geometry = body.geometry;
  const position = geometry.getAttribute('position'), indices = geometry.getAttribute('skinIndex'), weights = geometry.getAttribute('skinWeight');
  if (!position || !indices || !weights) return;
  geometry.computeBoundingBox();
  const bounds = geometry.boundingBox!;
  const height = bounds.max.y - bounds.min.y;
  if (!(height > 0)) return;
  const coverage = new Float32Array(position.count * 2);
  for (let v = 0; v < position.count; v++) {
    const h = (position.getY(v) - bounds.min.y) / height;
    let torso = 0;
    for (let k = 0; k < 4; k++) {
      const name = body.skeleton.bones[indices.getComponent(v, k)]?.name ?? '';
      if (/^(root|pelvis|spine_\d+|clavicle_[lr]|thigh_[lr])$/.test(name)) torso += weights.getComponent(v, k);
    }
    // Sleeveless top and knee-length bottom; hands, face and feet remain skin.
    const hem = THREE.MathUtils.smoothstep(h, .30, .34);
    const collar = 1 - THREE.MathUtils.smoothstep(h, .79, .81);
    coverage[v * 2] = THREE.MathUtils.smoothstep(torso, .25, .65) * hem * collar;
    coverage[v * 2 + 1] = THREE.MathUtils.smoothstep(h, .51, .54);
  }
  geometry.setAttribute('llBasicClothing', new THREE.BufferAttribute(coverage, 2));
  const top = new THREE.Color(CLOTH[profile.seed % CLOTH.length]);
  const bottom = top.clone().multiplyScalar(.63);
  for (const material of Array.isArray(body.material) ? body.material : [body.material]) {
    const prior = material.onBeforeCompile, priorKey = material.customProgramCacheKey();
    material.onBeforeCompile = (shader, renderer) => {
      prior.call(material, shader, renderer);
      if (typeof shader.vertexShader !== 'string' || typeof shader.fragmentShader !== 'string') return;
      shader.uniforms.llClothTop = { value: top }; shader.uniforms.llClothBottom = { value: bottom };
      shader.vertexShader = shader.vertexShader.replace('#include <common>', '#include <common>\nattribute vec2 llBasicClothing;\nvarying vec2 vLlBasicClothing;');
      shader.vertexShader = shader.vertexShader.replace('#include <begin_vertex>', '#include <begin_vertex>\nvLlBasicClothing = llBasicClothing;');
      shader.fragmentShader = shader.fragmentShader.replace('#include <common>', '#include <common>\nvarying vec2 vLlBasicClothing;\nuniform vec3 llClothTop;\nuniform vec3 llClothBottom;');
      shader.fragmentShader = shader.fragmentShader.replace('#include <color_fragment>', `#include <color_fragment>
vec3 llCloth = mix(llClothBottom, llClothTop, vLlBasicClothing.y);
diffuseColor.rgb = mix(diffuseColor.rgb, llCloth, vLlBasicClothing.x);`);
      shader.fragmentShader = shader.fragmentShader.replace('#include <roughnessmap_fragment>', '#include <roughnessmap_fragment>\nroughnessFactor = mix(roughnessFactor, .94, vLlBasicClothing.x);');
    };
    material.customProgramCacheKey = () => priorKey + '-LifeLensBasicClothing-v1';
    material.userData.observerBasicClothing = true;
    material.needsUpdate = true;
  }
}
