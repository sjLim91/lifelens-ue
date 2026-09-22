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
  private readonly instanceColor = new THREE.Color();
  private readonly up = new THREE.Vector3(0, 1, 0);
  private resources: WorldResourceNode[] = [];
  private facilities: WorldFacility[] = [];
  private lastResourceSignature = '';
  private readonly windUniforms: WindUniformSet[] = [];
  private windTime = 0;
  private windIntensity = 0;
  private cameraZoom = 1.25;

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
    this.applyLod();
  }

  setCameraZoom(zoom: number): void {
    this.cameraZoom = Math.max(0.1, Number(zoom) || 1);
    this.applyLod();
  }

  private applyLod(): void {
    const zoom = this.cameraZoom;
    this.trunks.visible = zoom >= 0.5;
    this.lowerCrowns.visible = zoom >= 0.5;
    this.middleCrowns.visible = zoom >= 0.5;
    this.upperCrowns.visible = zoom >= 0.5;
    this.shrubs.visible = zoom >= 0.82;
    this.rocks.visible = zoom >= 0.72;
    this.stumps.visible = zoom >= 0.92;
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
    const chunkMap = new Map(
      window.chunks.map((chunk) => [
        `${chunk.x}:${chunk.y}`,
        chunk,
      ]),
    );
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const halfChunk = chunkWorldSize * 0.5;
    let treeCountTotal = 0;
    let shrubCountTotal = 0;
    let rockCountTotal = 0;
    let stumpCountTotal = 0;
    const gridCellsPerChunk = WORLD_GRID_CONTRACT.gridCellsPerChunk;
    const worldUnitsPerGrid = chunkWorldSize / gridCellsPerChunk;

    const toWorldPoint = (
      gridX: number,
      gridY: number,
    ): { x: number; z: number } => {
      const chunkX = Math.floor(gridX / gridCellsPerChunk);
      const chunkY = Math.floor(gridY / gridCellsPerChunk);
      const localX = gridX - chunkX * gridCellsPerChunk;
      const localY = gridY - chunkY * gridCellsPerChunk;
      return {
        x: (
          chunkX - window.centerChunkX
          + localX / gridCellsPerChunk
          - 0.5
        ) * chunkWorldSize,
        z: (
          chunkY - window.centerChunkY
          + localY / gridCellsPerChunk
          - 0.5
        ) * chunkWorldSize,
      };
    };

    const facilityPoints = this.facilities.map((facility) => ({
      ...toWorldPoint(facility.gridX, facility.gridY),
      kind: facility.kind,
    }));
    const resourcePointsByMaterial = new Map<
      string,
      Array<{
        x: number;
        z: number;
        ratio: number;
      }>
    >();
    for (const resource of this.resources) {
      const maxQuantity = Math.max(
        1,
        Number(resource.maxQuantity) || Number(resource.quantity) || 1,
      );
      const ratio = Math.max(
        0,
        Math.min(1, (Number(resource.quantity) || 0) / maxQuantity),
      );
      const point = toWorldPoint(resource.gridX, resource.gridY);
      const points = resourcePointsByMaterial.get(resource.material) ?? [];
      points.push({ ...point, ratio });
      resourcePointsByMaterial.set(resource.material, points);
    }

    const facilityReadability = (
      worldX: number,
      worldZ: number,
    ): number => {
      let factor = 1;
      for (const facility of facilityPoints) {
        const distance = Math.hypot(
          worldX - facility.x,
          worldZ - facility.z,
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
      const resources = resourcePointsByMaterial.get(material) ?? [];
      for (const resource of resources) {
        const distance = Math.hypot(
          worldX - resource.x,
          worldZ - resource.z,
        );
        const radius = Math.max(
          worldUnitsPerGrid * 5,
          chunkWorldSize * 0.28,
        );
        if (distance >= radius) continue;
        const falloff = 1 - distance / radius;
        const localRetention = (
          1 - (1 - resource.ratio) * falloff
        );
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
      const moisture = Math.max(
        0,
        Math.min(1, Number(chunk.moisture01) || 0),
      );
      const elevation = Math.max(
        0,
        Math.min(1, Number(chunk.elevation01) || 0),
      );
      const biome = chunk.biome ?? 'Plains';
      const neighborForestValues = (
        ([
          [1, 0],
          [-1, 0],
          [0, 1],
          [0, -1],
        ] as Array<[number, number]>)
          .map(([dx, dy]) => (
            chunkMap.get(`${chunk.x + dx}:${chunk.y + dy}`)
          ))
          .filter((neighbor) => neighbor !== undefined)
          .map((neighbor) => Math.max(
            0,
            Math.min(
              1,
              Number(neighbor?.forestCoverage01) || 0,
            ),
          ))
      );
      const neighborForest = neighborForestValues.length > 0
        ? neighborForestValues.reduce(
            (sum, value) => sum + value,
            0,
          ) / neighborForestValues.length
        : forest;
      const forestEdge = Math.max(
        0,
        Math.min(
          1,
          Math.abs(forest - neighborForest) * 1.65
          + (
            forest > 0.32 && neighborForest < 0.24
              ? 0.24
              : 0
          ),
        ),
      );
      const forestInterior = Math.max(
        0,
        Math.min(
          1,
          forest * 1.08 - forestEdge * 0.38,
        ),
      );

      const treeCount = forest < 0.1
        ? 0
        : Math.min(
            12,
            1 + Math.floor(
              forest * 10.4 + forestInterior * 1.6,
            ),
          );
      const shrubCount = Math.min(
        18,
        Math.floor(
          shrub * 10
          + grass * 5
          + forest * 3
          + forestEdge * 4.2,
        ),
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
        ) * (0.32 + forestEdge * 0.42);
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
            this.instanceColor.setHex(0xffffff);
            this.instanceColor.offsetHSL(
              0,
              0,
              (hash01(seed, chunk.x, chunk.y, 1021 + index * 7) - 0.5) * 0.09,
            );
            this.stumps.setColorAt(stumpCountTotal, this.instanceColor);
            stumpCountTotal += 1;
          }
          continue;
        }

        const regrowthScale = 0.52 + treeRetention * 0.48;
        const formRoll = hash01(
          seed,
          chunk.x,
          chunk.y,
          1030 + index * 11,
        );
        const coniferBias = (
          biome === 'ColdSteppe'
          || biome === 'RockyHighland'
          || elevation > 0.68
        )
          ? 0.58
          : biome === 'TemperateForest'
            ? 0.2
            : 0.08;
        const saplingBias = Math.min(
          0.28,
          0.08
          + (1 - treeRetention) * 0.24
          + (forest < 0.35 ? 0.06 : 0),
        );
        const treeForm = formRoll < coniferBias
          ? 'Conifer'
          : formRoll > 1 - saplingBias
            ? 'Sapling'
            : 'Broadleaf';

        const treeTint = treeForm === 'Conifer'
          ? 0xd5e2d6
          : treeForm === 'Sapling'
            ? 0xf1fff0
            : 0xffffff;
        const toneJitter = (
          hash01(seed, chunk.x, chunk.y, 1031 + index * 11) - 0.5
        ) * 0.08;

        const formHeight = treeForm === 'Conifer'
          ? 1.16
          : treeForm === 'Sapling'
            ? 0.68
            : 1;
        const formWidth = treeForm === 'Conifer'
          ? 0.72
          : treeForm === 'Sapling'
            ? 0.76
            : 1;
        const moistureScale = 0.9 + moisture * 0.18;
        const edgeHeightScale = 0.94
          + forestInterior * 0.08
          - forestEdge * 0.04;
        const visualTreeScale =
          treeScale
          * regrowthScale
          * moistureScale
          * edgeHeightScale;
        const trunkHeight =
          3.05
          * visualTreeScale
          * formHeight;

        this.position.set(
          worldX,
          groundY + trunkHeight * 0.5,
          worldZ,
        );
        this.scale.set(
          visualTreeScale
            * formWidth
            * (0.9 + widthScale * 0.1),
          visualTreeScale * formHeight,
          visualTreeScale
            * formWidth
            * (0.9 + widthScale * 0.1),
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.trunks.setMatrixAt(treeCountTotal, this.matrix);
        this.instanceColor.setHex(0xffffff);
        this.instanceColor.offsetHSL(0, 0, toneJitter * 0.45);
        this.trunks.setColorAt(treeCountTotal, this.instanceColor);

        const crownBaseY = groundY
          + trunkHeight
          - 0.2 * visualTreeScale;

        const broadleaf = [
          {
            y: 1.12,
            x: 1,
            z: -0.45,
            sx: 1.72,
            sy: 1.28,
            sz: 1.56,
          },
          {
            y: 2.3,
            x: -0.5,
            z: 1,
            sx: 1.46,
            sy: 1.22,
            sz: 1.4,
          },
          {
            y: 3.38,
            x: 0.3,
            z: 0.22,
            sx: 1.04,
            sy: 1.04,
            sz: 1.02,
          },
        ];
        const conifer = [
          {
            y: 0.92,
            x: 0.28,
            z: -0.16,
            sx: 1.34,
            sy: 1.46,
            sz: 1.3,
          },
          {
            y: 2.12,
            x: -0.18,
            z: 0.22,
            sx: 1.02,
            sy: 1.38,
            sz: 1,
          },
          {
            y: 3.28,
            x: 0.08,
            z: 0.08,
            sx: 0.64,
            sy: 1.24,
            sz: 0.64,
          },
        ];
        const sapling = [
          {
            y: 0.92,
            x: 0.45,
            z: -0.25,
            sx: 1.34,
            sy: 1.12,
            sz: 1.2,
          },
          {
            y: 1.82,
            x: -0.28,
            z: 0.45,
            sx: 1.06,
            sy: 1.04,
            sz: 1,
          },
          {
            y: 2.66,
            x: 0.18,
            z: 0.12,
            sx: 0.72,
            sy: 0.9,
            sz: 0.7,
          },
        ];
        const silhouette = treeForm === 'Conifer'
          ? conifer
          : treeForm === 'Sapling'
            ? sapling
            : broadleaf;
        const crownMeshes = [
          this.lowerCrowns,
          this.middleCrowns,
          this.upperCrowns,
        ];

        for (let layerIndex = 0; layerIndex < crownMeshes.length; layerIndex += 1) {
          const shape = silhouette[layerIndex];
          const layerAsymmetry = treeForm === 'Conifer'
            ? asymmetry * 0.35
            : asymmetry;
          this.position.set(
            worldX + shape.x * layerAsymmetry * visualTreeScale,
            crownBaseY + shape.y * visualTreeScale * formHeight,
            worldZ + shape.z * layerAsymmetry * visualTreeScale,
          );
          this.scale.set(
            shape.sx
              * visualTreeScale
              * widthScale
              * canopyFactor
              * formWidth
              * (1 + forestEdge * 0.1),
            shape.sy
              * visualTreeScale
              * (0.62 + canopyFactor * 0.38),
            shape.sz
              * visualTreeScale
              * widthScale
              * canopyFactor
              * formWidth
              * (1 + forestEdge * 0.1),
          );
          this.matrix.compose(this.position, this.rotation, this.scale);
          crownMeshes[layerIndex].setMatrixAt(
            treeCountTotal,
            this.matrix,
          );
          this.instanceColor.setHex(treeTint);
          this.instanceColor.offsetHSL(
            toneJitter * 0.12,
            toneJitter * 0.18,
            toneJitter + layerIndex * 0.008,
          );
          crownMeshes[layerIndex].setColorAt(
            treeCountTotal,
            this.instanceColor,
          );
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
        this.instanceColor.setHex(0xffffff);
        const shrubTone = (
          hash01(seed, chunk.x, chunk.y, 3005 + index * 5) - 0.5
        ) * 0.11;
        this.instanceColor.offsetHSL(
          shrubTone * 0.12,
          shrubTone * 0.2,
          shrubTone,
        );
        this.shrubs.setColorAt(shrubCountTotal, this.instanceColor);
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
        this.instanceColor.setHex(0xffffff);
        const rockTone = (
          hash01(seed, chunk.x, chunk.y, 5004 + index * 5) - 0.5
        ) * 0.1;
        this.instanceColor.offsetHSL(
          rockTone * 0.05,
          rockTone * 0.08,
          rockTone,
        );
        this.rocks.setColorAt(rockCountTotal, this.instanceColor);
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
      if (mesh.instanceColor) {
        mesh.instanceColor.needsUpdate = true;
      }
      if (mesh.count > 0) {
        mesh.computeBoundingSphere();
      }
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
