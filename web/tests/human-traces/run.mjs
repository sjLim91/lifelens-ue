import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, resolve } from 'node:path';
import * as THREE from 'three';
import ts from 'typescript';
import { renderToStaticMarkup } from 'react-dom/server';
import { createElement } from 'react';

const require = createRequire(import.meta.url);
const modules = new Map();
function load(path) {
  if (modules.has(path)) return modules.get(path).exports;
  const module = { exports: {} };
  modules.set(path, module);
  const code = ts.transpileModule(readFileSync(path, 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020, jsx: ts.JsxEmit.ReactJSX },
  }).outputText;
  new Function('require', 'module', 'exports', code)(id =>
    id.startsWith('.') ? load(resolve(dirname(path), `${id}.ts`)) : require(id), module, module.exports);
  return module.exports;
}
const source = file => load(resolve(import.meta.dirname, '../../src', file));
const { visibleHumanTraces, describeHumanTrace, humanTraceFocus } = source('state/human-traces.ts');
const { buildHumanTraceGeometry, HumanTraceLayer } = source('render/human-trace-layer.ts');
const { createTerrainElevationSampler } = source('render/terrain-geometry.ts');
const { observerStore } = source('state/observer-store.ts');
const { WORLD_GRID_CONTRACT: grid } = source('runtime/lifelens-contract.ts');
const { HumanTracePanel, HumanTraceDetail } = source('ui/human-traces.tsx');

const resource = (extra = {}) => ({ id: 'resource:9007199254740993', kind: 'ResourceUse', gridX: 16, gridY: 16,
  material: 'Wood', quantity: 4, baselineQuantity: 10, renewable: true, ...extra });
const residue = (extra = {}) => ({ id: 'residue:1', kind: 'Residue', gridX: 20, gridY: 20,
  sourceResidentId: '1', amount: 1, intensity: 0.4, radiusTiles: 3, ...extra });
const facility = (extra = {}) => ({ id: 'facility:1', kind: 'Facility', gridX: 10, gridY: 10,
  sourceResidentId: '1', facilityKind: 'Shelter', state: 'UnderConstruction', progress01: 0.25,
  deliveredMaterialUnits: 3, requiredMaterialUnits: 13, active: false, lit: false, ...extra });
function windowOf(entries = [], centerChunkX = 0, centerChunkY = 0) {
  const chunks = [];
  for (let y = -2; y <= 2; y++) for (let x = -2; x <= 2; x++) {
    chunks.push({ x: centerChunkX + x, y: centerChunkY + y,
      elevation01: 0.5 + x * 0.035 + y * 0.018, waterKind: 'None', waterAvailability: 0 });
  }
  return { available: true, worldSeed: '42', centerChunkX, centerChunkY, chunks, humanTraces: { total: entries.length, entries } };
}
let passed = 0, failed = 0;
function test(name, run) {
  try { run(); passed++; console.log(`PASS ${name}`); }
  catch (error) { failed++; console.error(`FAIL ${name}: ${error.stack}`); }
}
function near(a, b, label, tolerance = 1e-4) { assert.ok(Math.abs(a - b) < tolerance, `${label}: ${a} vs ${b}`); }

