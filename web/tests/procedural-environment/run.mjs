import assert from 'node:assert/strict';
import { performance } from 'node:perf_hooks';
import { writeFileSync } from 'node:fs';
import * as THREE from 'three';
import { source } from './compile.mjs';
const { generateTree, TREE_TEMPLATES, acquireTreeGeometry } = await source('render/procedural-tree-generator.ts');
const { ProceduralTreeAsset, PlantWeatherMaterial } = await source('render/procedural-tree-layer.ts');
const { VegetationLayer } = await source('render/vegetation-layer.ts');
const { GroundDetailLayer } = await source('render/ground-detail-layer.ts');
const { NaturalResourceProjection } = await source('render/natural-resource-projection.ts');
const { createTerrainElevationSampler } = await source('render/terrain-geometry.ts');
const { createPlankGeometry, createVesselGeometry, installWoodGrain } = await source('render/procedural-asset-kit.ts');
const { SurfaceSnowModifier } = await source('render/environment-surface-presentation.ts');
const { WORLD_GRID_CONTRACT: grid } = await source('runtime/lifelens-contract.ts');
let tests = 0;
function test(name, run) { run(); tests++; console.log('PASS', name); }
const dispose = g => { g.bark.dispose(); g.foliage.dispose(); };
const triangles = g => g.index.count / 3;
const chunk = (x = 0, y = 0, extra = {}) => ({ x, y, elevation01: .45 + x * .01, waterKind: 'None',
  waterAvailability: .5, forestCoverage01: 1, grassCoverage01: .7, shrubCoverage01: .6,
  rockCoverage01: .3, moisture01: .7, ...extra });
const terrain = chunks => ({ available: true, worldSeed: 'ecology-review', centerChunkX: 0, centerChunkY: 0, chunks });
const resource = (quantity, id = 'wood', material = 'Wood') => ({ id, material,
  gridX: grid.gridCellsPerChunk / 2, gridY: grid.gridCellsPerChunk / 2,
  quantity, maxQuantity: 10, renewable: true, regenerationPerDay: 1 });
const projection = (w, quantity, material = 'Wood') => new NaturalResourceProjection({ available: true, resources: [resource(quantity, 'wood', material)] }, w);

