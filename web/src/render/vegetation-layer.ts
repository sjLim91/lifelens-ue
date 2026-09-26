import * as THREE from 'three';
import type { TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';
import { InstancedTreeAsset } from './tree-asset-layer';
import {
  TREE_ASSET_VARIANTS,
  TREE_BARK_PALETTE,
  TREE_CROWN_LOBE_LAYOUT,
  TREE_FOLIAGE_PALETTE,
  VEGETATION_PRESENTATION_CONTRACT,
  useMobileVegetationProfile,
} from './vegetation-profile';

const MAX_TREES = VEGETATION_PRESENTATION_CONTRACT.maxTrees;
const MAX_BRANCHES =
  MAX_TREES * VEGETATION_PRESENTATION_CONTRACT.branchCountPerTree;
const MAX_CROWN_LOBES =
  MAX_TREES * VEGETATION_PRESENTATION_CONTRACT.crownLobeCountPerTree;
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
  private readonly treeVariantMatrices = TREE_ASSET_VARIANTS.map(
    () => new Float32Array(MAX_TREES * 16),
  );
  private readonly crownGeometry = new THREE.IcosahedronGeometry(1, 0);
  private readonly trunkGeometry = new THREE.CylinderGeometry(
    0.72,
    1,
    1,
    8,
  );
  private readonly branchGeometry = new THREE.CylinderGeometry(
    0.5,
    1,
    1,
    6,
  );
  private readonly crownMaterial = new THREE.MeshLambertMaterial({
    color: 0xffffff,
  });
  private readonly barkMaterial = new THREE.MeshLambertMaterial({
    color: 0xffffff,
  });
  private readonly crowns = new THREE.InstancedMesh(
    this.crownGeometry,
    this.crownMaterial,
    MAX_CROWN_LOBES,
  );
  private readonly trunks = new THREE.InstancedMesh(
    this.trunkGeometry,
    this.barkMaterial,
    MAX_TREES,
  );
  private readonly branches = new THREE.InstancedMesh(
    this.branchGeometry,
    this.barkMaterial,
    MAX_BRANCHES,
  );
  private readonly actualTrees: InstancedTreeAsset[];
  private readonly matrix = new THREE.Matrix4();
  private readonly rotation = new THREE.Quaternion();
  private readonly branchRotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();
  private readonly position = new THREE.Vector3();
  private readonly branchDirection = new THREE.Vector3();
  private readonly treeColor = new THREE.Color();

  constructor() {
    this.actualTrees = TREE_ASSET_VARIANTS.map((variant) => (
      new InstancedTreeAsset({
        maxInstances: MAX_TREES,
        url: variant.modelUrl,
        onReady: () => this.refreshFallbackVisibility(),
      })
    ));

    for (const mesh of [this.crowns, this.trunks, this.branches]) {
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.castShadow = false;
      mesh.receiveShadow = false;
      mesh.frustumCulled = true;
    }

    this.group.add(this.trunks);
    this.group.add(this.branches);
    this.group.add(this.crowns);
    for (const asset of this.actualTrees) {
      this.group.add(asset.group);
    }
  }

  setTerrain(window: TerrainWindow): void {
    const seed = window.worldSeed ?? '0';
    const sampleElevation = createTerrainElevationSampler(window);
    const profile = VEGETATION_PRESENTATION_CONTRACT;
    const maxTreesPerChunk = this.mobileProfile
      ? profile.mobileMaxTreesPerChunk
      : profile.desktopMaxTreesPerChunk;
    const chunkWorldSize = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    const halfChunk = chunkWorldSize * 0.5;
    const placementSpan =
      chunkWorldSize * profile.placementSpanChunkRatio;

    let treeIndex = 0;
    let branchIndex = 0;
    let crownIndex = 0;
    const variantCounts = TREE_ASSET_VARIANTS.map(() => 0);

    for (const chunk of window.chunks) {
      if (treeIndex >= MAX_TREES) break;
      if (
        chunk.waterKind === 'Ocean'
        || chunk.waterKind === 'Coast'
        || chunk.waterKind === 'Lake'
        || chunk.waterKind === 'River'
        || chunk.waterKind === 'Stream'
        || chunk.waterKind === 'Spring'
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

      for (
        let localTreeIndex = 0;
        localTreeIndex < treesInChunk && treeIndex < MAX_TREES;
        localTreeIndex += 1
      ) {
        const offsetX =
          (hash01(seed, chunk.x, chunk.y, localTreeIndex * 2) - 0.5)
          * placementSpan;
        const offsetZ =
          (hash01(seed, chunk.x, chunk.y, localTreeIndex * 2 + 1) - 0.5)
          * placementSpan;
        const treeScale =
          0.9 + hash01(seed, chunk.x, chunk.y, localTreeIndex + 19) * 0.55;
        const widthScale =
          0.88 + hash01(seed, chunk.x, chunk.y, localTreeIndex + 31) * 0.26;
        const yaw =
          hash01(seed, chunk.x, chunk.y, localTreeIndex + 47)
          * Math.PI * 2;
        const worldX =
          (chunk.x - window.centerChunkX) * chunkWorldSize + offsetX;
        const worldZ =
          (chunk.y - window.centerChunkY) * chunkWorldSize + offsetZ;
        const localX01 = Math.max(
          0,
          Math.min(1, (offsetX + halfChunk) / chunkWorldSize),
        );
        const localY01 = Math.max(
          0,
          Math.min(1, (offsetZ + halfChunk) / chunkWorldSize),
        );
        const groundY = sampleElevation(
          chunk.x,
          chunk.y,
          localX01,
          localY01,
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
        const variantIndex = Math.min(
          TREE_ASSET_VARIANTS.length - 1,
          Math.floor(
            hash01(seed, chunk.x, chunk.y, localTreeIndex + 503)
            * TREE_ASSET_VARIANTS.length,
          ),
        );
        const variantInstanceIndex = variantCounts[variantIndex];
        this.matrix.toArray(
          this.treeVariantMatrices[variantIndex],
          variantInstanceIndex * 16,
        );
        variantCounts[variantIndex] += 1;

        const trunkHeight = profile.trunkHeightWorldUnits * treeScale;
        const trunkRadius =
          profile.trunkRadiusWorldUnits * treeScale * widthScale;

        this.position.set(
          worldX,
          groundY + trunkHeight * 0.5,
          worldZ,
        );
        this.scale.set(
          trunkRadius,
          trunkHeight,
          trunkRadius,
        );
        this.matrix.compose(this.position, this.rotation, this.scale);
        this.trunks.setMatrixAt(treeIndex, this.matrix);

        const barkColorIndex = Math.min(
          TREE_BARK_PALETTE.length - 1,
          Math.floor(
            hash01(seed, chunk.x, chunk.y, localTreeIndex + 73)
            * TREE_BARK_PALETTE.length,
          ),
        );
        this.treeColor.setHex(TREE_BARK_PALETTE[barkColorIndex]);
        this.trunks.setColorAt(treeIndex, this.treeColor);

        for (
          let localBranchIndex = 0;
          localBranchIndex < profile.branchCountPerTree
            && branchIndex < MAX_BRANCHES;
          localBranchIndex += 1
        ) {
          const branchYaw =
            yaw
            + (localBranchIndex / profile.branchCountPerTree)
              * Math.PI * 2
            + (
              hash01(
                seed,
                chunk.x,
                chunk.y,
                localTreeIndex * 17 + localBranchIndex + 101,
              ) - 0.5
            ) * 0.55;
          const branchTilt =
            0.82
            + hash01(
              seed,
              chunk.x,
              chunk.y,
              localTreeIndex * 17 + localBranchIndex + 151,
            ) * 0.28;
          const branchLength =
            profile.branchLengthWorldUnits
            * treeScale
            * (
              0.82
              + hash01(
                seed,
                chunk.x,
                chunk.y,
                localTreeIndex * 17 + localBranchIndex + 211,
              ) * 0.36
            );
          const branchRadius =
            profile.branchRadiusWorldUnits * treeScale * widthScale;
          const branchStartHeight =
            trunkHeight * (0.48 + localBranchIndex * 0.075);

          this.branchDirection.set(
            Math.cos(branchYaw) * Math.sin(branchTilt),
            Math.cos(branchTilt),
            Math.sin(branchYaw) * Math.sin(branchTilt),
          ).normalize();
          this.branchRotation.setFromUnitVectors(
            UP,
            this.branchDirection,
          );
          this.position.set(
            worldX + this.branchDirection.x * branchLength * 0.5,
            groundY
              + branchStartHeight
              + this.branchDirection.y * branchLength * 0.5,
            worldZ + this.branchDirection.z * branchLength * 0.5,
          );
          this.scale.set(
            branchRadius,
            branchLength,
            branchRadius,
          );
          this.matrix.compose(
            this.position,
            this.branchRotation,
            this.scale,
          );
          this.branches.setMatrixAt(branchIndex, this.matrix);
          this.branches.setColorAt(branchIndex, this.treeColor);
          branchIndex += 1;
        }

        const foliageColorIndex = Math.min(
          TREE_FOLIAGE_PALETTE.length - 1,
          Math.floor(
            hash01(seed, chunk.x, chunk.y, localTreeIndex + 281)
            * TREE_FOLIAGE_PALETTE.length,
          ),
        );
        const cosYaw = Math.cos(yaw);
        const sinYaw = Math.sin(yaw);
        const crownRadius =
          profile.crownRadiusWorldUnits * treeScale * widthScale;

        for (
          let lobeIndex = 0;
          lobeIndex < TREE_CROWN_LOBE_LAYOUT.length
            && crownIndex < MAX_CROWN_LOBES;
          lobeIndex += 1
        ) {
          const lobe = TREE_CROWN_LOBE_LAYOUT[lobeIndex];
          const localX = lobe.x * crownRadius;
          const localZ = lobe.z * crownRadius;
          const rotatedX = localX * cosYaw - localZ * sinYaw;
          const rotatedZ = localX * sinYaw + localZ * cosYaw;
          const lobeJitter =
            0.93
            + hash01(
              seed,
              chunk.x,
              chunk.y,
              localTreeIndex * 23 + lobeIndex + 337,
            ) * 0.14;
          const lobeRadius = crownRadius * lobe.radius * lobeJitter;

          this.position.set(
            worldX + rotatedX,
            groundY + fullTreeHeight * lobe.y,
            worldZ + rotatedZ,
          );
          this.scale.set(
            lobeRadius,
            lobeRadius * (0.88 + lobeIndex * 0.025),
            lobeRadius,
          );
          this.matrix.compose(this.position, this.rotation, this.scale);
          this.crowns.setMatrixAt(crownIndex, this.matrix);

          this.treeColor.setHex(
            TREE_FOLIAGE_PALETTE[foliageColorIndex],
          );
          this.treeColor.offsetHSL(
            0,
            0,
            (
              hash01(
                seed,
                chunk.x,
                chunk.y,
                localTreeIndex * 29 + lobeIndex + 401,
              ) - 0.5
            ) * 0.08,
          );
          this.crowns.setColorAt(crownIndex, this.treeColor);
          crownIndex += 1;
        }

        treeIndex += 1;
      }
    }

    for (let index = 0; index < this.actualTrees.length; index += 1) {
      this.actualTrees[index].setInstances(
        this.treeVariantMatrices[index],
        variantCounts[index],
      );
    }
    this.refreshFallbackVisibility();

    this.trunks.count = treeIndex;
    this.branches.count = branchIndex;
    this.crowns.count = crownIndex;

    this.trunks.instanceMatrix.needsUpdate = true;
    this.branches.instanceMatrix.needsUpdate = true;
    this.crowns.instanceMatrix.needsUpdate = true;

    if (this.trunks.instanceColor) {
      this.trunks.instanceColor.needsUpdate = true;
    }
    if (this.branches.instanceColor) {
      this.branches.instanceColor.needsUpdate = true;
    }
    if (this.crowns.instanceColor) {
      this.crowns.instanceColor.needsUpdate = true;
    }

    this.trunks.computeBoundingSphere();
    this.branches.computeBoundingSphere();
    this.crowns.computeBoundingSphere();
  }

  dispose(): void {
    for (const asset of this.actualTrees) {
      asset.dispose();
    }
    this.crownGeometry.dispose();
    this.trunkGeometry.dispose();
    this.branchGeometry.dispose();
    this.crownMaterial.dispose();
    this.barkMaterial.dispose();
  }

  private refreshFallbackVisibility(): void {
    this.setFallbackVisible(
      !this.actualTrees.every((asset) => asset.isReady),
    );
  }

  private setFallbackVisible(visible: boolean): void {
    this.trunks.visible = visible;
    this.branches.visible = visible;
    this.crowns.visible = visible;
  }
}
