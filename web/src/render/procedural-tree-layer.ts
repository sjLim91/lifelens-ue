import * as THREE from 'three';
import type { DynamicEnvironment, TerrainChunk } from '../runtime/core-types';
import { clamp01, snowPresentationCoverage } from './environment-surface-presentation';
import { acquireTreeGeometry, TREE_TEMPLATES, type TreeSpecies } from './procedural-tree-generator';

export const PROCEDURAL_TREE_VARIANTS = TREE_TEMPLATES.flatMap(t =>
  [0, 1].map(variant => ({ species: t.id, seed: `${t.id}:${variant}` })));

export function treeVariantForEnvironment(chunk: TerrainChunk, choice: number): number {
  const cold = /boreal|taiga|alpine|tundra/i.test(chunk.biome ?? '')
    || (Number.isFinite(chunk.temperature01) && Number(chunk.temperature01) < 0.3);
  const wet = Number(chunk.moisture01) > 0.65;
  const species = cold ? (choice < 0.8 ? 2 : 1)
    : wet ? (choice < 0.48 ? 0 : choice < 0.84 ? 1 : 2)
      : choice < 0.65 ? 0 : choice < 0.82 ? 1 : 2;
  return species * 2 + (choice * 19 % 1 < 0.5 ? 0 : 1);
}

/** Uniform weather affects shape/color only, never stock or placement. */
export class PlantWeatherMaterial {
  readonly uniforms = {
    llWind: { value: 0 }, llTime: { value: 0 }, llAutumn: { value: 0 },
    llLeafDensity: { value: 1 }, llSnow: { value: 0 }, llOrigin: { value: new THREE.Vector2() },
  };
  install(material: THREE.MeshLambertMaterial, evergreen = false, grass = false): void {
    material.onBeforeCompile = shader => {
      Object.assign(shader.uniforms, this.uniforms);
      shader.vertexShader = `uniform float llWind; uniform float llTime; uniform vec2 llOrigin;
        attribute float llLeafRank; varying float vLlLeafRank;\n` + shader.vertexShader;
      shader.vertexShader = shader.vertexShader.replace('#include <begin_vertex>', `
        #include <begin_vertex>
        vLlLeafRank = llLeafRank;
        vec4 llAnchor = vec4(0.0, 0.0, 0.0, 1.0);
        #ifdef USE_INSTANCING
          llAnchor = instanceMatrix * llAnchor;
        #endif
        float llPhase = dot(llAnchor.xz + llOrigin, vec2(0.37, 0.29));
        float llBend = pow(max(position.y, 0.0), 2.0) * llWind;
        transformed.x += sin(llTime * 1.3 + llPhase) * llBend * ${grass ? '0.12' : '0.016'};
        transformed.z += cos(llTime * 0.9 + llPhase) * llBend * ${grass ? '0.08' : '0.011'};
      `);
      shader.fragmentShader = `uniform float llAutumn; uniform float llLeafDensity; uniform float llSnow;
        varying float vLlLeafRank;\n` + shader.fragmentShader;
      shader.fragmentShader = shader.fragmentShader.replace('#include <color_fragment>', `
        #include <color_fragment>
        ${evergreen || grass ? '' : 'if (vLlLeafRank > llLeafDensity) discard;'}
        ${evergreen ? '' : 'diffuseColor.rgb = mix(diffuseColor.rgb, diffuseColor.rgb * vec3(1.32, 0.79, 0.56), llAutumn);'}
        diffuseColor.rgb = mix(diffuseColor.rgb, vec3(0.84, 0.88, 0.89), llSnow * 0.5);
      `);
    };
    material.customProgramCacheKey = () => `lifelens-plant-weather-v1:${evergreen}:${grass}`;
  }
  setEnvironment(environment: DynamicEnvironment | null, originX = 0, originZ = 0): void {
    const e = environment?.available === false ? null : environment;
    this.uniforms.llWind.value = clamp01(e?.windIntensity01);
    this.uniforms.llTime.value = Number.isFinite(e?.simulationMinute) ? Number(e!.simulationMinute) : 0;
    this.uniforms.llAutumn.value = e?.calendar?.season === 'Autumn' ? 0.8 : 0;
    this.uniforms.llLeafDensity.value = e?.calendar?.season === 'Winter' ? 0.18 : 1;
    this.uniforms.llSnow.value = snowPresentationCoverage(e);
    this.uniforms.llOrigin.value.set(originX, originZ);
  }
}

