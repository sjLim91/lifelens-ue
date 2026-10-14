import assert from 'node:assert/strict';
import * as THREE from 'three';
import { source } from './compile.mjs';
const { ResidentGroundSurface } = await source('render/resident-grounding.ts');
const { ObserverFoliageClarity, injectFoliageClarity } =
  await source('render/observer-foliage-clarity.ts');
const { PlantWeatherMaterial } = await source('render/procedural-tree-layer.ts');

{
  const clarity=new ObserverFoliageClarity();
  const camera=new THREE.PerspectiveCamera(42,1,.1,500);
  camera.position.set(0,6,11);
  camera.updateWorldMatrix(true,false);
  const target=new THREE.Vector3(0,.8,0);
  clarity.set(camera,target,false);
  assert(clarity.uniforms.llClarityStrength.value>0);
  assert.deepEqual(clarity.uniforms.llClarityFocus.value.toArray(),target.toArray());
  clarity.set(camera,target,true);
  assert.equal(clarity.uniforms.llClarityStrength.value,.92);
  clarity.set(camera,null,false);
  assert.equal(clarity.uniforms.llClarityStrength.value,0);
  camera.position.set(0,120,160);
  camera.updateWorldMatrix(true,false);
  clarity.set(camera,target,false);
  assert.equal(clarity.uniforms.llClarityStrength.value,0,
    'Zoomed-out globe must not contain transparent forest tunnels');
  const shader={uniforms:{},
    vertexShader:'#include <common>\n#include <begin_vertex>\n#include <project_vertex>',
    fragmentShader:'#include <common>\n#include <color_fragment>\n#include <dithering_fragment>'};
  const weather=new PlantWeatherMaterial(clarity);
  const material=new THREE.MeshLambertMaterial({vertexColors:true});
  weather.install(material);
  material.onBeforeCompile(shader,{});
  assert(shader.vertexShader.includes('llClarityVertex = instanceMatrix * llClarityVertex'));
  assert(shader.fragmentShader.includes('separation < llClarityRadius'));
  assert(shader.fragmentShader.includes('discard;'));
  assert(shader.fragmentShader.includes('llLeafDensity'));
  assert(shader.uniforms.llClarityFocus===clarity.uniforms.llClarityFocus);
  assert(shader.uniforms.llWind===weather.uniforms.llWind);
  material.dispose();
  console.log('PASS near-focus and selected-resident foliage clarity preserves wind, depth and global forest');
}

const { createTerrainGeometryBuilder } = await source('render/terrain-geometry.ts');
const { WildlifeLayer } = await source('render/wildlife-layer.ts');
const { createWildlifeGeometry } = await source('render/procedural-wildlife.ts');
const { LifeLensCoreBridge } = await source('runtime/core-bridge.ts');
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
const legacy = { available: true, centerChunkX: 0, centerChunkY: 0,
  radiusChunks: 1, chunks: [], humanTraces: { total: 1, entries: [] } };
const fallback = new LifeLensCoreBridge({ terrainWindowJson: () => JSON.stringify(legacy) });
assert.deepEqual(fallback.humanTracesWindow(0, 0, 1), {
  available: true, centerChunkX: 0, centerChunkY: 0, radiusChunks: 1,
  humanTraces: legacy.humanTraces,
});
const observedWildlife = { available: true, geographicallyConditioned: false,
  regionalWeatherApplied: false, animals: [] };
