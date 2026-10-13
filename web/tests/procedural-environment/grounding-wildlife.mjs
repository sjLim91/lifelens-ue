import assert from 'node:assert/strict';
import * as THREE from 'three';
import { source } from './compile.mjs';
const { ResidentGroundSurface } = await source('render/resident-grounding.ts');
const { createTerrainGeometryBuilder } = await source('render/terrain-geometry.ts');
const { WildlifeLayer } = await source('render/wildlife-layer.ts');
const { createWildlifeGeometry } = await source('render/procedural-wildlife.ts');
const { WORLD_GRID_CONTRACT: grid } = await source('runtime/lifelens-contract.ts');
const samples = Array.from({length: 289}, (_, i) => {
  const x = i % 17, y = Math.floor(i / 17);
  return .3 + .07 * Math.sin(x * .8) * Math.sin(y * .65);
});
const chunk = {x: -2, y: 3, elevation01: .3, elevationSamples: samples,
  waterKind: 'None', waterAvailability: 0};
const terrain = {available: true, worldSeed: 'wildlife', centerChunkX: -2,
  centerChunkY: 3, chunks: [chunk]};
const ground = new ResidentGroundSurface();
const ray = new THREE.Raycaster();
let assertions = 0;
for (const detailed of [false, true]) {
  const window = {...terrain, chunks: [{...chunk, elevationSamples: detailed ? samples : undefined}]};
  ground.setTerrain(window, -2, 3);
  const geometry = createTerrainGeometryBuilder(window)(window.chunks[0]);
  const material = new THREE.MeshBasicMaterial({side: THREE.DoubleSide});
  const mesh = new THREE.Mesh(geometry, material); mesh.updateMatrixWorld();
  // Peaks, valleys, both triangle halves, corners, and negative-world chunks.
  for (let x = -3.9; x < 4; x += .31) for (let z = -3.9; z < 4; z += .37) {
    ray.set(new THREE.Vector3(x, 100, z), new THREE.Vector3(0, -1, 0));
    const hit = ray.intersectObject(mesh)[0]; assert(hit);
    assert(Math.abs(ground.height(x, z) - hit.point.y) < 2e-6); assertions++;
  }
  const before = ground.height(.7, -.9);
  ground.setTerrain(window, 5, -4);
  assert(Math.abs(ground.height(.7 - 7 * grid.worldUnitsPerChunk, -.9 + 7 * grid.worldUnitsPerChunk) - before) < 1e-8);
  assert.equal(ground.height(NaN, 0), null); assert.equal(ground.height(1e9, 1e9), null);
  geometry.dispose(); material.dispose();
}
console.log('PASS grounding matches', assertions, 'actual rendered triangle intersections, with rebasing');
for (const species of ['Hare', 'Deer', 'Wolf', 'FreshwaterFish']) {
  const a = createWildlifeGeometry(species), b = createWildlifeGeometry(species);
  assert.deepEqual(a.getAttribute('position').array, b.getAttribute('position').array);
  assert([...a.getAttribute('position').array, ...a.getAttribute('normal').array].every(Number.isFinite));
  assert(a.boundingBox.min.y >= -1e-6); assert(a.getAttribute('color'));
  a.dispose(); b.dispose();
}
console.log('PASS four original wildlife meshes are finite, deterministic and grounded');
const animals = ['Hare', 'Deer', 'Wolf'].map((species, i) => ({id: String(i), species, sex: 'Female',
  gridX: -2 * grid.gridCellsPerChunk + 16 + i, gridY: 3 * grid.gridCellsPerChunk + 16, ageDays: 900}));
const wildlife = {available: true, geographicallyConditioned: false, regionalWeatherApplied: false, animals};
const layer = new WildlifeLayer(); const snapshot = {...terrain, wildlife};
const original = JSON.stringify(snapshot); layer.setTerrain(snapshot);
assert.equal(layer.instanceCount, 3); assert.equal(JSON.stringify(snapshot), original);
const first = layer.group.children.map(mesh => [...mesh.instanceMatrix.array]);
layer.setTerrain(snapshot); assert.deepEqual(layer.group.children.map(mesh => [...mesh.instanceMatrix.array]), first);
layer.setTerrain({...terrain, wildlife: {...wildlife, animals: [animals[0], animals[0], {...animals[1], gridX: NaN}]}});
assert.equal(layer.instanceCount, 1);
layer.setTerrain({...terrain, wildlife: {...wildlife, animals: []}}); assert.equal(layer.instanceCount, 0);
layer.setTerrain(snapshot); assert.equal(layer.instanceCount, 3);
layer.setTerrain(terrain); assert.equal(layer.instanceCount, 0);
layer.setTerrain({...snapshot, available: false}); assert.equal(layer.instanceCount, 0);
let disposed = 0; for (const mesh of layer.group.children) mesh.geometry.addEventListener('dispose', () => disposed++);
layer.dispose(); assert.equal(disposed, 4); assert.equal(layer.group.children.length, 0);
console.log('PASS Core-only observation, deduplication, removal, legacy runtime, reload, disposal');
