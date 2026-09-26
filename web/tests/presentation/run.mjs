import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, resolve } from 'node:path';
import * as THREE from 'three';
import ts from 'typescript';

const require = createRequire(import.meta.url);
const modules = new Map();
// Asset loading needs a browser/network; keep the real scene, camera, terrain,
// water and weather. Only the unrelated asset/dressing layers are replaced.
class AssetLayer {
  group = new THREE.Group();
  setTerrain() {} setResidents() {} setWetness() {} update() {}
  setSimulationSpeed() {} setSelectedResident() {} dispose() {}
}
function load(path) {
  if (modules.has(path)) return modules.get(path).exports;
  const module = { exports: {} };
  modules.set(path, module);
  const code = ts.transpileModule(readFileSync(path, 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
  }).outputText;
  new Function('require', 'module', 'exports', code)(id => {
    const asset = {
      './resident-world-layer': 'ResidentWorldLayer',
      './vegetation-layer': 'VegetationLayer',
      './ground-detail-layer': 'GroundDetailLayer',
    }[id];
    if (asset) return { [asset]: AssetLayer };
    return id.startsWith('.') ? load(resolve(dirname(path), `${id}.ts`)) : require(id);
  }, module, module.exports);
  return module.exports;
}
const source = file => load(resolve(import.meta.dirname, '../../src', file));
const { WorldScene } = source('render/world-scene.ts');
const { WaterLayer } = source('render/water-layer.ts');
const { buildOpenWaterSurfaceGeometry, buildFlowWaterSurfaceGeometry } = source('render/water-geometry.ts');
const { WORLD_GRID_CONTRACT: grid } = source('runtime/lifelens-contract.ts');
const size = grid.worldUnitsPerChunk, scale = grid.elevationScale;
const chunk = (x, y, waterKind = 'None', elevation01 = 0.5) => ({ x, y, waterKind, elevation01, waterAvailability: 1 });
const windowOf = (chunks, centerChunkX = 0, centerChunkY = 0, worldSeed = 'regression') => ({ available: true, centerChunkX, centerChunkY, worldSeed, chunks });
function flatWindow(centerX = 0, centerY = 0, elevation = 0.5) {
  const chunks = [];
  for (let y = -2; y <= 2; y++) for (let x = -2; x <= 2; x++) chunks.push(chunk(centerX + x, centerY + y, 'None', elevation));
  return windowOf(chunks, centerX, centerY);
}
const cameraState = (extra = {}) => ({ centerChunkX: 0, centerChunkY: 0, angle: 0, elevation: 0.28, zoom: 3.6, panX: 0, panZ: 0, ...extra });
const near = (actual, expected, message, tolerance = 1e-5) => assert.ok(Math.abs(actual - expected) < tolerance, `${message}: ${actual} vs ${expected}`);
let passed = 0, failed = 0;
function test(name, run) {
  try { run(); passed++; console.log(`PASS ${name}`); }
  catch (error) { failed++; console.error(`FAIL ${name}: ${error.message}`); }
}
function withScene(run) {
  const scene = new WorldScene();
  try { run(scene); } finally { scene.dispose(); }
}
function project(scene, point) {
  scene.camera.updateMatrixWorld(true);
  return point.clone().project(scene.camera);
}

test('close/low camera stays above elevated ground and frames its actual height', () => withScene(scene => {
  for (const elevation of [0.2, 0.5, 0.9]) {
    scene.setTerrain(flatWindow(0, 0, elevation));
    scene.setCamera(cameraState());
    scene.update(1);
    assert.ok(scene.camera.position.y > elevation * scale + 1, 'camera entered the ground');
    const target = project(scene, new THREE.Vector3(0, elevation * scale, 0));
    near(target.x, 0, 'horizontal framing'); near(target.y, 0, 'vertical framing');
  }
}));

test('camera targets the same interpolated slope that the terrain renders', () => withScene(scene => {
  const chunks = [];
  for (let y = -2; y <= 2; y++) for (let x = -2; x <= 2; x++) chunks.push(chunk(x, y, 'None', 0.5 + x * 0.05 + y * 0.025));
  scene.setTerrain(windowOf(chunks));
  scene.setCamera(cameraState({ panX: 2, panZ: -3 }));
  const expectedHeight = (0.5 + 2 / size * 0.05 - 3 / size * 0.025) * scale;
  const target = project(scene, new THREE.Vector3(2, expectedHeight, -3));
  near(target.y, 0, 'slope framing');
}));

test('low camera clears a higher hillside below its eye position', () => withScene(scene => {
  const chunks = [];
  for (let y = -8; y <= 8; y++) for (let x = -8; x <= 8; x++) chunks.push(chunk(x, y, 'None', x >= 4 ? 0.95 : 0.1));
  scene.setTerrain(windowOf(chunks));
  scene.setCamera(cameraState());
  assert.ok(scene.camera.position.y > 0.95 * scale + 1, 'camera is inside the uphill terrain');
  const target = project(scene, new THREE.Vector3(0, 0.1 * scale, 0));
  near(target.y, 0, 'raised camera still frames its focus');
}));

