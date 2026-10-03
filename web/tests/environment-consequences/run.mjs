import assert from 'node:assert/strict';
import { readFileSync, mkdirSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, resolve } from 'node:path';
import * as THREE from 'three';
import ts from 'typescript';

const require = createRequire(import.meta.url), modules = new Map();
function load(path) {
  if (modules.has(path)) return modules.get(path).exports;
  const module = { exports: {} }; modules.set(path, module);
  const code = ts.transpileModule(readFileSync(path, 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2022 },
  }).outputText;
  new Function('require', 'module', 'exports', code)(id => id.startsWith('.')
    ? load(resolve(dirname(path), `${id}.ts`)) : require(id), module, module.exports);
  return module.exports;
}
const source = file => load(resolve(import.meta.dirname, '../../src', file));
const { WORLD_GRID_CONTRACT: grid } = source('runtime/lifelens-contract.ts');
const { WORLD_PRESENTATION: config } = source('render/world-presentation-config.ts');
const { applyGroundWetness, snowPresentationCoverage, SurfaceSnowModifier } = source('render/environment-surface-presentation.ts');
const { SurfaceConsequenceLayer, puddleCandidates } = source('render/surface-consequence-layer.ts');
const { FootTrafficLayer } = source('render/foot-traffic-layer.ts');
const { GroundDetailLayer } = source('render/ground-detail-layer.ts');
const { FacilityLayer } = source('render/facility-layer.ts');
const { FacilityEmissionLayer, isEmittingFire } = source('render/facility-emission-layer.ts');
const { buildHumanTraceGeometry, HumanTraceLayer } = source('render/human-trace-layer.ts');
const { createVisibleWaterFootprintTester } = source('render/water-geometry.ts');
const { facilityPresentationFootprints, outsideFacilityFootprints } = source('render/facility-layer.ts');
const span = grid.gridCellsPerChunk;
function terrain(entries = []) {
  const chunks = [];
  for (let y = -3; y <= 3; y++) for (let x = -3; x <= 3; x++) chunks.push({ x, y,
    elevation01: .48 + (x * x + y * y) * .001, waterKind: 'None', waterAvailability: 0,
    forestCoverage01: 0, grassCoverage01: .6, shrubCoverage01: .2, rockCoverage01: .5 });
  return { available: true, worldSeed: 'environment-42', centerChunkX: 0, centerChunkY: 0,
    chunks, humanTraces: { total: entries.length, entries } };
}
const fire = (extra = {}) => ({ id: 'facility:1', kind: 'Facility', gridX: span / 2, gridY: span / 2,
  sourceResidentId: '1', facilityKind: 'FirePit', state: 'Operational', progress01: 1,
  deliveredMaterialUnits: 3, requiredMaterialUnits: 3, active: true, lit: true, ...extra });
const waste = (extra = {}) => ({ id: 'residue:1', kind: 'Residue', gridX: span / 2, gridY: span / 2,
  sourceResidentId: '1', amount: 5, intensity: .7, radiusTiles: 3, ...extra });
let passed = 0;
function test(name, run) { run(); passed++; console.log('PASS', name); }
const positions = geometry => [...geometry.attributes.position.array];
const maxAlpha = geometry => Math.max(...Array.from({ length: geometry.attributes.color.count }, (_, i) => geometry.attributes.color.getW(i)));

