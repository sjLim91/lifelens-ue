import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import type {
  DynamicEnvironment,
  TerrainWindow,
  WorldPresentationSnapshot,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';

type PhotorealAssetId =
  | 'boulder_01'
  | 'shrub_03'
  | 'tree_stump_01';

interface WebNatureManifest {
  assets?: Record<string, {
    models?: string[];
  }>;
}

interface PhotorealTemplate {
  geometry: THREE.BufferGeometry;
  material: THREE.Material | THREE.Material[];
  triangles: number;
}

interface PhotorealEntry {
  template: PhotorealTemplate;
  mesh: THREE.InstancedMesh;
  maxInstances: number;
}

const INSTANCE_CAPS: Record<PhotorealAssetId, number> = {
  boulder_01: 16,
  shrub_03: 36,
  tree_stump_01: 24,
};

const TRIANGLE_BUDGETS: Record<PhotorealAssetId, number> = {
  boulder_01: 320_000,
  shrub_03: 240_000,
  tree_stump_01: 220_000,
};

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function hash01(
  seed: string,
  x: number,
  y: number,
  salt: number,
): number {
  const input = `${seed}:${x}:${y}:${salt}`;
  let hash = 2166136261 >>> 0;
  for (let index = 0; index < input.length; index += 1) {
    hash ^= input.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

function materialArray(
  material: THREE.Material | THREE.Material[],
): THREE.Material[] {
  return Array.isArray(material) ? material : [material];
}

export class PhotorealNatureLayer {
  readonly group = new THREE.Group();

  private readonly loader = new GLTFLoader();
  private readonly entries = new Map<PhotorealAssetId, PhotorealEntry>();
  private readonly matrix = new THREE.Matrix4();
  private readonly position = new THREE.Vector3();
  private readonly scale = new THREE.Vector3();
  private readonly rotation = new THREE.Quaternion();
  private readonly euler = new THREE.Euler();

  private terrain: TerrainWindow | null = null;
  private presentation: WorldPresentationSnapshot | null = null;
  private environment: DynamicEnvironment | null = null;
  private zoom = 1.25;
  private loading = false;
  private loaded = false;
  private unavailable = false;
  private lastSignature = '';

  constructor() {
    this.group.visible = false;
  }

  setTerrain(window: TerrainWindow): void {
    this.terrain = window;
    if (this.loaded) this.rebuildIfNeeded();
  }

  setPresentation(
    snapshot: WorldPresentationSnapshot | null,
    window: TerrainWindow,
  ): void {
    this.presentation = snapshot;
    this.terrain = window;
    if (this.loaded) this.rebuildIfNeeded();
  }

  setCameraZoom(zoom: number): void {
    this.zoom = Math.max(0.1, Number(zoom) || 1);
    const shouldShow = this.zoom >= 1.02;
    this.group.visible = shouldShow && this.loaded;
    if (shouldShow && !this.loaded && !this.unavailable) {
      void this.ensureLoaded();
    }
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.environment = environment;
    const snow = environment?.precipitationType === 'Snow'
      ? clamp01(environment?.precipitationIntensity01)
      : 0;
    const wetness = clamp01(environment?.surfaceWetness01);

    for (const entry of this.entries.values()) {
      for (const material of materialArray(entry.mesh.material)) {
        if (!(material instanceof THREE.MeshStandardMaterial)) continue;

        if (!material.userData.lifeLensBaseColor) {
          material.userData.lifeLensBaseColor = material.color.clone();
        }
        const baseColor = material.userData.lifeLensBaseColor;
        if (baseColor instanceof THREE.Color) {
          material.color.copy(baseColor);
          material.color.multiplyScalar(1 - wetness * 0.06);
          if (snow > 0.02) {
            material.color.lerp(
              new THREE.Color(0xe0e6e0),
              Math.min(0.22, snow * 0.2),
            );
          }
        }

        const baseRoughness = Number(
          material.userData.lifeLensBaseRoughness
            ?? material.roughness,
        );
        material.userData.lifeLensBaseRoughness = baseRoughness;
        material.roughness = Math.max(
          0.34,
          baseRoughness - wetness * 0.18,
        );
      }
    }
  }

  dispose(): void {
    for (const entry of this.entries.values()) {
      this.group.remove(entry.mesh);
      entry.template.geometry.dispose();
      for (const material of materialArray(entry.template.material)) {
        material.dispose();
      }
    }
    this.entries.clear();
  }

  private assetRoot(): string {
    return `${import.meta.env.BASE_URL}vendor/lifelens-nature`;
  }

  private async ensureLoaded(): Promise<void> {
    if (this.loading || this.loaded || this.unavailable) return;
    this.loading = true;

    try {
      const response = await fetch(
        `${this.assetRoot()}/manifest.json`,
        { cache: 'force-cache' },
      );
      if (!response.ok) {
        throw new Error(
          `nature manifest unavailable: HTTP ${response.status}`,
        );
      }
      const manifest = await response.json() as WebNatureManifest;

      const ids: PhotorealAssetId[] = [
        'boulder_01',
        'shrub_03',
        'tree_stump_01',
      ];
      for (const id of ids) {
        const model = manifest.assets?.[id]?.models?.[0];
        if (!model) continue;
        try {
          const template = await this.loadTemplate(
            `${this.assetRoot()}/${id}/${model}`,
          );
          const triangleBudget = TRIANGLE_BUDGETS[id];
          const maxInstances = Math.max(
            1,
            Math.min(
              INSTANCE_CAPS[id],
              Math.floor(
                triangleBudget / Math.max(1, template.triangles),
              ),
            ),
          );
          const mesh = new THREE.InstancedMesh(
            template.geometry,
            template.material,
            maxInstances,
          );
          mesh.count = 0;
          mesh.castShadow = false;
          mesh.receiveShadow = false;
          mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
          mesh.frustumCulled = true;
          this.entries.set(id, {
            template,
            mesh,
            maxInstances,
          });
          this.group.add(mesh);
        } catch (error) {
          console.warn(
            `LifeLens photoreal nature asset failed: ${id}`,
            error,
          );
        }
      }

      this.loaded = this.entries.size > 0;
      this.unavailable = !this.loaded;
      this.group.visible = this.loaded && this.zoom >= 1.02;
      if (this.loaded) {
        this.setEnvironment(this.environment);
        this.rebuildIfNeeded(true);
      }
    } catch (error) {
      this.unavailable = true;
      console.warn(
        'LifeLens photoreal nature pack unavailable; procedural fallback remains active',
        error,
      );
    } finally {
      this.loading = false;
    }
  }

  private async loadTemplate(url: string): Promise<PhotorealTemplate> {
    const gltf = await this.loader.loadAsync(url);
    gltf.scene.updateMatrixWorld(true);

    let source: THREE.Mesh | null = null;
    gltf.scene.traverse((object) => {
      if (!source && object instanceof THREE.Mesh) {
        source = object;
      }
    });
    if (!source) {
      throw new Error(`no Mesh found in ${url}`);
    }

    const sourceMesh = source as THREE.Mesh;
    const geometry = sourceMesh.geometry.clone();
    geometry.applyMatrix4(sourceMesh.matrixWorld);
    geometry.computeBoundingBox();

    const box = geometry.boundingBox;
    if (!box) throw new Error(`missing bounds for ${url}`);

    const center = box.getCenter(new THREE.Vector3());
    geometry.translate(-center.x, -box.min.y, -center.z);
    geometry.computeBoundingBox();

    const normalizedBox = geometry.boundingBox;
    if (!normalizedBox) {
      throw new Error(`missing normalized bounds for ${url}`);
    }
    const size = normalizedBox.getSize(new THREE.Vector3());
    const maxDimension = Math.max(
      0.001,
      size.x,
      size.y,
      size.z,
    );
    geometry.scale(
      1 / maxDimension,
      1 / maxDimension,
      1 / maxDimension,
    );
    geometry.computeVertexNormals();

    const triangles = geometry.index
      ? Math.max(1, Math.floor(geometry.index.count / 3))
      : Math.max(
          1,
          Math.floor(
            geometry.getAttribute('position').count / 3,
          ),
        );

    const material = Array.isArray(sourceMesh.material)
      ? sourceMesh.material.map((item) => item.clone())
      : sourceMesh.material.clone();

    return { geometry, material, triangles };
  }

  private signature(): string {
    const terrain = this.terrain;
    if (!terrain?.available) return 'unavailable';

    const chunks = terrain.chunks.map((chunk) => [
      chunk.x,
      chunk.y,
      Number(chunk.rockCoverage01 ?? 0).toFixed(2),
      Number(chunk.shrubCoverage01 ?? 0).toFixed(2),
      chunk.waterKind,
    ].join(':')).join('|');

    const wood = (this.presentation?.resources ?? [])
      .filter((resource) => resource.material === 'Wood')
      .map((resource) => (
        `${resource.id}:${resource.quantity}:${resource.maxQuantity ?? 0}:${resource.gridX}:${resource.gridY}`
      ))
      .join('|');

    return [
      terrain.worldSeed ?? '0',
      terrain.centerChunkX,
      terrain.centerChunkY,
      chunks,
      wood,
    ].join('#');
  }

  private rebuildIfNeeded(force = false): void {
    const terrain = this.terrain;
    if (!terrain?.available || !this.loaded) return;

    const signature = this.signature();
    if (!force && signature === this.lastSignature) return;
    this.lastSignature = signature;

    const seed = terrain.worldSeed ?? '0';
    const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const cells = WORLD_GRID_CONTRACT.gridCellsPerChunk;
    const elevationScale = WORLD_GRID_CONTRACT.elevationScale;
    const sampleElevation = createTerrainElevationSampler(terrain);

    const boulder = this.entries.get('boulder_01');
    const shrub = this.entries.get('shrub_03');
    const stump = this.entries.get('tree_stump_01');
    let boulderCount = 0;
    let shrubCount = 0;
    let stumpCount = 0;

    const place = (
      entry: PhotorealEntry,
      index: number,
      x: number,
      y: number,
      z: number,
      uniformScale: number,
      yaw: number,
      tiltX = 0,
      tiltZ = 0,
    ): void => {
      this.position.set(x, y, z);
      this.euler.set(tiltX, yaw, tiltZ);
      this.rotation.setFromEuler(this.euler);
      this.scale.setScalar(uniformScale);
      this.matrix.compose(
        this.position,
        this.rotation,
        this.scale,
      );
      entry.mesh.setMatrixAt(index, this.matrix);
    };

    for (const chunk of terrain.chunks) {
      if (
        chunk.waterKind === 'Ocean'
        || chunk.waterKind === 'Coast'
        || chunk.waterKind === 'Lake'
      ) {
        continue;
      }

      const centerX = (
        chunk.x - terrain.centerChunkX
      ) * size;
      const centerZ = (
        chunk.y - terrain.centerChunkY
      ) * size;

      if (
        boulder
        && boulderCount < boulder.maxInstances
        && clamp01(chunk.rockCoverage01) >= 0.32
      ) {
        const roll = hash01(seed, chunk.x, chunk.y, 401);
        const threshold = clamp01(
          Number(chunk.rockCoverage01) * 0.68,
        );
        if (roll < threshold) {
          const localX = 0.18 + hash01(seed, chunk.x, chunk.y, 402) * 0.64;
          const localY = 0.18 + hash01(seed, chunk.x, chunk.y, 403) * 0.64;
          const x = centerX + (localX - 0.5) * size;
          const z = centerZ + (localY - 0.5) * size;
          const y = sampleElevation(
            chunk.x,
            chunk.y,
            localX,
            localY,
          ) * elevationScale - 0.16;
          place(
            boulder,
            boulderCount,
            x,
            y,
            z,
            1.25 + hash01(seed, chunk.x, chunk.y, 404) * 1.15,
            hash01(seed, chunk.x, chunk.y, 405) * Math.PI * 2,
            (hash01(seed, chunk.x, chunk.y, 406) - 0.5) * 0.22,
            (hash01(seed, chunk.x, chunk.y, 407) - 0.5) * 0.22,
          );
          boulderCount += 1;
        }
      }

      if (
        shrub
        && shrubCount < shrub.maxInstances
        && clamp01(chunk.shrubCoverage01) >= 0.22
      ) {
        const roll = hash01(seed, chunk.x, chunk.y, 421);
        const threshold = clamp01(
          Number(chunk.shrubCoverage01) * 0.72,
        );
        if (roll < threshold) {
          const localX = 0.12 + hash01(seed, chunk.x, chunk.y, 422) * 0.76;
          const localY = 0.12 + hash01(seed, chunk.x, chunk.y, 423) * 0.76;
          const x = centerX + (localX - 0.5) * size;
          const z = centerZ + (localY - 0.5) * size;
          const y = sampleElevation(
            chunk.x,
            chunk.y,
            localX,
            localY,
          ) * elevationScale;
          place(
            shrub,
            shrubCount,
            x,
            y,
            z,
            0.8 + hash01(seed, chunk.x, chunk.y, 424) * 0.65,
            hash01(seed, chunk.x, chunk.y, 425) * Math.PI * 2,
          );
          shrubCount += 1;
        }
      }
    }

    if (stump) {
      for (const resource of this.presentation?.resources ?? []) {
        if (
          resource.material !== 'Wood'
          || stumpCount >= stump.maxInstances
        ) {
          continue;
        }
        const maxQuantity = Math.max(
          1,
          Number(resource.maxQuantity)
          || Number(resource.quantity)
          || 1,
        );
        const depletion = 1 - clamp01(
          (Number(resource.quantity) || 0) / maxQuantity,
        );
        if (depletion < 0.28) continue;

        const chunkX = Math.floor(resource.gridX / cells);
        const chunkY = Math.floor(resource.gridY / cells);
        const localX = (
          resource.gridX - chunkX * cells
        ) / cells;
        const localY = (
          resource.gridY - chunkY * cells
        ) / cells;
        const x = (
          chunkX - terrain.centerChunkX + localX - 0.5
        ) * size;
        const z = (
          chunkY - terrain.centerChunkY + localY - 0.5
        ) * size;
        const y = sampleElevation(
          chunkX,
          chunkY,
          localX,
          localY,
        ) * elevationScale - 0.03;
        place(
          stump,
          stumpCount,
          x,
          y,
          z,
          0.62 + depletion * 0.45,
          hash01(
            seed,
            resource.gridX,
            resource.gridY,
            441,
          ) * Math.PI * 2,
        );
        stumpCount += 1;
      }
    }

    if (boulder) {
      boulder.mesh.count = boulderCount;
      boulder.mesh.instanceMatrix.needsUpdate = true;
    }
    if (shrub) {
      shrub.mesh.count = shrubCount;
      shrub.mesh.instanceMatrix.needsUpdate = true;
    }
    if (stump) {
      stump.mesh.count = stumpCount;
      stump.mesh.instanceMatrix.needsUpdate = true;
    }
  }
}