test('new worlds and older runtimes never fabricate traces', () => {
  assert.deepEqual(visibleHumanTraces(null), []);
  assert.deepEqual(visibleHumanTraces({ ...windowOf(), humanTraces: undefined }), []);
  assert.deepEqual(visibleHumanTraces(windowOf()), []);
});
test('restored resources, vanished residues, untouched plans and invalid payloads stay silent', () => {
  const bad = [resource({ quantity: 10 }), resource({ quantity: NaN }), residue({ intensity: 0 }),
    residue({ radiusTiles: NaN }), facility({ progress01: 0, deliveredMaterialUnits: 0 }),
    resource({ material: 'Water' }), resource({ gridX: Infinity }), resource({ gridX: 5000 }),
    resource({ kind: 'Unsupported' }), null];
  assert.deepEqual(visibleHumanTraces(windowOf(bad)), []);
});
test('ID precision is preserved and duplicate/large sets are bounded', () => {
  const entries = Array.from({ length: 100 }, (_, i) => resource({ id: `resource:${9007199254740993n + BigInt(i)}` }));
  const actual = visibleHumanTraces(windowOf([entries[0], ...entries]));
  assert.equal(actual.length, 64);
  assert.equal(actual[0].id, 'resource:9007199254740993');
  assert.equal(new Set(actual.map(trace => trace.id)).size, 64);
});
test('all mark vertices follow sloping terrain at negative coordinates', () => {
  const terrain = windowOf([resource({ gridX: -1, gridY: -2 })], -1, -1);
  const sampler = createTerrainElevationSampler(terrain);
  const { geometry } = buildHumanTraceGeometry(terrain, visibleHumanTraces(terrain));
  const p = geometry.attributes.position, s = grid.worldUnitsPerChunk;
  for (let i = 0; i < p.count; i++) {
    const x = p.getX(i), z = p.getZ(i);
    const cx = terrain.centerChunkX + Math.floor(x / s + 0.5);
    const cy = terrain.centerChunkY + Math.floor(z / s + 0.5);
    const expected = sampler(cx, cy, x / s + terrain.centerChunkX - cx + 0.5,
      z / s + terrain.centerChunkY - cy + 0.5) * grid.elevationScale + 0.045;
    near(p.getY(i), expected, 'ground contact');
  }
  const indices = geometry.index.array;
  for (let i = 0; i < indices.length; i += 3) {
    const a = new THREE.Vector3().fromBufferAttribute(p, indices[i]);
    const b = new THREE.Vector3().fromBufferAttribute(p, indices[i + 1]);
    const c = new THREE.Vector3().fromBufferAttribute(p, indices[i + 2]);
    assert.ok(b.sub(a).cross(c.sub(a)).y >= -1e-5, 'a mark faces underground');
  }
  geometry.dispose();
});
test('batched marks can be picked independently and clear after regeneration', () => {
  const layer = new HumanTraceLayer();
  layer.setTerrain(windowOf([resource(), residue()]));
  assert.equal(layer.group.children.length, 1, 'traces should share a draw call');
  layer.group.updateMatrixWorld(true);
  const ray = new THREE.Raycaster(new THREE.Vector3(0.1, 100, 0.1), new THREE.Vector3(0, -1, 0));
  assert.equal(layer.pickTrace(ray), resource().id);
  ray.set(new THREE.Vector3(1, 100, 1), new THREE.Vector3(0, -1, 0));
  assert.equal(layer.pickTrace(ray), residue().id);
  layer.setTerrain(windowOf());
  assert.equal(layer.pickTrace(ray), null);
  layer.dispose();
});
test('unchanged snapshots retain geometry; replacement and teardown dispose it', () => {
  const layer = new HumanTraceLayer();
  const terrain = windowOf([resource()]);
  layer.setTerrain(terrain);
  const mesh = layer.group.children[0], first = mesh.geometry;
  let disposed = 0;
  first.addEventListener('dispose', () => disposed++);
  layer.setTerrain(structuredClone(terrain));
  assert.equal(mesh.geometry, first);
  layer.setSelectedTrace(resource().id);
  assert.equal(disposed, 1);
  let finalDisposed = false;
  mesh.geometry.addEventListener('dispose', () => { finalDisposed = true; });
  layer.dispose();
  assert.ok(finalDisposed);
});
test('origin shifts rebase marks by exactly one chunk without leaving ghosts', () => {
  const entries = [resource()];
  const before = buildHumanTraceGeometry(windowOf(entries), entries).geometry;
  const after = buildHumanTraceGeometry(windowOf(entries, 1, -1), entries).geometry;
  near(after.attributes.position.getX(0) - before.attributes.position.getX(0), -grid.worldUnitsPerChunk, 'x rebase');
  near(after.attributes.position.getZ(0) - before.attributes.position.getZ(0), grid.worldUnitsPerChunk, 'z rebase');
  before.dispose(); after.dispose();
});
test('place focus preserves exact grid positions across positive and negative borders', () => {
  for (const [gridX, gridY] of [[0, 0], [-1, -33], [32, 63], [591, 975]]) {
    const focus = humanTraceFocus({ gridX, gridY });
    near((focus.centerChunkX + focus.panX / grid.worldUnitsPerChunk + 0.5) * grid.gridCellsPerChunk, gridX, 'focus x');
    near((focus.centerChunkY + focus.panZ / grid.worldUnitsPerChunk + 0.5) * grid.gridCellsPerChunk, gridY, 'focus y');
    assert.ok(!('angle' in focus) && !('elevation' in focus), 'focus must not reverse the view');
  }
});
test('trace selection clears on disappearance, resident selection, and world replacement', () => {
  observerStore.resetWorld();
  observerStore.update({ world: { worldSeed: '42' }, residents: [{ id: '1', name: '하늘' }], terrain: windowOf([resource()]) });
  observerStore.selectHumanTrace(resource().id);
  assert.equal(observerStore.getSnapshot().selectedHumanTraceId, resource().id);
  observerStore.selectResident('1');
  assert.equal(observerStore.getSnapshot().selectedHumanTraceId, null);
  observerStore.selectHumanTrace(resource().id);
  assert.equal(observerStore.getSnapshot().selectedResidentId, null);
  observerStore.update({ terrain: windowOf() });
  assert.equal(observerStore.getSnapshot().selectedHumanTraceId, null);
  observerStore.update({ terrain: windowOf([resource()]) });
  observerStore.selectHumanTrace(resource().id);
  observerStore.update({ world: { worldSeed: '99' }, terrain: windowOf([resource()]) });
  assert.equal(observerStore.getSnapshot().selectedHumanTraceId, null);
  observerStore.resetWorld();
});
test('Korean descriptions report actual quantities, sources and facility states', () => {
  assert.equal(describeHumanTrace(resource({ quantity: 0 })).status, '소진');
  assert.match(describeHumanTrace(resource()).detail, /4 \/ 처음 10/);
  assert.match(describeHumanTrace(residue(), [{ id: '1', name: '하늘' }]).detail, /하늘/);
  assert.equal(describeHumanTrace(facility()).status, '건설 25%');
  assert.equal(describeHumanTrace(facility({ state: 'Ruined' })).status, '무너짐');
  const panel = renderToStaticMarkup(createElement(HumanTracePanel, { terrain: windowOf([resource(), facility()]), selectedId: null, onFocus() {} }));
  assert.match(panel, /이 장소 보기/);
  assert.match(panel, /나무 채집 지점/);
  const detail = renderToStaticMarkup(createElement(HumanTraceDetail, { trace: facility(), residents: [], onClose() {} }));
  assert.match(detail, /실제 구조물/);
  assert.match(detail, /선택한 시설만 바닥에 얇은 위치 강조선/);
  assert.ok(!detail.includes('UnderConstruction'));
});

console.log(`${passed} passed; ${failed} failed`);
if (failed) process.exitCode = 1;
