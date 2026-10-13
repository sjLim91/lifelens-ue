import * as THREE from 'three';
import type { TerrainWindow, WildlifeSpecies } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT as grid } from '../runtime/lifelens-contract';
import { createWildlifeGeometry } from './procedural-wildlife';
import { ResidentGroundSurface } from './resident-grounding';
import { buildFlowWaterSurfaceGeometry, createVisibleWaterFootprintTester } from './water-geometry';
import { facilityPresentationFootprints, outsideFacilityFootprints } from './facility-layer';
import { terrainDressingSignature } from './terrain-dressing-signature';

const SPECIES: WildlifeSpecies[] = ['Hare', 'Deer', 'Wolf', 'FreshwaterFish'];
const LIMIT = 128;
/** Render only actual Core animals. No browser births, wandering or population
 * lottery; missing/old runtime data clears the view. Four shared mesh batches. */
export class WildlifeLayer {
  readonly group = new THREE.Group();
  private readonly material = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: .92 });
  private readonly meshes = new Map<WildlifeSpecies, THREE.InstancedMesh>();
  private readonly ground = new ResidentGroundSurface();
  private readonly transform = new THREE.Object3D();
  private terrainSignature = '';
  private water: THREE.Mesh | null = null;
  private readonly waterMaterial = new THREE.MeshBasicMaterial({ side: THREE.DoubleSide });
  private readonly ray = new THREE.Raycaster();
  private readonly rayOrigin = new THREE.Vector3();
  private readonly down = new THREE.Vector3(0, -1, 0);
  instanceCount = 0;

  constructor() {
    for (const species of SPECIES) {
      const mesh = new THREE.InstancedMesh(createWildlifeGeometry(species), this.material, LIMIT);
      mesh.count = 0;
      mesh.castShadow = species !== 'FreshwaterFish';
      mesh.receiveShadow = true;
      this.meshes.set(species, mesh);
      this.group.add(mesh);
    }
  }

  setTerrain(terrain: TerrainWindow): void {
    const signature = terrainDressingSignature(terrain);
    if (signature !== this.terrainSignature) {
      this.terrainSignature = signature;
      this.ground.setTerrain(terrain, terrain.centerChunkX, terrain.centerChunkY);
      this.water?.geometry.dispose();
      this.water = new THREE.Mesh(buildFlowWaterSurfaceGeometry(terrain), this.waterMaterial);
      this.water.updateMatrixWorld();
    }
    const counts = new Map(SPECIES.map(species => [species, 0]));
    this.instanceCount = 0;
    const insideWater = createVisibleWaterFootprintTester(terrain);
    const footprints = facilityPresentationFootprints(terrain);
    const animals = terrain.available && terrain.wildlife?.available === true
      && Array.isArray(terrain.wildlife.animals) ? terrain.wildlife.animals : [];
    // Prefer animals near the observer, preserving their authoritative position.
    const distance = (a: typeof animals[number]): number =>
      (a.gridX / grid.gridCellsPerChunk - terrain.centerChunkX - .5) ** 2
      + (a.gridY / grid.gridCellsPerChunk - terrain.centerChunkY - .5) ** 2;
    const seen = new Set<string>();
    for (const animal of animals.slice().sort((a, b) => distance(a) - distance(b) || a.id.localeCompare(b.id))) {
      if (this.instanceCount >= LIMIT) break;
      const mesh = this.meshes.get(animal.species);
      if (!mesh || !Number.isFinite(animal.gridX) || !Number.isFinite(animal.gridY)
        || !Number.isFinite(animal.ageDays) || animal.ageDays < 0 || !animal.id || seen.has(animal.id)) continue;
      seen.add(animal.id);
      const x = (animal.gridX / grid.gridCellsPerChunk - terrain.centerChunkX - .5) * grid.worldUnitsPerChunk;
      const z = (animal.gridY / grid.gridCellsPerChunk - terrain.centerChunkY - .5) * grid.worldUnitsPerChunk;
      const height = this.ground.height(x, z);
      if (height === null) continue;
      const fish = animal.species === 'FreshwaterFish';
      if (fish !== insideWater(x, z) || (!fish && !outsideFacilityFootprints(x, z, footprints, .35))) continue;
      let y = height;
      if (fish) {
        if (!this.water) continue;
        this.rayOrigin.set(x, 1000, z);
        this.ray.set(this.rayOrigin, this.down);
        const hit = this.ray.intersectObject(this.water)[0];
        if (!hit || hit.point.y - height < .2) continue;
        y = hit.point.y - .18;
      } else {
        // A conservative sole footprint keeps all four feet above hills even
        // without a quadruped IK rig. It never moves the authoritative X/Z.
        const width = animal.species === 'Hare' ? .14 : .2;
        const length = animal.species === 'Hare' ? .25 : animal.species === 'Deer' ? .5 : .4;
        for (const dx of [-width, width]) for (const dz of [-length, length])
          y = Math.max(y, this.ground.height(x + dx, z + dz) ?? height);
      }
      const maturity = animal.species === 'Deer' ? 420 : animal.species === 'Wolf' ? 600 : animal.species === 'Hare' ? 35 : 75;
      const scale = .45 + .55 * Math.min(1, animal.ageDays / maturity);
      this.transform.position.set(x, y, z);
      this.transform.rotation.set(0, 0, 0);
      this.transform.scale.setScalar(scale);
      this.transform.updateMatrix();
      const index = counts.get(animal.species)!;
      mesh.setMatrixAt(index, this.transform.matrix);
      counts.set(animal.species, index + 1);
      this.instanceCount++;
    }
    for (const [species, mesh] of this.meshes) {
      mesh.count = counts.get(species)!;
      mesh.instanceMatrix.needsUpdate = true;
      mesh.computeBoundingSphere();
    }
  }

  dispose(): void {
    for (const mesh of this.meshes.values()) { mesh.geometry.dispose(); mesh.dispose(); }
    this.meshes.clear();
    this.water?.geometry.dispose();
    this.waterMaterial.dispose();
    this.material.dispose();
    this.group.clear();
  }
}