test('three species are distinct, deterministic, finite, tapered and grounded', () => {
  const signatures = new Set();
  for (const t of TREE_TEMPLATES) {
    const a = generateTree(t.id, 'a'), same = generateTree(t.id, 'a'), other = generateTree(t.id, 'b');
    assert.deepEqual(a.bark.getAttribute('position').array, same.bark.getAttribute('position').array);
    assert.deepEqual(a.foliage.getAttribute('position').array, same.foliage.getAttribute('position').array);
    assert.notDeepEqual(a.foliage.getAttribute('position').array, other.foliage.getAttribute('position').array);
    for (const g of [a.bark, a.foliage]) {
      assert([...g.getAttribute('position').array, ...g.getAttribute('normal').array].every(Number.isFinite));
      assert(g.index.array.every(i => i < g.getAttribute('position').count));
      assert(g.boundingBox.max.y <= 1.08); assert(g.boundingBox.min.y >= -.045);
    }
    signatures.add(JSON.stringify([a.bark.boundingBox, a.foliage.boundingBox, a.foliage.index.count]));
    dispose(a); dispose(same); dispose(other);
  }
  assert.equal(signatures.size, 3);
});
test('mobile/distant LOD reduces triangles with the same branch endpoints', () => {
  for (const t of TREE_TEMPLATES) {
    const near = generateTree(t.id, 'lod'), far = generateTree(t.id, 'lod', true);
    assert(triangles(far.bark) + triangles(far.foliage) < (triangles(near.bark) + triangles(near.foliage)) * .55);
    assert(Math.abs(near.bark.boundingBox.max.y - far.bark.boundingBox.max.y) < .025);
    dispose(near); dispose(far);
  }
});
test('geometry cache is shared and released only after last owner, release idempotent', () => {
  const a = acquireTreeGeometry('oak', 'cache', false), b = acquireTreeGeometry('oak', 'cache', false);
  assert.equal(a.geometry, b.geometry); let releases = 0;
  a.geometry.bark.addEventListener('dispose', () => releases++);
  a.release(); a.release(); assert.equal(releases, 0); b.release(); assert.equal(releases, 1);
});
test('LOD uploads only on selection changes; zero stock clears both levels', () => {
  const a = new ProceduralTreeAsset('oak', 'lod', 4), m = new THREE.Matrix4().makeScale(6, 6, 6);
  a.setInstances(new Float32Array(m.elements), 1, new Float32Array([.2, .6, .1]));
  const camera = new THREE.PerspectiveCamera(); camera.position.set(0, 4, 10); a.update(camera);
  assert.equal(a.levels[0].bark.count, 1); assert.equal(a.levels[1].bark.count, 0);
  const version = a.levels[0].bark.instanceMatrix.version; camera.position.z += 3; a.update(camera);
  assert.equal(a.levels[0].bark.instanceMatrix.version, version);
  camera.position.z = 100; a.update(camera); assert.equal(a.levels[1].bark.count, 1);
  a.setInstances(new Float32Array(), 0, new Float32Array());
  assert(a.levels.every(l => l.bark.count === 0 && l.foliage.count === 0)); a.dispose(); a.dispose();
});
test('weather is observed season/wind/time, unavailable resets; no accumulation clock', () => {
  const weather = new PlantWeatherMaterial(), material = new THREE.MeshLambertMaterial(); weather.install(material);
  weather.setEnvironment({ available: true, windIntensity01: .8, simulationMinute: 123,
    calendar: { season: 'Winter' }, precipitationType: 'Snow', precipitationIntensity01: .6, airTemperatureC: -4 });
  assert.equal(weather.uniforms.llWind.value, .8); assert.equal(weather.uniforms.llTime.value, 123);
  assert.equal(weather.uniforms.llLeafDensity.value, .18); assert(weather.uniforms.llSnow.value > 0);
  weather.setEnvironment({ available: false }); assert.equal(weather.uniforms.llWind.value, 0);
  assert.equal(weather.uniforms.llLeafDensity.value, 1); assert.equal(weather.uniforms.llSnow.value, 0); material.dispose();
});
test('stock changes, reload and unchanged terrain preserve procedural identity and buffers', () => {
  const w = terrain([chunk(0, 0, { forestCoverage01: 0 })]), layer = new VegetationLayer();
  const original = JSON.stringify(w); layer.setTerrain(w, projection(w, 10));
  assert.equal(layer.instanceCount, 3); const full = layer.actualTrees.map(a => Array.from(a.matrices));
  const slots = projection(w, 10).candidates('trees', 0, 0, w.worldSeed, [], 6);
  const partial = projection(w, 4).candidates('trees', 0, 0, w.worldSeed, [], 6);
  assert.equal(partial[0].assetKey, slots[0].assetKey);
  layer.setTerrain(w, projection(w, 4)); assert(layer.instanceCount < 3);
  layer.setTerrain(w, projection(w, 0)); assert.equal(layer.instanceCount, 0);
  assert(layer.actualTrees.every(a => a.levels.every(l => l.bark.count === 0 && l.foliage.count === 0)));
  layer.setTerrain(w, projection(w, 10)); assert.deepEqual(layer.actualTrees.map(a => Array.from(a.matrices)), full);
  const versions = layer.actualTrees.flatMap(a => a.levels.map(l => l.bark.instanceMatrix.version));
  for (let i = 0; i < 100; i++) layer.setTerrain(w, projection(w, 10));
  assert.deepEqual(layer.actualTrees.flatMap(a => a.levels.map(l => l.bark.instanceMatrix.version)), versions);
  assert.equal(JSON.stringify(w), original); layer.dispose();
});
test('all tree anchors use the shared terrain sampler; origin rebasing preserves world identity', () => {
  const w = terrain([chunk(-1), chunk(), chunk(1)]), a = new VegetationLayer(); a.setTerrain(w);
  const sample = createTerrainElevationSampler(w), size = grid.worldUnitsPerChunk;
  for (let i = 0; i < a.instanceCount; i++) {
    const m = new THREE.Matrix4().fromArray(a.instanceMatrices, i * 16), p = new THREE.Vector3().setFromMatrixPosition(m);
    const cx = Math.floor(p.x / size + .5), cy = Math.floor(p.z / size + .5);
    assert(Math.abs(p.y - sample(cx, cy, p.x / size - cx + .5, p.z / size - cy + .5) * grid.elevationScale) < 1e-5);
  }
  const b = new VegetationLayer(); b.setTerrain({ ...w, centerChunkX: 1 });
  assert.equal(a.instanceCount, b.instanceCount);
  for (let i = 0; i < a.instanceCount; i++) {
    assert(Math.abs(a.instanceMatrices[i * 16 + 12] - b.instanceMatrices[i * 16 + 12] - size) < 1e-5);
    for (const offset of [0, 2, 5, 8, 10, 13, 14]) assert.equal(a.instanceMatrices[i * 16 + offset], b.instanceMatrices[i * 16 + offset]);
  }
  a.dispose(); b.dispose();
});
test('ground detail stock clears real grass/shrub/rock batches and restores from state', () => {
  const w = terrain([chunk(0, 0, { forestCoverage01: 0, grassCoverage01: 0, shrubCoverage01: 0, rockCoverage01: 0 })]);
  for (const [material, name] of [['Fiber', 'grass'], ['PlantFood', 'shrubs'], ['Stone', 'rocks']]) {
    const layer = new GroundDetailLayer(); layer.setTerrain(w, projection(w, 10, material)); assert.equal(layer[name].count, 3);
    layer.setTerrain(w, projection(w, 0, material)); assert.equal(layer[name].count, 0); layer.dispose();
  }
});

