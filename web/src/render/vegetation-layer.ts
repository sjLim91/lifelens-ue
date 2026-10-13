import * as THREE from 'three';
import type { DynamicEnvironment, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import {
  facilityPresentationFootprints,
  outsideFacilityFootprints,
} from './facility-layer';
import { createTerrainElevationSampler } from './terrain-geometry';
import { terrainDressingSignature } from './terrain-dressing-signature';
import { createVisibleWaterFootprintTester } from './water-geometry';
import { ProceduralTreeAsset, PROCEDURAL_TREE_VARIANTS, treeVariantForEnvironment } from './procedural-tree-layer';
import { assetRandom } from './procedural-asset-kit';
import {
  TREE_FOLIAGE_PALETTE,
  VEGETATION_PRESENTATION_CONTRACT,
  useMobileVegetationProfile,
} from './vegetation-profile';

import { NaturalResourceProjection } from './natural-resource-projection';

const MAX_TREES = VEGETATION_PRESENTATION_CONTRACT.maxTrees;
const UP = new THREE.Vector3(0, 1, 0);

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

export class VegetationLayer {
  readonly group = new THREE.Group();

  private readonly mobileProfile = useMobileVegetationProfile();
  private readonly treeVariantMatrices = PROCEDURAL_TREE_VARIANTS.map(
    () => new Float32Array(MAX_TREES * 16),
  );
  private readonly treeVariantFoliageTints = PROCEDURAL_TREE_VARIANTS.map(
    () => new Float32Array(MAX_TREES * 3),
  );
  private readonly actualTrees: ProceduralTreeAsset[];
  instanceCount = 0;
  readonly instanceMatrices = new Float32Array(MAX_TREES * 16);
  private readonly matrix = new THREE.Matrix4();
  private readonly rotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();
  private readonly position = new THREE.Vector3();
  private readonly treeColor = new THREE.Color();
  private terrainSignature = '';

  constructor() {
    this.actualTrees = PROCEDURAL_TREE_VARIANTS.map(variant =>
      new ProceduralTreeAsset(variant.species, variant.seed, MAX_TREES, this.mobileProfile));
    for (const asset of this.actualTrees) this.group.add(asset.group);
  }

  setEnvironment(environment: DynamicEnvironment | null, originX = 0, originZ = 0): void {
    for (const asset of this.actualTrees) asset.weather.setEnvironment(environment, originX, originZ);
  }
  update(camera: THREE.Camera): void { for (const asset of this.actualTrees) asset.update(camera); }

  setTerrain(window: TerrainWindow, resources = new NaturalResourceProjection({ available: false }, window)): void {
    const nextSignature = terrainDressingSignature(window) + resources.treeSignature;
    if (nextSignature === this.terrainSignature) return;
    this.terrainSignature = nextSignature;

    const seed = window.worldSeed ?? '0';
    const sampleElevation = createTerrainElevationSampler(window);
    const profile = VEGETATION_PRESENTATION_CONTRACT;
    const maxTreesPerChunk = this.mobileProfile
      ? profile.mobileMaxTreesPerChunk
      : profile.desktopMaxTreesPerChunk;
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const placementSpan =
      chunkWorldSize * profile.placementSpanChunkRatio;
    const facilityFootprints = facilityPresentationFootprints(window);
    const isInsideVisibleWater =
      createVisibleWaterFootprintTester(window);

    let treeIndex = 0;
    const variantCounts = PROCEDURAL_TREE_VARIANTS.map(() => 0);

    for (const chunk of window.chunks) {
      if (treeIndex >= MAX_TREES) break;
      if (
        chunk.waterKind === 'Ocean'
        || chunk.waterKind === 'Coast'
      ) {
        continue;
      }

      const forest = Math.max(
        0,
        Math.min(1, Number(chunk.forestCoverage01) || 0),
      );
      const treesInChunk = forest < profile.minForestCoverage01
        ? 0
        : Math.min(
          maxTreesPerChunk,
          1 + Math.floor(forest * maxTreesPerChunk),
        );

      const baseline = Array.from({ length: treesInChunk }, (_, index) => ({
        x: (hash01(seed, chunk.x, chunk.y, index * 2) - 0.5) * placementSpan,
        z: (hash01(seed, chunk.x, chunk.y, index * 2 + 1) - 0.5) * placementSpan,
        scale: 1,
      }));
      const candidates = resources.candidates('trees', chunk.x, chunk.y, seed, baseline, maxTreesPerChunk);
      for (let localTreeIndex = 0; localTreeIndex < candidates.length && treeIndex < MAX_TREES; localTreeIndex++) {
        const candidate = candidates[localTreeIndex];
        const offsetX = candidate.x, offsetZ = candidate.z;
        // A resource slot keeps its identity when preceding slots disappear.
        const random = assetRandom(candidate.assetKey ?? `${seed}:${chunk.x}:${chunk.y}:${offsetX}:${offsetZ}`);
        const shapeSeed = random();
        const treeScale = candidate.scale * (
          0.9 + shapeSeed * 0.55);
        const widthScale =
          0.88 + random() * 0.26;
        const yaw =
          random()
          * Math.PI * 2;
        const worldX =
          (chunk.x - window.centerChunkX) * chunkWorldSize + offsetX;
        const worldZ =
          (chunk.y - window.centerChunkY) * chunkWorldSize + offsetZ;
        if (isInsideVisibleWater(worldX, worldZ)) {
          continue;
        }
        if (!outsideFacilityFootprints(
          worldX,
          worldZ,
          facilityFootprints,
          0.55,
        )) {
          continue;
        }
        const sampleChunkX = window.centerChunkX + Math.floor(worldX / chunkWorldSize + 0.5);
        const sampleChunkY = window.centerChunkY + Math.floor(worldZ / chunkWorldSize + 0.5);
        const groundY = sampleElevation(sampleChunkX, sampleChunkY,
          worldX / chunkWorldSize + window.centerChunkX - sampleChunkX + 0.5,
          worldZ / chunkWorldSize + window.centerChunkY - sampleChunkY + 0.5,
        ) * WORLD_GRID_CONTRACT.elevationScale;

        const fullTreeHeight = profile.treeHeightWorldUnits * treeScale;
        this.rotation.setFromAxisAngle(UP, yaw);
        this.position.set(worldX, groundY, worldZ);
        this.scale.set(
          fullTreeHeight * widthScale,
          fullTreeHeight,
          fullTreeHeight * widthScale,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);

        const variantIndex = treeVariantForEnvironment(chunk, random());
        const paletteLength = PROCEDURAL_TREE_VARIANTS[variantIndex].species === 'pine'
          ? 3 : TREE_FOLIAGE_PALETTE.length;
        const foliageColorIndex = Math.min(
          paletteLength - 1,
          Math.floor(
            random()
            * paletteLength,
          ),
        );
        this.treeColor.setHex(
          TREE_FOLIAGE_PALETTE[foliageColorIndex],
        );
        this.treeColor.offsetHSL(
          (
            random()
            - 0.5
          ) * 0.025,
          (
            random()
            - 0.5
          ) * 0.08,
          (
            random()
            - 0.5
          ) * 0.06,
        );

        const variantInstanceIndex = variantCounts[variantIndex];
        this.matrix.toArray(
          this.treeVariantMatrices[variantIndex],
          variantInstanceIndex * 16,
        );
        const tintOffset = variantInstanceIndex * 3;
        this.treeVariantFoliageTints[variantIndex][tintOffset] =
          this.treeColor.r;
        this.treeVariantFoliageTints[variantIndex][tintOffset + 1] =
          this.treeColor.g;
        this.treeVariantFoliageTints[variantIndex][tintOffset + 2] =
          this.treeColor.b;
        variantCounts[variantIndex] += 1;

        this.matrix.toArray(this.instanceMatrices, treeIndex * 16);
        treeIndex += 1;
      }
    }

    for (let index = 0; index < this.actualTrees.length; index += 1) {
      this.actualTrees[index].setInstances(
        this.treeVariantMatrices[index],
        variantCounts[index],
        this.treeVariantFoliageTints[index],
      );
    }

    this.instanceCount = treeIndex;
  }

  dispose(): void { for (const asset of this.actualTrees) asset.dispose(); this.group.clear(); }
}
