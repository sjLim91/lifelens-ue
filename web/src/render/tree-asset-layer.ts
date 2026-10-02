import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';

export interface InstancedTreeAssetOptions {
  maxInstances: number;
  url: string;
  onReady?: () => void;
}

const FOLIAGE_NAME_PATTERN =
  /(leaf|leaves|foliage|crown|canopy|needle|needles)/i;

function isFoliageMaterial(
  material: THREE.Material,
  objectName: string,
): boolean {
  const label = `${objectName} ${material.name || ''}`;
  if (FOLIAGE_NAME_PATTERN.test(label)) return true;

  if (
    material instanceof THREE.MeshStandardMaterial
    || material instanceof THREE.MeshLambertMaterial
  ) {
    const { r, g, b } = material.color;
    return g > 0.22 && g > r * 1.08 && g > b * 1.08;
  }

  return false;
}

function enableFoliageTint(material: THREE.MeshLambertMaterial): void {
  // The source Quaternius textures already carry a green albedo. A normal
  // material/instance color would multiply that green texture, making red or
  // gold variants collapse toward black. Preserve the texture's luminance and
  // alpha, then replace only its hue with the deterministic per-instance tint.
  material.color.setHex(0xffffff);
  material.onBeforeCompile = (shader) => {
    shader.vertexShader = shader.vertexShader.replace(
      '#include <common>',
      `#include <common>
attribute vec3 lifeLensFoliageTint;
varying vec3 vLifeLensFoliageTint;`,
    );
    shader.vertexShader = shader.vertexShader.replace(
      '#include <begin_vertex>',
      `#include <begin_vertex>
vLifeLensFoliageTint = lifeLensFoliageTint;`,
    );
    shader.fragmentShader = shader.fragmentShader.replace(
      '#include <common>',
      `#include <common>
varying vec3 vLifeLensFoliageTint;`,
    );
    shader.fragmentShader = shader.fragmentShader.replace(
      '#include <color_fragment>',
      `#include <color_fragment>
float lifeLensFoliageLuma = max(
  dot(diffuseColor.rgb, vec3(0.2126, 0.7152, 0.0722)),
  0.22
);
diffuseColor.rgb = vLifeLensFoliageTint * lifeLensFoliageLuma;`,
    );
  };
  material.customProgramCacheKey = () => 'lifelens-foliage-tint-v1';
}

function toMobileMaterial(
  material: THREE.Material,
  foliage: boolean,
): THREE.Material {
  if (material instanceof THREE.MeshStandardMaterial) {
    const mobile = new THREE.MeshLambertMaterial({
      name: `${material.name || 'Tree'}_Mobile`,
      color: material.color.clone(),
      map: material.map,
      emissive: material.emissive.clone(),
      emissiveMap: material.emissiveMap,
      alphaMap: material.alphaMap,
      transparent: material.transparent,
      opacity: material.opacity,
      alphaTest: material.alphaTest,
      side: material.side,
      vertexColors: material.vertexColors,
    });
    if (foliage) enableFoliageTint(mobile);
    return mobile;
  }

  const clone = material.clone();
  if (foliage && clone instanceof THREE.MeshLambertMaterial) {
    enableFoliageTint(clone);
  }
  return clone;
}

export class InstancedTreeAsset {
  readonly group = new THREE.Group();

  private readonly loader = new GLTFLoader();
  private readonly maxInstances: number;
  private readonly url: string;
  private readonly onReady?: () => void;
  private readonly packedMatrices: Float32Array;
  private readonly packedFoliageTints: Float32Array;
  private readonly scratchMatrix = new THREE.Matrix4();
  private readonly meshes: THREE.InstancedMesh[] = [];
  private instanceCount = 0;
  private ready = false;
  private disposed = false;

  constructor(options: InstancedTreeAssetOptions) {
    this.maxInstances = options.maxInstances;
    this.url = options.url;
    this.onReady = options.onReady;
    this.packedMatrices = new Float32Array(this.maxInstances * 16);
    this.packedFoliageTints = new Float32Array(this.maxInstances * 3);
    this.packedFoliageTints.fill(1);
    this.group.visible = false;
    void this.load();
  }