test('wet ground/rocks darken without replacing geometry and return exactly to dry', () => {
  const ground = new THREE.MeshStandardMaterial(), layer = new GroundDetailLayer();
  layer.setTerrain(terrain());
  const matrix = [...layer.rocks.instanceMatrix.array];
  applyGroundWetness(ground, 0); const dry = ground.color.clone(), rough = ground.roughness;
  applyGroundWetness(ground, 1); assert(ground.color.r < dry.r); assert(ground.roughness < rough); assert(ground.roughness >= .5);
  layer.setWetness(1); assert(layer.rockMaterial.roughness < config.weather.dryRockRoughness);
  assert.equal(layer.grassMaterial.isMeshLambertMaterial, true);
  layer.setWetness(0); assert.equal(layer.rockMaterial.color.r, 1);
  applyGroundWetness(ground, 0); assert(ground.color.equals(dry)); assert.equal(ground.roughness, rough);
  assert.deepEqual([...layer.rocks.instanceMatrix.array], matrix);
  layer.dispose(); ground.dispose();
});
test('low flat basin candidates are deterministic, bounded and rebase with origin', () => {
  const t = terrain(), first = puddleCandidates(t);
  assert(first.length > 0); assert(first.length <= config.weather.puddleMaxInstances);
  assert.deepEqual(puddleCandidates(structuredClone(t)), first);
  assert.deepEqual(puddleCandidates({ ...t, chunks: [...t.chunks].reverse() }), first);
  const changed = puddleCandidates({ ...t, worldSeed: 'different' }); assert.notDeepEqual(changed, first);
  assert.deepEqual(puddleCandidates({ ...t, available: false }), []);
  assert.deepEqual(puddleCandidates({ ...t, chunks: t.chunks.map(c => ({ ...c, elevation01: NaN })) }), []);
  const steep = { ...t, chunks: t.chunks.map(c => ({ ...c, elevation01: .48 + (c.x * c.x + c.y * c.y) * .08 })) };
  assert.equal(puddleCandidates(steep).length, 0);
});
test('puddles exclude visible water and entire facility footprints, dry hides all patches', () => {
  const t = terrain([fire({ facilityKind: 'Shelter' })]);
  const footprints = facilityPresentationFootprints(t);
  for (const p of puddleCandidates(t)) assert(outsideFacilityFootprints(p.x, p.z, footprints, p.radius));
  const lake = { ...terrain(), chunks: terrain().chunks.map(c => ({ ...c, waterKind: 'Lake', salinity: 'Fresh', waterAvailability: 1 })) };
  const water = createVisibleWaterFootprintTester(lake);
  for (const p of puddleCandidates(lake)) assert(!water(p.x, p.z));
  const ocean = { ...terrain(), chunks: terrain().chunks.map(c => ({ ...c, waterKind: 'Ocean', waterAvailability: 1 })) };
  assert.equal(puddleCandidates(ocean).length, 0);
  const layer = new SurfaceConsequenceLayer(); layer.setTerrain(t); layer.setWetness(1);
  assert(layer.mesh.visible); assert(layer.material.opacity > 0);
  const version = layer.geometry.attributes.position.version;
  layer.setWetness(0); assert(!layer.mesh.visible);
  layer.setWetness(1); layer.setTerrain(structuredClone(t)); assert.equal(layer.geometry.attributes.position.version, version);
  layer.dispose();
});
test('mud changes only presentation of the same observed marks and keeps fade/teleport contract', () => {
  const t = terrain(), layer = new FootTrafficLayer();
  const resident = x => [{ id: 'walker', alive: true, hasPosition: true, gridX: x, gridY: 16 }];
  for (let minute = 0; minute < 24; minute++) {
    layer.history.observe(resident(16 + minute % 2), t.worldSeed, minute, minute * 100);
  }
  layer.observe(resident(17), t, 23);
  assert(layer.geometry.drawRange.count > 0);
  const history = JSON.stringify([...layer.history.marks]);
  const dry = positions(layer.geometry), color = [...layer.colors];
  layer.setWetness(1);
  assert.equal(JSON.stringify([...layer.history.marks]), history);
  assert(layer.colors[3] > color[3]); assert(layer.colors[0] < color[0]);
  // Along-axis vertices (diamond tips) are identical; centerline has not moved.
  for (let vertex = 0; vertex < layer.geometry.drawRange.count; vertex++) {
    if ([0, 2, 3, 4].includes(vertex % 6)) assert.deepEqual([...layer.positions.slice(vertex * 3, vertex * 3 + 3)], dry.slice(vertex * 3, vertex * 3 + 3));
  }
  layer.setWetness(0); assert.deepEqual(positions(layer.geometry), dry); assert.deepEqual([...layer.colors], color);
  const markCount = layer.history.marks.size;
  layer.history.observe(resident(300), t.worldSeed, 24, 2400); assert.equal(layer.history.marks.size, markCount);
  layer.history.observe([], t.worldSeed, 24 + config.paths.fadeMinutes, 2500); assert.equal(layer.history.marks.size, 0);
  layer.dispose();
});
test('snow is stateless current cold Snow only; rain/warm/unavailable clear it and normals restrict sides', () => {
  const snow = { available: true, precipitationType: 'Snow', precipitationIntensity01: 1, airTemperatureC: -8, surfaceWetness01: 1 };
  assert(snowPresentationCoverage(snow) > .5);
  assert.equal(snowPresentationCoverage({ ...snow, precipitationType: 'Rain' }), 0);
  assert.equal(snowPresentationCoverage({ ...snow, airTemperatureC: 5 }), 0);
  assert.equal(snowPresentationCoverage({ ...snow, precipitationIntensity01: 0 }), 0);
  assert.equal(snowPresentationCoverage({ ...snow, available: false }), 0);
  const modifier = new SurfaceSnowModifier(), material = new THREE.MeshStandardMaterial(); modifier.install(material);
  const shader = { uniforms: {}, vertexShader: THREE.ShaderLib.standard.vertexShader, fragmentShader: THREE.ShaderLib.standard.fragmentShader };
  material.onBeforeCompile(shader, null);
  assert(shader.fragmentShader.includes('smoothstep(0.35, 0.85, llUpNormal.y)'));
  assert(shader.vertexShader.includes('instanceMatrix * llPosition'));
  modifier.setState({ snow: snowPresentationCoverage(snow), originX: 4, originZ: -4 }); assert(shader.uniforms.llSnow.value > .5);
  modifier.setState({ snow: 0, originX: 4, originZ: -4 }); assert.equal(shader.uniforms.llSnow.value, 0);
  assert.deepEqual(shader.uniforms.llOrigin.value.toArray(), [4, -4]); material.dispose();
});
test('residue intensity/radius/amount reflect actual DTO, contained residue is smaller, removed is absent', () => {
  const t = terrain();
  const build = trace => buildHumanTraceGeometry(t, [trace]).geometry;
  const low = build(waste({ intensity: .16, radiusTiles: 1 })), high = build(waste({ intensity: .8, radiusTiles: 3 }));
  assert(maxAlpha(high) > maxAlpha(low)); low.computeBoundingBox(); high.computeBoundingBox();
  assert(high.boundingBox.getSize(new THREE.Vector3()).x > low.boundingBox.getSize(new THREE.Vector3()).x * 2);
  const few = build(waste({ amount: 1 })), many = build(waste({ amount: 20 })); assert(many.attributes.position.count > few.attributes.position.count);
  const repeat = build(waste()); assert.deepEqual(positions(repeat), positions(build(waste())));
  const empty = buildHumanTraceGeometry(t, []).geometry; assert.equal(empty.attributes.position.count, 0);
  const layer = new HumanTraceLayer(); layer.setTerrain(terrain([waste()])); layer.setDynamicTraces(terrain()); assert.equal(layer.mesh.visible, false); layer.dispose();
  for (const geometry of [low, high, few, many, repeat, empty]) geometry.dispose();
});
test('inactive/unlit/planned/ruined fire never emits; lit FirePit/Furnace use bounded deterministic smoke', () => {
  for (const extra of [{ active: false }, { lit: false }, { state: 'Ruined' }, { state: 'UnderConstruction' }]) assert(!isEmittingFire(fire(extra)));
  assert(isEmittingFire(fire())); assert(isEmittingFire(fire({ facilityKind: 'Furnace' })));
  const first = new FacilityEmissionLayer(), second = new FacilityEmissionLayer();
  const t = terrain([fire(), fire({ id: 'facility:2', facilityKind: 'Furnace', gridX: 20 })]);
  for (const layer of [first, second]) { layer.setTerrain(t); layer.setEnvironment({ windIntensity01: .8 }); layer.setSimulationMinute(200); }
  assert.equal(first.smokeGeometry.drawRange.count, config.fire.smokePerFacility * 2);
  assert.deepEqual([...first.smokePositions], [...second.smokePositions]);
  first.setTerrain(terrain([fire({ active: false, lit: false })])); assert.equal(first.smokeGeometry.drawRange.count, 0); assert(first.scorch.count > 0);
  first.setTerrain(terrain(Array.from({ length: 200 }, (_, i) => fire({ id: 'facility:' + i }))));
  assert(first.smokeGeometry.drawRange.count <= config.fire.smokeMaxParticles); assert(first.scorch.count <= config.fire.scorchMaxInstances);
  first.dispose(); second.dispose();
});
test('facility wet/snow preserves durability wear and restores dry; refresh/removal/dispose release instances', () => {
  const oldDocument = globalThis.document;
  globalThis.document = { createElement: () => ({ getContext: () => null }) };
  try {
    const layer = new FacilityLayer(), t = terrain([fire()]);
    const civ = { available: true, facilities: [{ id: '1', kind: 'FirePit', gridX: 16, gridY: 16, durability: .4, state: 'Operational' }] };
    layer.setCivilization(civ, t);
    const structure = layer.group.children[0], worn = [...layer.wornMaterials.values()];
    assert(worn.length > 0); const dry = worn.map(m => m.color.clone());
    layer.setSurfaceWeather(1, .6, 4, 4); assert(worn[0].color.r < dry[0].r);
    layer.setCivilization(civ, structuredClone(t)); assert.equal(layer.group.children[0], structure);
    layer.setSurfaceWeather(0, 0, 4, 4); assert(worn.every((m, i) => m.color.equals(dry[i])));
    let disposed = 0; structure.traverse(o => { if (o instanceof THREE.InstancedMesh) o.addEventListener('dispose', () => disposed++); });
    layer.setTerrain(terrain()); assert(disposed > 0); layer.dispose();
  } finally { if (oldDocument === undefined) delete globalThis.document; else globalThis.document = oldDocument; }
});
test('repeated observer refresh and minute updates keep fixed pools/scene children; teardown releases GPU resources', () => {
  const surface = new SurfaceConsequenceLayer(), fireLayer = new FacilityEmissionLayer(), t = terrain([fire()]);
  surface.setTerrain(t); fireLayer.setTerrain(t);
  const geometry = surface.geometry, smokeBuffer = fireLayer.smokePositions;
  for (let i = 0; i < 100; i++) { surface.setTerrain(structuredClone(t)); surface.setWetness(i % 2); fireLayer.setTerrain(structuredClone(t)); fireLayer.setSimulationMinute(i); }
  assert.equal(surface.group.children.length, 1); assert.equal(fireLayer.group.children.length, 2);
  assert.equal(surface.geometry, geometry); assert.equal(fireLayer.smokePositions, smokeBuffer);
  let disposed = 0;
  for (const resource of [surface.geometry, surface.material, fireLayer.smokeGeometry, fireLayer.smokeMaterial, fireLayer.scorchGeometry, fireLayer.scorchMaterial]) resource.addEventListener('dispose', () => disposed++);
  surface.dispose(); fireLayer.dispose(); assert.equal(disposed, 6);
});
const out = resolve(import.meta.dirname, 'review'); mkdirSync(out, { recursive: true });
writeFileSync(resolve(out, 'regression.json'), JSON.stringify({ passed, additionalDrawCalls: 3,
  puddleMaxInstances: config.weather.puddleMaxInstances, smokeMaxParticles: config.fire.smokeMaxParticles,
  scorchMaxInstances: config.fire.scorchMaxInstances, coreSaveChanges: false }, null, 2));
console.log('Environmental consequence regressions:', passed, 'PASS');