const current = new LifeLensCoreBridge({ terrainWindowJson: () => JSON.stringify({ ...legacy, wildlife: observedWildlife }) });
assert.deepEqual(current.humanTracesWindow(0, 0, 1).wildlife, observedWildlife);
const direct = new LifeLensCoreBridge({ humanTracesWindowJson: () => JSON.stringify({ ...legacy, wildlife: observedWildlife }) });
assert.deepEqual(direct.humanTracesWindow(0, 0, 1).wildlife, observedWildlife);
console.log('PASS legacy observer response shape and actual wildlife forwarding');
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
for (const species of ['Hare', 'Deer', 'Wolf', 'FreshwaterFish', 'Tiger']) {
  const a = createWildlifeGeometry(species), b = createWildlifeGeometry(species);
  assert.deepEqual(a.getAttribute('position').array, b.getAttribute('position').array);
  assert([...a.getAttribute('position').array, ...a.getAttribute('normal').array].every(Number.isFinite));
  assert(a.boundingBox.min.y >= -1e-6); assert(a.getAttribute('color'));
  a.dispose(); b.dispose();
}
// Q1 authored seamless body: near/mid/far use genuinely reduced geometry
// density and preserve pose, grounded sole, deterministic identity/pigment.
for(const species of ['Hare','Deer','Wolf','FreshwaterFish','Tiger']){
  const near=createWildlifeGeometry(species,'Female','near');
  const mid=createWildlifeGeometry(species,'Female','mid');
  const far=createWildlifeGeometry(species,'Female','far');
  assert.equal(near.userData.wildlifeSurfaceKind,'fused-implicit');
  assert.equal(near.userData.wildlifeAuthoredVersion,'Q1');
  assert(near.index.count>mid.index.count*1.3,'Near mesh insufficiently detailed '+species);
  assert(mid.index.count>far.index.count*1.3,'Far LOD insufficiently lighter '+species);
  assert(near.boundingBox.min.y>=-1e-6&&mid.boundingBox.min.y>=-1e-6);
  assert(near.getAttribute('wildlifeSpeciesId'));
  assert(near.getAttribute('normal').array.every(Number.isFinite));
  near.dispose();mid.dispose();far.dispose();
}
const doe=createWildlifeGeometry('Deer','Female','near'),
  buck=createWildlifeGeometry('Deer','Male','near');
assert(buck.boundingBox.max.y>doe.boundingBox.max.y+.15,
  'Male deer must grow true authored antlers');
doe.dispose();buck.dispose();
console.log('PASS 5 seamless fur surfaces, sex differences and 3 actual GPU LODs');
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
const moving={...terrain,wildlife:{...wildlife,animals:[
  {...animals[0],gridX:animals[0].gridX+3,behavior:'Flee'},...animals.slice(1)
]}};
layer.setTerrain(moving);
const m0=[...layer.group.children[0].instanceMatrix.array];
layer.update(.016);
const m1=[...layer.group.children[0].instanceMatrix.array];
assert.notDeepEqual(m0,m1);
for(let i=0;i<100;i++)layer.update(.1);
layer.setTerrain(terrain); assert.equal(layer.instanceCount, 0);
layer.setTerrain({...snapshot, available: false}); assert.equal(layer.instanceCount, 0);
let disposed = 0; for (const mesh of layer.group.children) mesh.geometry.addEventListener('dispose', () => disposed++);
layer.dispose(); assert.equal(disposed, 18); assert.equal(layer.group.children.length, 0);
console.log('PASS Core-only observation, deduplication, removal, legacy runtime, reload, disposal');

// Use the actual rendered freshwater surface and body bounds: adult fish must
// not protrude above water, and shallow water must not bury them in the bed.
const { buildFlowWaterSurfaceGeometry } = await source('render/water-geometry.ts');
const fishLayer = new WildlifeLayer();
for (const ageDays of [0, 900]) for (const bed of [.2, .3]) {
  const lake = {...terrain, chunks: [{...chunk, waterKind: 'Lake', salinity: 'Fresh',
    waterAvailability: 1, elevationSamples: Array(289).fill(bed)}],
    wildlife: {...wildlife, animals: [{...animals[0], species: 'FreshwaterFish', ageDays}]}};
  fishLayer.setTerrain(lake);
  if (bed === .3) { assert.equal(fishLayer.instanceCount, 0); continue; }
  assert.equal(fishLayer.instanceCount, 1);
  const fish = fishLayer.group.children[3], matrix = new THREE.Matrix4();
  fish.getMatrixAt(0, matrix);
  const bounds = fish.geometry.boundingBox.clone().applyMatrix4(matrix);
  const waterGeometry = buildFlowWaterSurfaceGeometry(lake);
  const waterMaterial = new THREE.MeshBasicMaterial({side: THREE.DoubleSide});
  const water = new THREE.Mesh(waterGeometry, waterMaterial); water.updateMatrixWorld();
  ray.set(new THREE.Vector3(0, 100, 0), new THREE.Vector3(0, -1, 0));
  const surface = ray.intersectObject(water)[0]; assert(surface);
  ground.setTerrain(lake, lake.centerChunkX, lake.centerChunkY);
  assert(bounds.min.y > ground.height(0, 0));
  assert(bounds.max.y < surface.point.y);
  waterGeometry.dispose(); waterMaterial.dispose();
}
fishLayer.dispose();
console.log('PASS juvenile/adult fish fit below actual water and above bed; shallow fish hidden');