  setInstances(
    matrices: Float32Array,
    count: number,
    foliageTints?: Float32Array,
  ): void {
    const safeCount = Math.max(
      0,
      Math.min(this.maxInstances, Math.floor(count)),
    );
    const matrixFloatCount = safeCount * 16;
    const tintFloatCount = safeCount * 3;
    this.packedMatrices.fill(0);
    this.packedMatrices.set(
      matrices.subarray(0, matrixFloatCount),
      0,
    );
    this.packedFoliageTints.fill(1);
    if (foliageTints) {
      this.packedFoliageTints.set(
        foliageTints.subarray(0, tintFloatCount),
        0,
      );
    }
    this.instanceCount = safeCount;
    this.applyInstances();
  }

  get isReady(): boolean {
    return this.ready;
  }

  dispose(): void {
    this.disposed = true;
    for (const mesh of this.meshes) {
      mesh.geometry.dispose();
      const materials = Array.isArray(mesh.material)
        ? mesh.material
        : [mesh.material];
      for (const material of materials) material.dispose();
    }
    this.meshes.length = 0;
    this.group.clear();
  }

  private async load(): Promise<void> {
    try {
      const gltf = await this.loader.loadAsync(this.url);
      if (this.disposed) return;

      const source = gltf.scene;
      source.updateMatrixWorld(true);

      const bounds = new THREE.Box3().setFromObject(source);
      const size = bounds.getSize(new THREE.Vector3());
      const center = bounds.getCenter(new THREE.Vector3());
      const modelHeight = Math.max(0.001, size.y);
      const translateToOrigin = new THREE.Matrix4().makeTranslation(
        -center.x,
        -bounds.min.y,
        -center.z,
      );
      const normalizeHeight = new THREE.Matrix4().makeScale(
        1 / modelHeight,
        1 / modelHeight,
        1 / modelHeight,
      );

      source.traverse((object) => {
        if (!(object instanceof THREE.Mesh)) return;
        if (!(object.geometry instanceof THREE.BufferGeometry)) return;

        const sourceMaterials = Array.isArray(object.material)
          ? object.material
          : [object.material];
        const foliageFlags = sourceMaterials.map((material) => (
          isFoliageMaterial(material, object.name)
        ));
        const hasFoliage = foliageFlags.some(Boolean);

        const geometry = object.geometry.clone();
        geometry.applyMatrix4(object.matrixWorld);
        geometry.applyMatrix4(translateToOrigin);
        geometry.applyMatrix4(normalizeHeight);
        geometry.computeVertexNormals();
        geometry.computeBoundingSphere();
        if (hasFoliage) {
          geometry.setAttribute(
            'lifeLensFoliageTint',
            new THREE.InstancedBufferAttribute(
              new Float32Array(this.maxInstances * 3),
              3,
            ),
          );
        }

        const mobileMaterials = sourceMaterials.map(
          (material, index) => toMobileMaterial(
            material,
            foliageFlags[index],
          ),
        );
        const material = Array.isArray(object.material)
          ? mobileMaterials
          : mobileMaterials[0];
        const instanced = new THREE.InstancedMesh(
          geometry,
          material,
          this.maxInstances,
        );
        instanced.count = 0;
        instanced.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
        instanced.castShadow = false;
        instanced.receiveShadow = false;
        instanced.frustumCulled = true;

        this.meshes.push(instanced);
        this.group.add(instanced);
      });

      if (this.meshes.length === 0) {
        throw new Error('Tree GLB contained no renderable meshes');
      }

      this.ready = true;
      this.group.visible = true;
      this.applyInstances();
      this.onReady?.();
    } catch (error) {
      console.warn(
        'LifeLens real-tree GLB unavailable; keeping procedural fallback',
        error,
      );
    }
  }

  private applyInstances(): void {
    if (!this.ready) return;

    for (const mesh of this.meshes) {
      for (let index = 0; index < this.instanceCount; index += 1) {
        this.scratchMatrix.fromArray(
          this.packedMatrices,
          index * 16,
        );
        mesh.setMatrixAt(index, this.scratchMatrix);
      }

      const tintAttribute =
        mesh.geometry.getAttribute('lifeLensFoliageTint');
      if (tintAttribute instanceof THREE.InstancedBufferAttribute) {
        tintAttribute.array.set(this.packedFoliageTints);
        tintAttribute.needsUpdate = true;
      }

      mesh.count = this.instanceCount;
      mesh.instanceMatrix.needsUpdate = true;
      mesh.computeBoundingSphere();
    }
  }
}