test('component geometry preserves bed/plank support bounds, vessel has an inner wall', () => {
  const plank = createPlankGeometry(), vessel = createVesselGeometry();
  for (const g of [plank, vessel]) {
    assert(Math.abs(g.boundingBox.min.y + .5) < 1e-6); assert(Math.abs(g.boundingBox.max.y - .5) < 1e-6);
    assert([...g.getAttribute('position').array].every(Number.isFinite));
  }
  assert(vessel.index.count > 600); plank.dispose(); vessel.dispose();
});
test('material grain and snow compose and use different shader cache keys', () => {
  const wood = new THREE.MeshStandardMaterial(), stone = new THREE.MeshStandardMaterial();
  installWoodGrain(wood); const snow = new SurfaceSnowModifier(); snow.install(wood); snow.install(stone);
  assert.notEqual(wood.customProgramCacheKey(), stone.customProgramCacheKey());
  const shader = { uniforms: {}, vertexShader: '#include <begin_vertex>\n#include <project_vertex>',
    fragmentShader: '#include <color_fragment>\n#include <normal_fragment_maps>' };
  wood.onBeforeCompile(shader, {}); assert(shader.vertexShader.includes('llGrainPosition'));
  assert(shader.fragmentShader.includes('llCover')); wood.dispose(); stone.dispose();
});
test('reordered streamed chunks keep the same forest matrices', () => {
  const w = terrain([chunk(1), chunk(), chunk(-1)]), a = new VegetationLayer(), b = new VegetationLayer();
  a.setTerrain(w); b.setTerrain({ ...w, chunks: [...w.chunks].reverse() });
  assert.equal(a.instanceCount, b.instanceCount);
  assert.deepEqual(a.instanceMatrices.subarray(0, a.instanceCount * 16), b.instanceMatrices.subarray(0, b.instanceCount * 16));
  a.dispose(); b.dispose();
});

const window = terrain(Array.from({ length: 121 }, (_, i) => chunk(i % 11 - 5, Math.floor(i / 11) - 5)));
const benchmark = profile => {
  const old = globalThis.window;
  if (profile === 'mobile') globalThis.window = { innerWidth: 390, innerHeight: 844, matchMedia: () => ({ matches: true }) };
  const start = performance.now(), layer = new VegetationLayer(); const generatedMs = performance.now() - start;
  const begin = performance.now(); layer.setTerrain(window); const dressingMs = performance.now() - begin;
  const sameStart = performance.now(); for (let i = 0; i < 200; i++) layer.setTerrain(window);
  const unchangedMs = (performance.now() - sameStart) / 200;
  let triangles = 0, geometryBytes = 0, instanceBytes = 0, drawCalls = 0;
  layer.group.traverse(o => { if (!o.isInstancedMesh) return;
    if (o.count) { triangles += o.geometry.index.count / 3 * o.count; drawCalls++; }
    for (const a of Object.values(o.geometry.attributes)) geometryBytes += a.array.byteLength;
    geometryBytes += o.geometry.index.array.byteLength; instanceBytes += o.instanceMatrix.array.byteLength + (o.instanceColor?.array.byteLength ?? 0);
  });
  const result = { profile, trees: layer.instanceCount, generatedMs, dressingMs, unchangedMs,
    triangles, geometryBytes, instanceBytes, drawCalls };
  assert(layer.instanceCount <= (profile === 'mobile' ? 6 : 9) * 121); assert(drawCalls <= 12);
  layer.dispose(); globalThis.window = old; return result;
};
const benchmarks = ['desktop', 'mobile'].map(benchmark);
writeFileSync(new URL('./review/cpu-metrics.json', import.meta.url), JSON.stringify({ tests, benchmarks }, null, 2));
console.log(JSON.stringify({ tests, benchmarks }, null, 2));
