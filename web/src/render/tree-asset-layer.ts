import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';

export interface InstancedTreeAssetOptions {
  maxInstances: number;
  url: string;
  onReady?: () => void;
}

function toMobileMaterial(material: THREE.Material): THREE.Material {
  if (material instanceof THREE.MeshStandardMaterial) {
    return new THREE.MeshLambertMaterial({
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
  }

  return material.clone();
}

export class InstancedTreeAsset {
  readonly group = new THREE.Group();

  private readonly loader = new GLTFLoader();
  private readonly maxInstances: number;
  private readonly url: string;
  private readonly onReady?: () => void;
  private readonly packedMatrices: Float32Array;
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
    this.group.visible = false;
    void this.load();
  }

  setInstances(matrices: Float32Array, count: number): void {
    const safeCount = Math.max(
      0,
      Math.min(this.maxInstances, Math.floor(count)),
    );
    const floatCount = safeCount * 16;
    this.packedMatrices.fill(0);
    this.packedMatrices.set(matrices.subarray(0, floatCount), 0);
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

        const geometry = object.geometry.clone();
        geometry.applyMatrix4(object.matrixWorld);
        geometry.applyMatrix4(translateToOrigin);
        geometry.applyMatrix4(normalizeHeight);
        geometry.computeVertexNormals();
        geometry.computeBoundingSphere();

        const material = Array.isArray(object.material)
          ? object.material.map(toMobileMaterial)
          : toMobileMaterial(object.material);
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

      mesh.count = this.instanceCount;
      mesh.instanceMatrix.needsUpdate = true;
      mesh.computeBoundingSphere();
    }
  }
}
