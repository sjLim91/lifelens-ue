import * as THREE from 'three';
import type {
  DynamicEnvironment,
  TerrainWindow,
  WorldFacility,
  WorldPresentationSnapshot,
  WorldResourceNode,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';

const MAX_TREES = 4096;
const MAX_SHRUBS = 6144;
const MAX_ROCKS = 3072;
const MAX_STUMPS = 2048;

function hash01(seed: string, x: number, y: number, index: number): number {
  const input = `${seed}:${x}:${y}:${index}`;
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < input.length; i += 1) {
    hash ^= input.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

interface WindUniformSet {
  time: { value: number };
  intensity: { value: number };
}

export class VegetationLayer {
  readonly group = new THREE.Group();

  private readonly trunkGeometry = new THREE.CylinderGeometry(
    0.16,
    0.25,
    3.05,
    7,
  );
  private readonly lowerCrownGeometry = new THREE.IcosahedronGeometry(1, 1);
  private readonly middleCrownGeometry = new THREE.IcosahedronGeometry(1, 1);
  private readonly upperCrownGeometry = new THREE.IcosahedronGeometry(1, 1);
  private readonly shrubGeometry = new THREE.IcosahedronGeometry(0.52, 1);
  private readonly rockGeometry = new THREE.DodecahedronGeometry(0.48, 0);
  private readonly stumpGeometry = new THREE.CylinderGeometry(
    0.22,
    0.28,
    0.34,
    8,
  );

  private readonly trunkMaterial = new THREE.MeshStandardMaterial({
    color: 0x4b3827,
    roughness: 0.98,
    metalness: 0,
  });
  private readonly lowerCrownMaterial = new THREE.MeshStandardMaterial({
    color: 0x264b2a,
    roughness: 0.96,
    metalness: 0,
  });
  private readonly middleCrownMaterial = new THREE.MeshStandardMaterial({
    color: 0x2d5930,
    roughness: 0.95,
    metalness: 0,
  });
  private readonly upperCrownMaterial = new THREE.MeshStandardMaterial({
    color: 0x37683a,
    roughness: 0.94,
    metalness: 0,
  });
  private readonly shrubMaterial = new THREE.MeshStandardMaterial({
    color: 0x415f34,
    roughness: 0.98,
    metalness: 0,
  });
  private readonly rockMaterial = new THREE.MeshStandardMaterial({
    color: 0x6b6b63,
    roughness: 1,
    metalness: 0,
  });
  private readonly stumpMaterial = new THREE.MeshStandardMaterial({
    color: 0x66503a,
    roughness: 1,
    metalness: 0,
  });

  private readonly trunks = new THREE.InstancedMesh(
    this.trunkGeometry,
    this.trunkMaterial,
    MAX_TREES,
  );
  private readonly lowerCrowns = new THREE.InstancedMesh(
    this.lowerCrownGeometry,
    this.lowerCrownMaterial,
    MAX_TREES,
  );
  private readonly middleCrowns = new THREE.InstancedMesh(
    this.middleCrownGeometry,
    this.middleCrownMaterial,
    MAX_TREES,
  );
  private readonly upperCrowns = new THREE.InstancedMesh(
    this.upperCrownGeometry,
    this.upperCrownMaterial,
    MAX_TREES,
  );
  private readonly shrubs = new THREE.InstancedMesh(
    this.shrubGeometry,
    this.shrubMaterial,
    MAX_SHRUBS,
  );
  private readonly rocks = new THREE.InstancedMesh(
    this.rockGeometry,
    this.rockMaterial,
    MAX_ROCKS,
  );
  private readonly stumps = new THREE.InstancedMesh(
    this.stumpGeometry,
    this.stumpMaterial,
    MAX_STUMPS,
  );

  private readonly matrix = new THREE.Matrix4();
  private readonly rotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();
  private readonly position = new THREE.Vector3();
  private readonly up = new THREE.Vector3(0, 1, 0);
  private resources: WorldResourceNode[] = [];
  private facilities: WorldFacility[] = [];
  private lastResourceSignature = '';
  private readonly windUniforms: WindUniformSet[] = [];
  private windTime = 0;
  private windIntensity = 0;

  constructor() {
    this.enableWind(this.lowerCrownMaterial, 0.16);
    this.enableWind(this.middleCrownMaterial, 0.2);
    this.enableWind(this.upperCrownMaterial, 0.24);
    this.enableWind(this.shrubMaterial, 0.22);

    for (const mesh of [
      this.trunks,
      this.lowerCrowns,
      this.middleCrowns,
      this.upperCrowns,
      this.shrubs,
      this.rocks,
      this.stumps,
    ]) {
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.castShadow = false;
      mesh.receiveShadow = false;
      mesh.frustumCulled = true;
      this.group.add(mesh);
    }
  }

  setEnvironment(
    environment: DynamicEnvironment | null,
  ): void {
    this.windIntensity = Math.max(
      0,
      Math.min(
        1,
        Number(environment?.windIntensity01) || 0,
      ),
    );
    for (const uniforms of this.windUniforms) {
      uniforms.intensity.value = this.windIntensity;
    }

    const snow = environment?.precipitationType === 'Snow'
      ? Math.max(
          0,
          Math.min(
            1,
            Number(environment?.precipitationIntensity01) || 0,
          ),
        )
      : 0;
    const wetness = Math.max(
      0,
      Math.min(1, Number(environment?.surfaceWetness01) || 0),
    );

    const applySurface = (
      material: THREE.MeshStandardMaterial,
      snowAmount: number,
      wetDarken: number,
    ): void => {
      if (!material.userData.lifeLensBaseColor) {
        material.userData.lifeLensBaseColor = material.color.clone();
      }
      const baseColor = material.userData.lifeLensBaseColor;
      if (baseColor instanceof THREE.Color) {
        material.color.copy(baseColor);
        material.color.multiplyScalar(1 - wetness * wetDarken);
        if (snow > 0.02 && snowAmount > 0) {
          material.color.lerp(
            new THREE.Color(0xdde6df),
            Math.min(snowAmount, snow * snowAmount),
          );
        }
      }
      const baseRoughness = Number(
        material.userData.lifeLensBaseRoughness
          ?? material.roughness,
      );
      material.userData.lifeLensBaseRoughness = baseRoughness;
      material.roughness = Math.max(
        0.55,
        baseRoughness - wetness * 0.18,
      );
      material.needsUpdate = true;
    };

    applySurface(this.trunkMaterial, 0.05, 0.08);
    applySurface(this.lowerCrownMaterial, 0.1, 0.1);
    applySurface(this.middleCrownMaterial, 0.16, 0.1);
    applySurface(this.upperCrownMaterial, 0.24, 0.1);
    applySurface(this.shrubMaterial, 0.13, 0.1);
    applySurface(this.rockMaterial, 0.2, 0.09);
    applySurface(this.stumpMaterial, 0.08, 0.08);
  }

  update(deltaSeconds: number): void {
    this.windTime += Math.min(
      0.05,
      Math.max(0, deltaSeconds),
    );
    for (const uniforms of this.windUniforms) {
      uniforms.time.value = this.windTime;
      uniforms.intensity.value = this.windIntensity;
    }
  }

  private enableWind(
    material: THREE.MeshStandardMaterial,
    strength: number,
  ): void {
    material.onBeforeCompile = (shader) => {
      const uniforms: WindUniformSet = {
        time: { value: this.windTime },
        intensity: { value: this.windIntensity },
      };
      shader.uniforms.uLifeLensWindTime = uniforms.time;
      shader.uniforms.uLifeLensWindIntensity = uniforms.intensity;
      this.windUniforms.push(uniforms);

      shader.vertexShader = shader.vertexShader.replace(
        '#include <common>',
        `#include <common>
uniform float uLifeLensWindTime;
uniform float uLifeLensWindIntensity;`,
      );
      shader.vertexShader = shader.vertexShader.replace(
        '#include <begin_vertex>',
        `#include <begin_vertex>
#ifdef USE_INSTANCING
  float lifeLensWindPhase =
    uLifeLensWindTime * 1.85
    + instanceMatrix[3].x * 0.071
    + instanceMatrix[3].z * 0.093;
  float lifeLensWindHeight = clamp(position.y + 1.15, 0.0, 2.4);
  float lifeLensWindAmount =
    uLifeLensWindIntensity * ${strength.toFixed(3)};
  transformed.x +=
    sin(lifeLensWindPhase) * lifeLensWindAmount * lifeLensWindHeight;
  transformed.z +=
    cos(lifeLensWindPhase * 0.83)
    * lifeLensWindAmount
    * lifeLensWindHeight
    * 0.55;
#endif`,
      );
    };
    material.customProgramCacheKey = () => (
      `lifelens-wind-v2-${strength.toFixed(3)}`
    );
    material.needsUpdate = true;
  }

  setPresentation(
    snapshot: WorldPresentationSnapshot | null,
    window: TerrainWindow,
  ): void {
    const resources = snapshot?.resources ?? [];
    const facilities = (snapshot?.facilities ?? []).filter(
      (facility) => facility.state !== 'Ruined',
    );
    const signature = [
      ...resources.map((resource) => (
        `r:${resource.id}:${resource.material}:${resource.quantity}:${resource.maxQuantity ?? 0}:${resource.gridX}:${resource.gridY}`
      )),
      ...facilities.map((facility) => (
        `f:${facility.id}:${facility.state}:${facility.gridX}:${facility.gridY}`
      )),
    ].join('|');
    if (signature === this.lastResourceSignature) return;
    this.lastResourceSignature = signature;
    this.resources = resources;
    this.facilities = facilities;
    this.setTerrain(window);
  }

  setTerrain(window: TerrainWindow): void {
    const seed = window.worldSeed ?? '0';
    const sampleElevation = createTerrainElevationSampler(window);
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const halfChunk = chunkWorldSize * 0.5;
    let treeCountTotal = 0;
    let shrubCountTotal = 0;
    let rockCountTotal = 0;
    let stumpCountTotal = 0;
    const gridCellsPerChunk = WORLD_GRID_CONTRACT.gridCellsPerChunk;
    const worldUnitsPerGrid = chunkWorldSize / gridCellsPerChunk;

    const facilityReadability = (
      worldX: number,
      worldZ: number,
    ): number => {
      let factor = 1;
      for (const facility of this.facilities) {
        const facilityChunkX = Math.floor(
          facility.gridX / gridCellsPerChunk,
        );
        const facilityChunkY = Math.floor(
          facility.gridY / gridCellsPerChunk,
        );
        const facilityLocalX =
          facility.gridX - facilityChunkX * gridCellsPerChunk;
        const facilityLocalY =
          facility.gridY - facilityChunkY * gridCellsPerChunk;
        const facilityWorldX = (
          facilityChunkX - window.centerChunkX
          + facilityLocalX / gridCellsPerChunk
          - 0.5
        ) * chunkWorldSize;
        const facilityWorldZ = (
          facilityChunkY - window.centerChunkY
          + facilityLocalY / gridCellsPerChunk
          - 0.5
        ) * chunkWorldSize;
        const distance = Math.hypot(
          worldX - facilityWorldX,
          worldZ - facilityWorldZ,
        );
        const radius = facility.kind === 'Shelter' ? 3.4 : 2.6;
        if (distance >= radius) continue;
        const t = Math.max(0, Math.min(1, distance / radius));
        const localFactor = 0.46 + t * 0.54;
        factor = Math.min(factor, localFactor);
      }
      return factor;
    };

    const resourceInfluence = (
      material: string,
      worldX: number,
      worldZ: number,
    ): number => {
      let retention = 1;
      for (const resource of this.resources) {
        if (resource.material !== material) continue;
        const maxQuantity = Math.max(
          1,
          Number(resource.maxQuantity) || Number(resource.quantity) || 1,
        );
        const ratio = Math.max(
          0,
          Math.min(1, (Number(resource.quantity) || 0) / maxQuantity),
        );
        const nodeChunkX = Math.floor(resource.gridX / gridCellsPerChunk);
        const nodeChunkY = Math.floor(resource.gridY / gridCellsPerChunk);
        const nodeLocalX = resource.gridX - nodeChunkX * gridCellsPerChunk;
        const nodeLocalY = resource.gridY - nodeChunkY * gridCellsPerChunk;
        const nodeWorldX = (
          nodeChunkX - window.centerChunkX
          + nodeLocalX / gridCellsPerChunk
          - 0.5
        ) * chunkWorldSize;
        const nodeWorldZ = (
          nodeChunkY - window.centerChunkY
          + nodeLocalY / gridCellsPerChunk
          - 0.5
        ) * chunkWorldSize;
        const distance = Math.hypot(
          worldX - nodeWorldX,
          worldZ - nodeWorldZ,
        );
        const radius = Math.max(
          worldUnitsPerGrid * 5,
          chunkWorldSize * 0.28,
        );
        if (distance >= radius) continue;
        const falloff = 1 - distance / radius;
        const localRetention = 1 - (1 - ratio) * falloff;
        retention = Math.min(retention, localRetention);
      }
      return Math.max(0.03, Math.min(1, retention));
    };

    const groundAt = (
      chunkX: number,
      chunkY: number,
      offsetX: number,
      offsetZ: number,
    ): number => {
      const localX01 = Math.max(
        0,
        Math.min(1, (offsetX + halfChunk) / chunkWorldSize),
      );
      const localY01 = Math.max(
        0,
        Math.min(1, (offsetZ + halfChunk) / chunkWorldSize),
      );
      return sampleElevation(
        chunkX,
        chunkY,
        localX01,
        localY01,
      ) * WORLD_GRID_CONTRACT.elevationScale;
    };

    for (const chunk of window.chunks) {
      const forest = Math.max(
        0,
        Math.min(1, Number(chunk.forestCoverage01) || 0),
      );
      const shrub = Math.max(
        0,
        Math.min(1, Number(chunk.shrubCoverage01) || 0),
      );
      const grass = Math.max(
        0,
        Math.min(1, Number(chunk.grassCoverage01) || 0),
      );
      const rock = Math.max(
        0,
        Math.min(1, Number(chunk.rockCoverage01) || 0),
      );

      const treeCount = forest < 0.1
        ? 0
        : Math.min(12, 1 + Math.floor(forest * 11));
      const shrubCount = Math.min(
        18,
        Math.floor(shrub * 10 + grass * 5 + forest * 4),
      );
      const rockCount = Math.min(8, Math.floor(rock * 8));

      for (
        let index = 0;
        index < treeCount && treeCountTotal < MAX_TREES;
        index += 1
      ) {
        const offsetX = (
          hash01(seed, chunk.x, chunk.y, 1000 + index * 7) - 0.5
        ) * chunkWorldSize * 0.88;
        const offsetZ = (
          hash01(seed, chunk.x, chunk.y, 1001 + index * 7) - 0.5
        ) * chunkWorldSize * 0.88;
        const treeScale = 0.78
          + hash01(seed, chunk.x, chunk.y, 1002 + index * 7) * 0.74;
        const widthScale = 0.78
          + hash01(seed, chunk.x, chunk.y, 1003 + index * 7) * 0.48;
        const asymmetry = (
          hash01(seed, chunk.x, chunk.y, 1004 + index * 7) - 0.5
        ) * 0.45;
        const yaw = hash01(
          seed,
          chunk.x,
          chunk.y,
          1005 + index * 7,
        ) * Math.PI * 2;
        const worldX = (
          chunk.x - window.centerChunkX
        ) * chunkWorldSize + offsetX;
        const worldZ = (
          chunk.y - window.centerChunkY
        ) * chunkWorldSize + offsetZ;
        const treeRetention = resourceInfluence('Wood', worldX, worldZ);
        const canopyFactor = facilityReadability(worldX, worldZ);
        const treePresenceRoll = hash01(
          seed,
          chunk.x,
          chunk.y,
          1010 + index * 7,
        );
        const groundY = groundAt(
          chunk.x,
          chunk.y,
          offsetX,
          offsetZ,
        );

        this.rotation.setFromAxisAngle(this.up, yaw);

        if (treePresenceRoll > treeRetention) {
          if (stumpCountTotal < MAX_STUMPS && treeRetention < 0.82) {
            this.position.set(
              worldX,
              groundY + 0.17,
              worldZ,
            );
            const stumpScale = 0.78
              + hash01(seed, chunk.x, chunk.y, 1020 + index * 7) * 0.52;
            this.scale.set(stumpScale, 0.72, stumpScale);
            this.matrix.compose(this.position, this.rotation, this.scale);
            this.stumps.setMatrixAt(stumpCountTotal, this.matrix);
            stumpCountTotal += 1;
          }
          continue;
        }

        const regrowthScale = 0.52 + treeRetention * 0.48;
        const visualTreeScale = treeScale * regrowthScale;
        const trunkHeight = 3.05 * visualTreeScale;
        this.position.set(
          worldX,
          groundY + trunkHeight * 0.5,
          worldZ,
        );
        this.scale.set(
          visualTreeScale * (0.9 + widthScale * 0.1),
          visualTreeScale,
          visualTreeScale * (0.9 + widthScale * 0.1),
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.trunks.setMatrixAt(treeCountTotal, this.matrix);

        const crownBaseY = groundY + trunkHeight - 0.2 * visualTreeScale;
        const crownLayers: Array<{
          mesh: THREE.InstancedMesh;
          y: number;
          x: number;
          z: number;
          sx: number;
          sy: number;
          sz: number;
        }> = [
          {
            mesh: this.lowerCrowns,
            y: crownBaseY + 1.15 * visualTreeScale,
            x: asymmetry * visualTreeScale,
            z: -asymmetry * 0.45 * visualTreeScale,
            sx: 1.7 * visualTreeScale * widthScale * canopyFactor,
            sy: 1.28 * visualTreeScale * (0.62 + canopyFactor * 0.38),
            sz: 1.55 * visualTreeScale * widthScale * canopyFactor,
          },
          {
            mesh: this.middleCrowns,
            y: crownBaseY + 2.35 * visualTreeScale,
            x: -asymmetry * 0.5 * visualTreeScale,
            z: asymmetry * visualTreeScale,
            sx: 1.45 * visualTreeScale * widthScale * canopyFactor,
            sy: 1.22 * visualTreeScale * (0.62 + canopyFactor * 0.38),
            sz: 1.38 * visualTreeScale * widthScale * canopyFactor,
          },
          {
            mesh: this.upperCrowns,
            y: crownBaseY + 3.45 * visualTreeScale,
            x: asymmetry * 0.3 * visualTreeScale,
            z: asymmetry * 0.22 * visualTreeScale,
            sx: 1.05 * visualTreeScale * widthScale * canopyFactor,
            sy: 1.05 * visualTreeScale * (0.62 + canopyFactor * 0.38),
            sz: 1.02 * visualTreeScale * widthScale * canopyFactor,
          },
        ];
        for (const layer of crownLayers) {
          this.position.set(
            worldX + layer.x,
            layer.y,
            worldZ + layer.z,
          );
          this.scale.set(layer.sx, layer.sy, layer.sz);
          this.matrix.compose(this.position, this.rotation, this.scale);
          layer.mesh.setMatrixAt(treeCountTotal, this.matrix);
        }

        treeCountTotal += 1;
      }

      for (
        let index = 0;
        index < shrubCount && shrubCountTotal < MAX_SHRUBS;
        index += 1
      ) {
        const offsetX = (
          hash01(seed, chunk.x, chunk.y, 3000 + index * 5) - 0.5
        ) * chunkWorldSize * 0.92;
        const offsetZ = (
          hash01(seed, chunk.x, chunk.y, 3001 + index * 5) - 0.5
        ) * chunkWorldSize * 0.92;
        const scaleBase = 0.38
          + hash01(seed, chunk.x, chunk.y, 3002 + index * 5) * 0.75;
        const worldX = (
          chunk.x - window.centerChunkX
        ) * chunkWorldSize + offsetX;
        const worldZ = (
          chunk.y - window.centerChunkY
        ) * chunkWorldSize + offsetZ;
        const groundY = groundAt(
          chunk.x,
          chunk.y,
          offsetX,
          offsetZ,
        );
        const fiberRetention = resourceInfluence('Fiber', worldX, worldZ);
        const foodRetention = resourceInfluence('PlantFood', worldX, worldZ);
        const shrubRetention = Math.min(fiberRetention, foodRetention);
        const shrubPresenceRoll = hash01(
          seed,
          chunk.x,
          chunk.y,
          3004 + index * 5,
        );
        if (shrubPresenceRoll > shrubRetention) continue;
        const yaw = hash01(
          seed,
          chunk.x,
          chunk.y,
          3003 + index * 5,
        ) * Math.PI * 2;
        this.rotation.setFromAxisAngle(this.up, yaw);
        this.position.set(
          worldX,
          groundY + scaleBase * 0.42,
          worldZ,
        );
        const regrowthScale = 0.58 + shrubRetention * 0.42;
        const groundReadability = Math.max(
          0.58,
          facilityReadability(worldX, worldZ),
        );
        this.scale.set(
          scaleBase * 1.35 * regrowthScale * groundReadability,
          scaleBase * 0.85 * regrowthScale * groundReadability,
          scaleBase * regrowthScale * groundReadability,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.shrubs.setMatrixAt(shrubCountTotal, this.matrix);
        shrubCountTotal += 1;
      }

      for (
        let index = 0;
        index < rockCount && rockCountTotal < MAX_ROCKS;
        index += 1
      ) {
        const offsetX = (
          hash01(seed, chunk.x, chunk.y, 5000 + index * 5) - 0.5
        ) * chunkWorldSize * 0.9;
        const offsetZ = (
          hash01(seed, chunk.x, chunk.y, 5001 + index * 5) - 0.5
        ) * chunkWorldSize * 0.9;
        const scaleBase = 0.34
          + hash01(seed, chunk.x, chunk.y, 5002 + index * 5) * 0.78;
        const worldX = (
          chunk.x - window.centerChunkX
        ) * chunkWorldSize + offsetX;
        const worldZ = (
          chunk.y - window.centerChunkY
        ) * chunkWorldSize + offsetZ;
        const groundY = groundAt(
          chunk.x,
          chunk.y,
          offsetX,
          offsetZ,
        );
        const yaw = hash01(
          seed,
          chunk.x,
          chunk.y,
          5003 + index * 5,
        ) * Math.PI * 2;
        this.rotation.setFromAxisAngle(this.up, yaw);
        this.position.set(
          worldX,
          groundY + scaleBase * 0.32,
          worldZ,
        );
        this.scale.set(
          scaleBase * 1.25,
          scaleBase * 0.65,
          scaleBase,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.rocks.setMatrixAt(rockCountTotal, this.matrix);
        rockCountTotal += 1;
      }
    }

    this.trunks.count = treeCountTotal;
    this.lowerCrowns.count = treeCountTotal;
    this.middleCrowns.count = treeCountTotal;
    this.upperCrowns.count = treeCountTotal;
    this.shrubs.count = shrubCountTotal;
    this.rocks.count = rockCountTotal;
    this.stumps.count = stumpCountTotal;

    for (const mesh of [
      this.trunks,
      this.lowerCrowns,
      this.middleCrowns,
      this.upperCrowns,
      this.shrubs,
      this.rocks,
      this.stumps,
    ]) {
      mesh.instanceMatrix.needsUpdate = true;
    }
  }

  dispose(): void {
    this.trunkGeometry.dispose();
    this.lowerCrownGeometry.dispose();
    this.middleCrownGeometry.dispose();
    this.upperCrownGeometry.dispose();
    this.shrubGeometry.dispose();
    this.rockGeometry.dispose();
    this.stumpGeometry.dispose();
    this.trunkMaterial.dispose();
    this.lowerCrownMaterial.dispose();
    this.middleCrownMaterial.dispose();
    this.upperCrownMaterial.dispose();
    this.shrubMaterial.dispose();
    this.rockMaterial.dispose();
    this.stumpMaterial.dispose();
    this.windUniforms.length = 0;
  }
}