test('origin shifts do not jump or reverse the view across positive and negative chunk borders', () => {
  for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1], [1, -1]]) withScene(scene => {
    scene.setTerrain(flatWindow());
    scene.setCamera(cameraState({ panX: dx * 7.9, panZ: dy * 7.9 }));
    const landmark = new THREE.Vector3(0, 0.5 * scale, 0);
    const before = project(scene, landmark);
    scene.setTerrain(flatWindow(dx, dy));
    scene.setCamera(cameraState({ centerChunkX: dx, centerChunkY: dy, panX: dx * 0.1, panZ: dy * 0.1 }));
    scene.update(0);
    landmark.x -= dx * size; landmark.z -= dy * size;
    const after = project(scene, landmark);
    near(after.x, before.x, 'origin horizontal jump'); near(after.y, before.y, 'origin vertical jump');
    scene.update(1);
    const focus = project(scene, new THREE.Vector3(dx * 0.1, 0.5 * scale, dy * 0.1));
    near(focus.x, 0, 'continued pan x', 0.001); near(focus.y, 0, 'continued pan y', 0.001);
  });
});

test('new-world seed snaps to its new focus instead of retaining a distant old origin', () => withScene(scene => {
  scene.setTerrain(flatWindow(150, -90));
  scene.setCamera(cameraState({ centerChunkX: 150, centerChunkY: -90, panX: 7 }));
  scene.setTerrain({ ...flatWindow(), worldSeed: 'another-world' });
  scene.setCamera(cameraState());
  const target = project(scene, new THREE.Vector3(0, 0.5 * scale, 0));
  near(target.x, 0, 'new world x'); near(target.y, 0, 'new world y');
}));

function assertUpwardTriangles(geometry) {
  const position = geometry.attributes.position, index = geometry.index;
  const a = new THREE.Vector3(), b = new THREE.Vector3(), c = new THREE.Vector3();
  const count = index ? index.count : position.count;
  assert.ok(count > 0, 'missing water triangles');
  for (let n = 0; n < count; n += 3) {
    a.fromBufferAttribute(position, index ? index.getX(n) : n);
    b.fromBufferAttribute(position, index ? index.getX(n + 1) : n + 1);
    c.fromBufferAttribute(position, index ? index.getX(n + 2) : n + 2);
    const face = b.sub(a).cross(c.sub(a));
    assert.ok(face.y > 0, `triangle ${n / 3} faces down (${face.y})`);
  }
}
test('all 15 open-water shoreline masks face the sky', () => {
  const coords = [[0, 0], [1, 0], [1, 1], [0, 1]];
  for (let mask = 1; mask < 16; mask++) {
    const geometry = buildOpenWaterSurfaceGeometry(windowOf(coords.filter((_, i) => mask & (1 << i)).map(([x, y]) => chunk(x, y, 'Lake'))));
    try { assertUpwardTriangles(geometry); } finally { geometry.dispose(); }
  }
});
test('river ribbons and spring caps have upward front faces', () => {
  const cases = [[chunk(0, 0, 'Spring')]];
  for (const [x, y] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) cases.push([chunk(0, 0, 'River', 0.5), chunk(x, y, 'River', 0.45)]);
  for (const chunks of cases) {
    const geometry = buildFlowWaterSurfaceGeometry(windowOf(chunks));
    try { assertUpwardTriangles(geometry); } finally { geometry.dispose(); }
  }
});
test('water can be hit from above using its front face', () => {
  const geometry = buildOpenWaterSurfaceGeometry(windowOf([chunk(0, 0, 'Lake')]));
  const material = new THREE.MeshBasicMaterial({ side: THREE.FrontSide });
  try {
    const mesh = new THREE.Mesh(geometry, material);
    const ray = new THREE.Raycaster(new THREE.Vector3(0.1, 100, 0.1), new THREE.Vector3(0, -1, 0));
    assert.ok(ray.intersectObject(mesh).length > 0, 'only the underside is visible');
  } finally { geometry.dispose(); material.dispose(); }
});
test('river mouths meet the connected ocean/coast surface rather than the coastal bed height', () => {
  const window = windowOf([chunk(0, 0, 'River', 0.5), chunk(1, 0, 'Coast', 0.2), chunk(2, 0, 'Ocean', 0.1)]);
  const open = buildOpenWaterSurfaceGeometry(window), flow = buildFlowWaterSurfaceGeometry(window);
  try {
    const position = flow.attributes.position;
    // End cross-section of the first and only ribbon (the final six-segment pair).
    for (const vertex of [12, 13]) near(position.getY(vertex), open.attributes.position.getY(0), 'mouth height', 0.01);
  } finally { open.dispose(); flow.dispose(); }
});
test('changing worlds with matching terrain still rebuilds seed-dependent water', () => {
  const layer = new WaterLayer();
  try {
    const window = windowOf([chunk(0, 0, 'River'), chunk(1, 0, 'River', 0.4)]);
    layer.setTerrain(window);
    const old = layer.group.children[1].geometry;
    let disposed = false; old.addEventListener('dispose', () => { disposed = true; });
    layer.setTerrain({ ...window, worldSeed: 'new-seed' });
    assert.notEqual(layer.group.children[1].geometry, old, 'old-world water reused');
    assert.ok(disposed, 'replaced geometry was not disposed');
  } finally { layer.dispose(); }
});
test('unchanged terrain refreshes retain water geometry', () => {
  const layer = new WaterLayer();
  try {
    const window = windowOf([chunk(0, 0, 'Lake'), chunk(1, 0, 'Coast')]);
    layer.setTerrain(window);
    const old = layer.group.children[0].geometry;
    layer.setTerrain({ ...window, chunks: [...window.chunks].reverse() });
    assert.equal(layer.group.children[0].geometry, old);
  } finally { layer.dispose(); }
});

console.log(`${passed} presentation regression checks passed; ${failed} failed`);
if (failed) process.exitCode = 1;
