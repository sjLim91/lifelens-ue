import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainChunk,
  TerrainWindow,
  WorldPresentationSnapshot,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';

const MAX_GRASS = 7000;
const MAX_PEBBLES = 3600;
const MAX_SMALL_ROCKS = 2200;
const MAX_BOULDERS = 1200;
const MAX_DEADWOOD = 1600;
const MAX_LITTER = 3200;

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function hash01(
  seed: string,
  x: number,
  y: number,
  index: number,
  salt: number,
): number {
  const input = `${seed}:${x}:${y}:${index}:${salt}`;
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < input.length; i += 1) {
    hash ^= input.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

interface GroundWindUniforms {
  time: { value: number };
  intensity: { value: number };
}

function createGrassGeometry(): THREE.BufferGeometry {
  const positions: number[] = [];
  const indices: number[] = [];

  for (let blade = 0; blade < 3; blade += 1) {
    const angle = (Math.PI * blade) / 3;
    const dx = Math.cos(angle) * 0.07;
    const dz = Math.sin(angle) * 0.07;
    const baseIndex = positions.length / 3;

    positions.push(
      -dx, 0, -dz,
      dx, 0, dz,
      dx * 0.12, 0.46 + blade * 0.045, dz * 0.12,
    );
    indices.push(baseIndex, baseIndex + 1, baseIndex + 2);
  }

  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute(
    'position',
    new THREE.Float32BufferAttribute(positions, 3),
  );
  geometry.setIndex(indices);
  geometry.computeVertexNormals();
  return geometry;
}

function createLitterGeometry(): THREE.BufferGeometry {
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute(
    'position',
    new THREE.Float32BufferAttribute([
      -0.24, 0, -0.08,
       0.03, 0, -0.17,
       0.28, 0,  0.02,
       0.08, 0,  0.16,
      -0.19, 0,  0.13,
    ], 3),
  );
  geometry.setIndex([
    0, 1, 4,
    1, 2, 3,
    1, 3, 4,
  ]);
  geometry.computeVertexNormals();
  return geometry;
}

function gridToWorld(
  gridX: number,
  gridY: number,
  window: TerrainWindow,
): { x: number; z: number } {
  const cells = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const chunkX = Math.floor(gridX / cells);
  const chunkY = Math.floor(gridY / cells);
  const localX = (gridX - chunkX * cells) / cells;
  const localY = (gridY - chunkY * cells) / cells;
  return {
    x: (chunkX - window.centerChunkX + localX - 0.5) * size,
    z: (chunkY - window.centerChunkY + localY - 0.5) * size,
  };
}

export class GroundDetailLayer {
  readonly group = new THREE.Group();

  private readonly grassGeometry = createGrassGeometry();
  private readonly pebbleGeometry = new THREE.DodecahedronGeometry(0.075, 0);
  private readonly rockGeometry = new THREE.DodecahedronGeometry(0.17, 0);
  private readonly boulderGeometry = new THREE.DodecahedronGeometry(0.42, 1);
  private readonly deadwoodGeometry = new THREE.CylinderGeometry(
    0.045,
    0.065,
    0.72,
    6,
  );
  private readonly litterGeometry = createLitterGeometry();

  private readonly grassBase = new THREE.Color(0x5d7340);
  private readonly pebbleBase = new THREE.Color(0x77736a);
  private readonly rockBase = new THREE.Color(0x68665f);
  private readonly boulderBase = new THREE.Color(0x625f58);
  private readonly deadwoodBase = new THREE.Color(0x5c4935);
  private readonly litterBase = new THREE.Color(0x62513a);

  private readonly grassMaterial = new THREE.MeshStandardMaterial({
    color: this.grassBase,
    roughness: 1,
    metalness: 0,
    side: THREE.DoubleSide,
  });
  private readonly pebbleMaterial = new THREE.MeshStandardMaterial({
    color: this.pebbleBase,
    roughness: 1,
    metalness: 0,
  });
  private readonly rockMaterial = new THREE.MeshStandardMaterial({
    color: this.rockBase,
    roughness: 1,
    metalness: 0,
  });
  private readonly boulderMaterial = new THREE.MeshStandardMaterial({
    color: this.boulderBase,
    roughness: 1,
    metalness: 0,
  });
  private readonly deadwoodMaterial = new THREE.MeshStandardMaterial({
    color: this.deadwoodBase,
    roughness: 1,
    metalness: 0,
  });
  private readonly litterMaterial = new THREE.MeshStandardMaterial({
    color: this.litterBase,
    roughness: 1,
    metalness: 0,
    side: THREE.DoubleSide,
  });

  private readonly grass = new THREE.InstancedMesh(
    this.grassGeometry,
    this.grassMaterial,
    MAX_GRASS,
  );
  private readonly pebbles = new THREE.InstancedMesh(
    this.pebbleGeometry,
    this.pebbleMaterial,
    MAX_PEBBLES,
  );
  private readonly rocks = new THREE.InstancedMesh(
    this.rockGeometry,
    this.rockMaterial,
    MAX_SMALL_ROCKS,
  );
  private readonly boulders = new THREE.InstancedMesh(
    this.boulderGeometry,
    this.boulderMaterial,
    MAX_BOULDERS,
  );
  private readonly deadwood = new THREE.InstancedMesh(
    this.deadwoodGeometry,
    this.deadwoodMaterial,
    MAX_DEADWOOD,
  );
  private readonly litter = new THREE.InstancedMesh(
    this.litterGeometry,
    this.litterMaterial,
    MAX_LITTER,
  );

  private readonly matrix = new THREE.Matrix4();
  private readonly position = new THREE.Vector3();
  private readonly scale = new THREE.Vector3();
  private readonly rotation = new THREE.Quaternion();
  private readonly euler = new THREE.Euler();
  private readonly instanceColor = new THREE.Color();

  private terrain: TerrainWindow | null = null;
  private presentation: WorldPresentationSnapshot | null = null;
  private lastSignature = '';
  private zoom = 1.25;
  private windTime = 0;
  private windIntensity = 0;
  private grassWindUniforms: GroundWindUniforms | null = null;

  constructor() {
    this.enableGrassWind();

    for (const mesh of [
      this.grass,
      this.pebbles,
      this.rocks,
      this.boulders,
      this.deadwood,
      this.litter,
    ]) {
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.castShadow = false;
      mesh.receiveShadow = false;
      mesh.frustumCulled = true;
      this.group.add(mesh);
    }
    this.applyLod();
  }

  setTerrain(window: TerrainWindow): void {
    this.terrain = window;
    this.rebuildIfNeeded();
  }

  setPresentation(
    snapshot: WorldPresentationSnapshot | null,
    window: TerrainWindow,
  ): void {
    this.presentation = snapshot;
    this.terrain = window;
    this.rebuildIfNeeded();
  }

  setCameraZoom(zoom: number): void {
    this.zoom = Math.max(0.1, Number(zoom) || 1);
    this.applyLod();
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.windIntensity = clamp01(environment?.windIntensity01);
    if (this.grassWindUniforms) {
      this.grassWindUniforms.intensity.value = this.windIntensity;
    }

    const snow = environment?.precipitationType === 'Snow'
      ? clamp01(environment?.precipitationIntensity01)
      : 0;
    const wet = clamp01(environment?.surfaceWetness01);

    const apply = (
      material: THREE.MeshStandardMaterial,
      base: THREE.Color,
      snowMix: number,
      wetDarken: number,
    ): void => {
      material.color.copy(base);
      material.color.multiplyScalar(1 - wet * wetDarken);
      if (snow > 0.02) {
        material.color.lerp(
          new THREE.Color(0xdfe7df),
          Math.min(snowMix, snow * snowMix),
        );
      }
      material.roughness = Math.max(0.62, 1 - wet * 0.22);
    };

    apply(this.grassMaterial, this.grassBase, 0.2, 0.1);
    apply(this.pebbleMaterial, this.pebbleBase, 0.28, 0.08);
    apply(this.rockMaterial, this.rockBase, 0.3, 0.08);
    apply(this.boulderMaterial, this.boulderBase, 0.34, 0.08);
    apply(this.deadwoodMaterial, this.deadwoodBase, 0.14, 0.09);
    apply(this.litterMaterial, this.litterBase, 0.16, 0.1);
  }

  update(deltaSeconds: number): void {
    this.windTime += Math.min(
      0.05,
      Math.max(0, deltaSeconds),
    );
    if (this.grassWindUniforms) {
      this.grassWindUniforms.time.value = this.windTime;
      this.grassWindUniforms.intensity.value = this.windIntensity;
    }
  }

  private enableGrassWind(): void {
    this.grassMaterial.onBeforeCompile = (shader) => {
      const uniforms: GroundWindUniforms = {
        time: { value: this.windTime },
        intensity: { value: this.windIntensity },
      };
      this.grassWindUniforms = uniforms;
      shader.uniforms.uLifeLensGroundWindTime = uniforms.time;
      shader.uniforms.uLifeLensGroundWindIntensity = uniforms.intensity;

      shader.vertexShader = shader.vertexShader.replace(
        '#include <common>',
        `#include <common>
uniform float uLifeLensGroundWindTime;
uniform float uLifeLensGroundWindIntensity;`,
      );
      shader.vertexShader = shader.vertexShader.replace(
        '#include <begin_vertex>',
        `#include <begin_vertex>
#ifdef USE_INSTANCING
  float lifeLensGroundWindPhase =
    uLifeLensGroundWindTime * 2.15
    + instanceMatrix[3].x * 0.19
    + instanceMatrix[3].z * 0.23;
  float lifeLensBladeHeight = clamp(position.y / 0.55, 0.0, 1.0);
  float lifeLensGroundWind =
    uLifeLensGroundWindIntensity
    * 0.12
    * lifeLensBladeHeight
    * lifeLensBladeHeight;
  transformed.x += sin(lifeLensGroundWindPhase) * lifeLensGroundWind;
  transformed.z += cos(lifeLensGroundWindPhase * 0.81) * lifeLensGroundWind * 0.55;
#endif`,
      );
    };
    this.grassMaterial.customProgramCacheKey = () => 'lifelens-ground-wind-v1';
    this.grassMaterial.needsUpdate = true;
  }

  dispose(): void {
    this.grassGeometry.dispose();
    this.pebbleGeometry.dispose();
    this.rockGeometry.dispose();
    this.boulderGeometry.dispose();
    this.deadwoodGeometry.dispose();
    this.litterGeometry.dispose();
    this.grassMaterial.dispose();
    this.pebbleMaterial.dispose();
    this.rockMaterial.dispose();
    this.boulderMaterial.dispose();
    this.deadwoodMaterial.dispose();
    this.litterMaterial.dispose();
    this.grassWindUniforms = null;
  }

  private applyLod(): void {
    this.grass.visible = this.zoom >= 0.92;
    this.pebbles.visible = this.zoom >= 1.05;
    this.rocks.visible = this.zoom >= 0.78;
    this.boulders.visible = this.zoom >= 0.6;
    this.deadwood.visible = this.zoom >= 0.96;
    this.litter.visible = this.zoom >= 1.08;
  }

  private signature(): string {
    const window = this.terrain;
    if (!window?.available) return 'unavailable';

    const terrainSig = window.chunks.map((chunk) => [
      chunk.x,
      chunk.y,
      Number(chunk.elevation01).toFixed(3),
      Number(chunk.grassCoverage01 ?? 0).toFixed(2),
      Number(chunk.shrubCoverage01 ?? 0).toFixed(2),
      Number(chunk.forestCoverage01 ?? 0).toFixed(2),
      Number(chunk.rockCoverage01 ?? 0).toFixed(2),
      Number(chunk.wetlandCoverage01 ?? 0).toFixed(2),
      chunk.waterKind,
    ].join(':')).join('|');

    const facilitySig = (this.presentation?.facilities ?? [])
      .filter((facility) => facility.state !== 'Ruined')
      .map((facility) => (
        `${facility.id}:${facility.gridX}:${facility.gridY}:${facility.state}`
      ))
      .join('|');

    const resourceSig = (this.presentation?.resources ?? [])
      .filter((resource) => (
        resource.material === 'Wood'
        || resource.material === 'Fiber'
        || resource.material === 'PlantFood'
        || resource.material === 'Stone'
        || resource.material === 'Clay'
      ))
      .map((resource) => (
        `${resource.id}:${resource.material}:${resource.quantity}:${resource.maxQuantity ?? 0}:${resource.gridX}:${resource.gridY}`
      ))
      .join('|');

    const residueSig = (this.presentation?.residues ?? [])
      .map((residue) => (
        `${residue.id}:${Number(residue.intensity) || 0}:${Number(residue.radiusTiles) || 0}:${residue.gridX}:${residue.gridY}`
      ))
      .join('|');

    return [
      window.worldSeed ?? '0',
      window.centerChunkX,
      window.centerChunkY,
      terrainSig,
      facilitySig,
      resourceSig,
      residueSig,
    ].join('#');
  }

  private rebuildIfNeeded(): void {
    const window = this.terrain;
    if (!window?.available) {
      this.grass.count = 0;
      this.pebbles.count = 0;
      this.rocks.count = 0;
      this.boulders.count = 0;
      this.deadwood.count = 0;
      this.litter.count = 0;
      return;
    }

    const signature = this.signature();
    if (signature === this.lastSignature) return;
    this.lastSignature = signature;

    const seed = window.worldSeed ?? '0';
    const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const worldUnitsPerGrid = (
      size / WORLD_GRID_CONTRACT.gridCellsPerChunk
    );
    const elevationScale = WORLD_GRID_CONTRACT.elevationScale;
    const sampleElevation = createTerrainElevationSampler(window);
    const chunkMap = new Map(
      window.chunks.map((chunk) => [
        `${chunk.x}:${chunk.y}`,
        chunk,
      ]),
    );

    const waterDirections = (
      chunk: TerrainChunk,
    ): Array<[number, number]> => (
      ([
        [1, 0],
        [-1, 0],
        [0, 1],
        [0, -1],
      ] as Array<[number, number]>).filter(([dx, dy]) => {
        const neighbor = chunkMap.get(
          `${chunk.x + dx}:${chunk.y + dy}`,
        );
        return neighbor
          ? (
              neighbor.waterKind === 'Spring'
              || neighbor.waterKind === 'Stream'
              || neighbor.waterKind === 'River'
              || neighbor.waterKind === 'Lake'
              || neighbor.waterKind === 'Wetland'
            )
          : false;
      })
    );

    const distanceToSegment = (
      px: number,
      pz: number,
      ax: number,
      az: number,
      bx: number,
      bz: number,
    ): number => {
      const abx = bx - ax;
      const abz = bz - az;
      const denom = abx * abx + abz * abz;
      const t = denom <= 0.000001
        ? 0
        : Math.max(
            0,
            Math.min(
              1,
              ((px - ax) * abx + (pz - az) * abz) / denom,
            ),
          );
      const cx = ax + abx * t;
      const cz = az + abz * t;
      return Math.hypot(px - cx, pz - cz);
    };

    const riverShoreFactor = (
      chunk: TerrainChunk,
      localX01: number,
      localY01: number,
    ): { shore: number; water: boolean } => {
      if (
        chunk.waterKind !== 'River'
        && chunk.waterKind !== 'Stream'
        && chunk.waterKind !== 'Spring'
      ) {
        return { shore: 0, water: false };
      }

      const px = (localX01 - 0.5) * size;
      const pz = (localY01 - 0.5) * size;
      const width = chunk.waterKind === 'River'
        ? size * 0.22
        : chunk.waterKind === 'Stream'
          ? size * 0.11
          : size * 0.08;
      const halfWidth = width * 0.5;
      const halfLength = size * 0.52;
      const directions = waterDirections(chunk);
      const activeDirections = directions.length > 0
        ? directions
        : ([[1, 0], [-1, 0]] as Array<[number, number]>);

      let distance = Math.hypot(px, pz);
      for (const [dx, dy] of activeDirections) {
        distance = Math.min(
          distance,
          distanceToSegment(
            px,
            pz,
            0,
            0,
            dx * halfLength,
            dy * halfLength,
          ),
        );
      }
      const water = distance <= halfWidth;
      const shoreWidth = 0.72;
      const shore = water
        ? 0
        : clamp01(1 - (distance - halfWidth) / shoreWidth);
      return { shore, water };
    };

    const coastalEdgeFactor = (
      chunk: TerrainChunk,
      localX01: number,
      localY01: number,
    ): number => {
      if (
        chunk.waterKind === 'Ocean'
        || chunk.waterKind === 'Coast'
        || chunk.waterKind === 'Lake'
        || chunk.waterKind === 'Wetland'
      ) {
        return 0;
      }

      let factor = 0;
      const neighbors: Array<[number, number, number]> = [
        [1, 0, localX01],
        [-1, 0, 1 - localX01],
        [0, 1, localY01],
        [0, -1, 1 - localY01],
      ];
      for (const [dx, dy, edgeDistance01] of neighbors) {
        const neighbor = chunkMap.get(
          `${chunk.x + dx}:${chunk.y + dy}`,
        );
        if (!neighbor) continue;
        if (
          neighbor.waterKind !== 'Ocean'
          && neighbor.waterKind !== 'Coast'
          && neighbor.waterKind !== 'Lake'
        ) {
          continue;
        }
        factor = Math.max(
          factor,
          clamp01(1 - edgeDistance01 / 0.18),
        );
      }
      return factor;
    };

    const facilityPoints = (this.presentation?.facilities ?? [])
      .filter((facility) => facility.state !== 'Ruined')
      .map((facility) => ({
        ...gridToWorld(facility.gridX, facility.gridY, window),
        radius: facility.state === 'UnderConstruction' ? 3.1 : 2.35,
      }));

    const depletedResources = (this.presentation?.resources ?? [])
      .map((resource) => {
        const max = Math.max(
          1,
          Number(resource.maxQuantity) || Number(resource.quantity) || 1,
        );
        return {
          ...gridToWorld(resource.gridX, resource.gridY, window),
          material: resource.material,
          depletion: 1 - clamp01((Number(resource.quantity) || 0) / max),
        };
      })
      .filter((resource) => resource.depletion > 0.18);

    const residuePoints = (this.presentation?.residues ?? [])
      .map((residue) => ({
        ...gridToWorld(residue.gridX, residue.gridY, window),
        intensity: clamp01(residue.intensity),
        radius: Math.max(
          worldUnitsPerGrid * 0.7,
          (Number(residue.radiusTiles) || 1) * worldUnitsPerGrid,
        ),
      }))
      .filter((residue) => residue.intensity > 0.04);

    const contaminationKeepAt = (
      x: number,
      z: number,
    ): number => {
      let keep = 1;
      for (const residue of residuePoints) {
        const distance = Math.hypot(
          x - residue.x,
          z - residue.z,
        );
        if (distance >= residue.radius) continue;
        const falloff = 1 - distance / residue.radius;
        keep = Math.min(
          keep,
          1 - residue.intensity * falloff * 0.76,
        );
      }
      return clamp01(keep);
    };

    const humanClearanceAt = (x: number, z: number): number => {
      let keep = 1;
      for (const facility of facilityPoints) {
        const distance = Math.hypot(x - facility.x, z - facility.z);
        if (distance >= facility.radius) continue;
        keep = Math.min(
          keep,
          clamp01(distance / Math.max(0.01, facility.radius)),
        );
      }
      return keep;
    };

    const resourceKeepAt = (
      x: number,
      z: number,
      kinds: string[],
    ): number => {
      let keep = 1;
      for (const resource of depletedResources) {
        if (!kinds.includes(resource.material)) continue;
        const distance = Math.hypot(x - resource.x, z - resource.z);
        const radius = 2.8;
        if (distance >= radius) continue;
        const falloff = 1 - distance / radius;
        keep = Math.min(
          keep,
          1 - resource.depletion * falloff,
        );
      }
      return clamp01(keep);
    };

    let grassCount = 0;
    let pebbleCount = 0;
    let rockCount = 0;
    let boulderCount = 0;
    let deadwoodCount = 0;
    let litterCount = 0;

    const placeInstance = (
      mesh: THREE.InstancedMesh,
      index: number,
      x: number,
      y: number,
      z: number,
      sx: number,
      sy: number,
      sz: number,
      yaw: number,
      tiltX = 0,
      tiltZ = 0,
      colorBase?: THREE.Color,
      colorVariation = 0,
      colorTint?: THREE.Color,
      colorTintStrength = 0,
    ): void => {
      this.position.set(x, y, z);
      this.euler.set(tiltX, yaw, tiltZ);
      this.rotation.setFromEuler(this.euler);
      this.scale.set(sx, sy, sz);
      this.matrix.compose(this.position, this.rotation, this.scale);
      mesh.setMatrixAt(index, this.matrix);
      if (colorBase && colorVariation > 0) {
        const tone = Math.sin(
          x * 1.37 + z * 1.91 + index * 0.73,
        );
        this.instanceColor.copy(colorBase);
        this.instanceColor.offsetHSL(
          tone * colorVariation * 0.18,
          tone * colorVariation * 0.12,
          tone * colorVariation,
        );
        if (colorTint && colorTintStrength > 0) {
          this.instanceColor.lerp(
            colorTint,
            clamp01(colorTintStrength),
          );
        }
        mesh.setColorAt(index, this.instanceColor);
      }
    };

    const slopeAt = (
      chunk: TerrainChunk,
      localX: number,
      localY: number,
    ): number => {
      const e = 0.07;
      const x0 = Math.max(0, localX - e);
      const x1 = Math.min(1, localX + e);
      const y0 = Math.max(0, localY - e);
      const y1 = Math.min(1, localY + e);
      const hx = (
        sampleElevation(chunk.x, chunk.y, x1, localY)
        - sampleElevation(chunk.x, chunk.y, x0, localY)
      ) * elevationScale;
      const hz = (
        sampleElevation(chunk.x, chunk.y, localX, y1)
        - sampleElevation(chunk.x, chunk.y, localX, y0)
      ) * elevationScale;
      const horizontal = Math.max(0.01, e * 2 * size);
      const gradient = Math.hypot(hx / horizontal, hz / horizontal);
      return clamp01(Math.atan(gradient) / (Math.PI * 0.5));
    };

    for (const chunk of window.chunks) {
      const grassCoverage = clamp01(chunk.grassCoverage01);
      const shrubCoverage = clamp01(chunk.shrubCoverage01);
      const forestCoverage = clamp01(chunk.forestCoverage01);
      const rockCoverage = clamp01(chunk.rockCoverage01);
      const wetlandCoverage = clamp01(chunk.wetlandCoverage01);
      const moisture = clamp01(chunk.moisture01);
      const elevation = clamp01(chunk.elevation01);
      const grassTint = new THREE.Color(
        moisture > 0.62 || wetlandCoverage > 0.42
          ? 0x4f6c3c
          : moisture < 0.28
            ? 0x817449
            : 0x667748,
      );
      const stoneTint = new THREE.Color(
        elevation > 0.66
          ? 0x777b78
          : moisture > 0.62
            ? 0x686d66
            : 0x746f66,
      );
      const shoreTint = new THREE.Color(0x8a8477);
      const deadwoodTint = new THREE.Color(
        moisture > 0.58 ? 0x504735 : 0x66513b,
      );

      const chunkCenterX = (
        chunk.x - window.centerChunkX
      ) * size;
      const chunkCenterZ = (
        chunk.y - window.centerChunkY
      ) * size;

      const grassCandidates = Math.min(
        30,
        5 + Math.floor((grassCoverage + wetlandCoverage * 0.5) * 24),
      );
      for (
        let index = 0;
        index < grassCandidates && grassCount < MAX_GRASS;
        index += 1
      ) {
        const localX = hash01(seed, chunk.x, chunk.y, index, 11);
        const localY = hash01(seed, chunk.x, chunk.y, index, 12);
        const x = chunkCenterX + (localX - 0.5) * size * 0.94;
        const z = chunkCenterZ + (localY - 0.5) * size * 0.94;
        const slope = slopeAt(chunk, localX, localY);
        const riverEdge = riverShoreFactor(
          chunk,
          localX,
          localY,
        );
        const fullyWaterCovered = (
          chunk.waterKind === 'Ocean'
          || chunk.waterKind === 'Coast'
          || chunk.waterKind === 'Lake'
        );
        if (fullyWaterCovered || riverEdge.water) continue;
        const clusterSeed = hash01(
          seed,
          chunk.x,
          chunk.y,
          index,
          117,
        );
        const cluster = 0.5 + 0.5 * Math.sin(
          (chunk.x + localX) * 4.31
          + (chunk.y + localY) * 3.17
          + clusterSeed * Math.PI * 2,
        );
        const waterPenalty = (
          riverEdge.shore > 0
          || coastalEdgeFactor(chunk, localX, localY) > 0
        ) ? 0.42 : 1;
        const clearance = humanClearanceAt(x, z)
          * resourceKeepAt(x, z, ['Fiber', 'PlantFood'])
          * contaminationKeepAt(x, z);
        const probability = clamp01(
          (grassCoverage * 0.78
            + shrubCoverage * 0.12
            + wetlandCoverage * 0.18
            + moisture * 0.06)
          * (1 - slope * 0.72)
          * (0.45 + cluster * 0.78)
          * waterPenalty
          * clearance,
        );
        if (hash01(seed, chunk.x, chunk.y, index, 13) > probability) {
          continue;
        }

        const y = sampleElevation(
          chunk.x,
          chunk.y,
          localX,
          localY,
        ) * elevationScale + 0.012;
        const scaleBase = (
          0.55
          + hash01(seed, chunk.x, chunk.y, index, 14) * 0.85
        ) * (wetlandCoverage > 0.4 ? 1.18 : 1);
        placeInstance(
          this.grass,
          grassCount,
          x,
          y,
          z,
          scaleBase,
          scaleBase,
          scaleBase,
          hash01(seed, chunk.x, chunk.y, index, 15) * Math.PI * 2,
          0,
          (hash01(seed, chunk.x, chunk.y, index, 16) - 0.5) * 0.18,
          this.grassBase,
          0.055,
          grassTint,
          0.12 + Math.abs(moisture - 0.5) * 0.18,
        );
        grassCount += 1;
      }

      const pebbleCandidates = Math.min(
        18,
        2 + Math.floor(rockCoverage * 12)
          + (
            chunk.waterKind === 'Coast'
            || chunk.waterKind === 'River'
            || chunk.waterKind === 'Stream'
              ? 5
              : 0
          ),
      );
      for (
        let index = 0;
        index < pebbleCandidates && pebbleCount < MAX_PEBBLES;
        index += 1
      ) {
        const localX = hash01(seed, chunk.x, chunk.y, index, 31);
        const localY = hash01(seed, chunk.x, chunk.y, index, 32);
        const x = chunkCenterX + (localX - 0.5) * size * 0.96;
        const z = chunkCenterZ + (localY - 0.5) * size * 0.96;
        const slope = slopeAt(chunk, localX, localY);
        const riverEdge = riverShoreFactor(
          chunk,
          localX,
          localY,
        );
        if (
          riverEdge.water
          || chunk.waterKind === 'Ocean'
          || chunk.waterKind === 'Coast'
          || chunk.waterKind === 'Lake'
        ) {
          continue;
        }
        const coastalEdge = coastalEdgeFactor(
          chunk,
          localX,
          localY,
        );
        const shorelineBoost = Math.max(
          riverEdge.shore * 0.62,
          coastalEdge * 0.72,
        );
        const cluster = hash01(
          seed,
          Math.floor((chunk.x + localX) * 3),
          Math.floor((chunk.y + localY) * 3),
          0,
          33,
        );
        const probability = clamp01(
          rockCoverage * 0.66
          + shorelineBoost
          + slope * 0.2,
        ) * (0.38 + cluster * 0.82) * humanClearanceAt(x, z);
        if (hash01(seed, chunk.x, chunk.y, index, 34) > probability) {
          continue;
        }

        const y = sampleElevation(
          chunk.x,
          chunk.y,
          localX,
          localY,
        ) * elevationScale + 0.018;
        const scaleBase = 0.65
          + hash01(seed, chunk.x, chunk.y, index, 35) * 0.9;
        const shoreFlattening = Math.max(
          riverEdge.shore,
          coastalEdge,
        );
        placeInstance(
          this.pebbles,
          pebbleCount,
          x,
          y,
          z,
          scaleBase * (1.2 + shoreFlattening * 0.35),
          scaleBase * (0.52 - shoreFlattening * 0.18),
          scaleBase * (0.96 + shoreFlattening * 0.22),
          hash01(seed, chunk.x, chunk.y, index, 36) * Math.PI * 2,
          0,
          0,
          this.pebbleBase,
          0.07,
          shorelineBoost > 0.12 ? shoreTint : stoneTint,
          shorelineBoost > 0.12 ? 0.22 : 0.1,
        );
        pebbleCount += 1;
      }

      const rockCandidates = Math.min(
        9,
        1 + Math.floor(rockCoverage * 8),
      );
      for (
        let index = 0;
        index < rockCandidates && rockCount < MAX_SMALL_ROCKS;
        index += 1
      ) {
        const localX = hash01(seed, chunk.x, chunk.y, index, 51);
        const localY = hash01(seed, chunk.x, chunk.y, index, 52);
        const x = chunkCenterX + (localX - 0.5) * size * 0.9;
        const z = chunkCenterZ + (localY - 0.5) * size * 0.9;
        const slope = slopeAt(chunk, localX, localY);
        const cluster = hash01(
          seed,
          Math.floor((chunk.x + localX) * 2),
          Math.floor((chunk.y + localY) * 2),
          0,
          53,
        );
        const depletionKeep = resourceKeepAt(
          x,
          z,
          ['Stone', 'Clay'],
        );
        const probability = clamp01(
          (rockCoverage * 0.72 + slope * 0.28)
          * (0.42 + cluster * 0.72)
          * humanClearanceAt(x, z)
          * (0.72 + contaminationKeepAt(x, z) * 0.28)
          * (0.55 + depletionKeep * 0.45),
        );
        if (hash01(seed, chunk.x, chunk.y, index, 54) > probability) {
          continue;
        }

        const y = sampleElevation(
          chunk.x,
          chunk.y,
          localX,
          localY,
        ) * elevationScale - 0.03;
        const scaleBase = 0.6
          + hash01(seed, chunk.x, chunk.y, index, 55) * 1.25;
        placeInstance(
          this.rocks,
          rockCount,
          x,
          y,
          z,
          scaleBase * 1.2,
          scaleBase * 0.65,
          scaleBase,
          hash01(seed, chunk.x, chunk.y, index, 56) * Math.PI * 2,
          (hash01(seed, chunk.x, chunk.y, index, 57) - 0.5) * 0.24,
          (hash01(seed, chunk.x, chunk.y, index, 58) - 0.5) * 0.24,
          this.rockBase,
          0.065,
          stoneTint,
          0.14,
        );
        rockCount += 1;
      }

      const boulderCandidates = Math.min(
        5,
        Math.floor(
          rockCoverage * 3.5
          + (Number(chunk.elevation01) > 0.64 ? 1.5 : 0),
        ),
      );
      for (
        let index = 0;
        index < boulderCandidates && boulderCount < MAX_BOULDERS;
        index += 1
      ) {
        const localX = hash01(seed, chunk.x, chunk.y, index, 61);
        const localY = hash01(seed, chunk.x, chunk.y, index, 62);
        const riverEdge = riverShoreFactor(chunk, localX, localY);
        if (
          riverEdge.water
          || chunk.waterKind === 'Ocean'
          || chunk.waterKind === 'Coast'
          || chunk.waterKind === 'Lake'
        ) {
          continue;
        }
        const x = chunkCenterX + (localX - 0.5) * size * 0.88;
        const z = chunkCenterZ + (localY - 0.5) * size * 0.88;
        const slope = slopeAt(chunk, localX, localY);
        const cluster = hash01(
          seed,
          Math.floor((chunk.x + localX) * 1.6),
          Math.floor((chunk.y + localY) * 1.6),
          0,
          63,
        );
        const depletionKeep = resourceKeepAt(
          x,
          z,
          ['Stone', 'Clay'],
        );
        const probability = clamp01(
          rockCoverage * 0.52
          + slope * 0.34
          + Math.max(0, Number(chunk.elevation01) - 0.58) * 0.42,
        )
          * (0.35 + cluster * 0.72)
          * humanClearanceAt(x, z)
          * (0.58 + depletionKeep * 0.42);
        if (hash01(seed, chunk.x, chunk.y, index, 64) > probability) {
          continue;
        }

        const y = sampleElevation(
          chunk.x,
          chunk.y,
          localX,
          localY,
        ) * elevationScale - 0.18;
        const scaleBase = 0.58
          + hash01(seed, chunk.x, chunk.y, index, 65) * 1.05;
        placeInstance(
          this.boulders,
          boulderCount,
          x,
          y,
          z,
          scaleBase * 1.25,
          scaleBase * (0.72 + slope * 0.18),
          scaleBase,
          hash01(seed, chunk.x, chunk.y, index, 66) * Math.PI * 2,
          (hash01(seed, chunk.x, chunk.y, index, 67) - 0.5) * 0.28,
          (hash01(seed, chunk.x, chunk.y, index, 68) - 0.5) * 0.28,
          this.boulderBase,
          0.055,
          stoneTint,
          0.12,
        );
        boulderCount += 1;
      }

      const deadwoodCandidates = Math.min(
        7,
        Math.floor(forestCoverage * 6),
      );
      for (
        let index = 0;
        index < deadwoodCandidates && deadwoodCount < MAX_DEADWOOD;
        index += 1
      ) {
        const localX = hash01(seed, chunk.x, chunk.y, index, 71);
        const localY = hash01(seed, chunk.x, chunk.y, index, 72);
        const x = chunkCenterX + (localX - 0.5) * size * 0.9;
        const z = chunkCenterZ + (localY - 0.5) * size * 0.9;
        const slope = slopeAt(chunk, localX, localY);
        const cluster = hash01(
          seed,
          Math.floor((chunk.x + localX) * 2),
          Math.floor((chunk.y + localY) * 2),
          0,
          73,
        );
        const woodKeep = resourceKeepAt(x, z, ['Wood']);
        const depletionBoost = 1 - woodKeep;
        const probability = clamp01(
          forestCoverage * 0.28
          + depletionBoost * 0.55,
        ) * (0.45 + cluster * 0.68)
          * (1 - slope * 0.55)
          * humanClearanceAt(x, z)
          * (0.72 + contaminationKeepAt(x, z) * 0.28);
        if (hash01(seed, chunk.x, chunk.y, index, 74) > probability) {
          continue;
        }

        const y = sampleElevation(
          chunk.x,
          chunk.y,
          localX,
          localY,
        ) * elevationScale + 0.055;
        const lengthScale = 0.72
          + hash01(seed, chunk.x, chunk.y, index, 75) * 1.05;
        placeInstance(
          this.deadwood,
          deadwoodCount,
          x,
          y,
          z,
          1,
          lengthScale,
          1,
          hash01(seed, chunk.x, chunk.y, index, 76) * Math.PI * 2,
          0,
          Math.PI * 0.5,
          this.deadwoodBase,
          0.045,
          deadwoodTint,
          0.14,
        );
        deadwoodCount += 1;
      }

      const litterCandidates = Math.min(
        14,
        Math.floor(
          forestCoverage * 10
          + shrubCoverage * 2
          + (moisture > 0.35 ? 1 : 0),
        ),
      );
      for (
        let index = 0;
        index < litterCandidates && litterCount < MAX_LITTER;
        index += 1
      ) {
        const localX = hash01(seed, chunk.x, chunk.y, index, 81);
        const localY = hash01(seed, chunk.x, chunk.y, index, 82);
        const riverEdge = riverShoreFactor(chunk, localX, localY);
        if (
          riverEdge.water
          || chunk.waterKind === 'Ocean'
          || chunk.waterKind === 'Coast'
          || chunk.waterKind === 'Lake'
          || wetlandCoverage > 0.68
        ) {
          continue;
        }

        const x = chunkCenterX + (localX - 0.5) * size * 0.9;
        const z = chunkCenterZ + (localY - 0.5) * size * 0.9;
        const slope = slopeAt(chunk, localX, localY);
        const cluster = hash01(
          seed,
          Math.floor((chunk.x + localX) * 3),
          Math.floor((chunk.y + localY) * 3),
          0,
          83,
        );
        const keep = humanClearanceAt(x, z)
          * contaminationKeepAt(x, z)
          * resourceKeepAt(x, z, ['Wood']);
        const probability = clamp01(
          forestCoverage * 0.62
          + shrubCoverage * 0.08,
        )
          * (0.42 + cluster * 0.7)
          * (1 - slope * 0.7)
          * keep;
        if (
          hash01(seed, chunk.x, chunk.y, index, 84)
          > probability
        ) {
          continue;
        }

        const y = sampleElevation(
          chunk.x,
          chunk.y,
          localX,
          localY,
        ) * elevationScale + 0.014;
        const scaleBase = 0.72
          + hash01(seed, chunk.x, chunk.y, index, 85) * 1.0;
        const litterTint = new THREE.Color(
          moisture > 0.58 ? 0x514936 : 0x6d593e,
        );
        placeInstance(
          this.litter,
          litterCount,
          x,
          y,
          z,
          scaleBase * 1.15,
          scaleBase,
          scaleBase,
          hash01(seed, chunk.x, chunk.y, index, 86) * Math.PI * 2,
          0,
          0,
          this.litterBase,
          0.06,
          litterTint,
          0.16,
        );
        litterCount += 1;
      }
    }

    this.grass.count = grassCount;
    this.pebbles.count = pebbleCount;
    this.rocks.count = rockCount;
    this.boulders.count = boulderCount;
    this.deadwood.count = deadwoodCount;
    this.litter.count = litterCount;

    for (const mesh of [
      this.grass,
      this.pebbles,
      this.rocks,
      this.boulders,
      this.deadwood,
      this.litter,
    ]) {
      mesh.instanceMatrix.needsUpdate = true;
      if (mesh.instanceColor) {
        mesh.instanceColor.needsUpdate = true;
      }
      if (mesh.count > 0) {
        mesh.computeBoundingSphere();
      }
    }

    this.applyLod();
  }
}