export class ProceduralTreeAsset {
  readonly group = new THREE.Group();
  readonly levels: { bark: THREE.InstancedMesh; foliage: THREE.InstancedMesh }[];
  readonly weather = new PlantWeatherMaterial();
  private readonly packed: Float32Array;
  private readonly tints: Float32Array;
  private readonly matrix = new THREE.Matrix4();
  private readonly color = new THREE.Color();
  private readonly center = new THREE.Vector3();
  private readonly selected: Uint8Array;
  private readonly releases: (() => void)[] = [];
  private disposed = false;
  private cameraPosition = new THREE.Vector3(Infinity, Infinity, Infinity);
  count = 0;
  get matrices(): Float32Array { return this.packed.subarray(0, this.count * 16); }

  constructor(species: TreeSpecies, seed: string, private readonly capacity: number, private readonly mobile = false) {
    this.packed = new Float32Array(capacity * 16); this.tints = new Float32Array(capacity * 3);
    this.selected = new Uint8Array(capacity); this.selected.fill(255);
    const barkMaterial = new THREE.MeshLambertMaterial({ vertexColors: true });
    const leafMaterial = new THREE.MeshLambertMaterial({ vertexColors: true, side: THREE.DoubleSide });
    this.weather.install(leafMaterial, species === 'pine');
    this.levels = (this.mobile ? [true] : [false, true]).map(low => {
      const cached = acquireTreeGeometry(species, seed, low);
      this.releases.push(cached.release);
      // Geometry holds only static vertex attributes. Instance data belongs to meshes.
      const g = cached.geometry;
      const bark = new THREE.InstancedMesh(g.bark, barkMaterial, capacity);
      const foliage = new THREE.InstancedMesh(g.foliage, leafMaterial, capacity);
      for (const mesh of [bark, foliage]) {
        mesh.count = 0; mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
        mesh.frustumCulled = true; mesh.castShadow = false; mesh.receiveShadow = true;
        this.group.add(mesh);
      }
      return { bark, foliage };
    });
  }
  setInstances(matrices: Float32Array, count: number, tints: Float32Array): void {
    this.count = Math.max(0, Math.min(this.capacity, count));
    this.packed.set(matrices.subarray(0, this.count * 16)); this.tints.set(tints.subarray(0, this.count * 3));
    this.selected.fill(255); this.repack();
  }
  update(camera: THREE.Camera): void {
    camera.getWorldPosition(this.center);
    if (this.center.distanceToSquared(this.cameraPosition) < 4) return;
    this.cameraPosition.copy(this.center); this.repack();
  }
  private repack(): void {
    const counts = [0, 0]; let changed = false;
    const threshold = this.mobile ? 20 : 42;
    for (let i = 0; i < this.count; i++) {
      this.matrix.fromArray(this.packed, i * 16); this.center.setFromMatrixPosition(this.matrix);
      const lod = this.mobile ? 0 : this.center.distanceTo(this.cameraPosition) > threshold ? 1 : 0;
      if (this.selected[i] !== lod) changed = true;
      this.selected[i] = lod; counts[lod]++;
    }
    if (!changed && this.levels.every((level, i) => level.bark.count === counts[i])) return;
    const slots = [0, 0];
    for (let i = 0; i < this.count; i++) {
      const level = this.levels[this.selected[i]], index = slots[this.selected[i]]++;
      this.matrix.fromArray(this.packed, i * 16);
      level.bark.setMatrixAt(index, this.matrix); level.foliage.setMatrixAt(index, this.matrix);
      this.color.fromArray(this.tints, i * 3); level.foliage.setColorAt(index, this.color);
    }
    this.levels.forEach((level, i) => {
      for (const mesh of [level.bark, level.foliage]) {
        mesh.count = counts[i]; mesh.instanceMatrix.needsUpdate = true;
        if (mesh.instanceColor) mesh.instanceColor.needsUpdate = true;
        mesh.computeBoundingSphere();
      }
    });
  }
  dispose(): void {
    if (this.disposed) return; this.disposed = true;
    const materials = new Set<THREE.Material>();
    for (const { bark, foliage } of this.levels) for (const mesh of [bark, foliage]) {
      mesh.dispose(); materials.add(mesh.material as THREE.Material);
    }
    materials.forEach(m => m.dispose()); this.releases.forEach(release => release()); this.group.clear();
  }
}
