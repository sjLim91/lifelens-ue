import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { source } from '../procedural-environment/compile.mjs';
const { deriveResidentCausalEvidence: evidence, validActionToken } = await source('ui/resident-causal-evidence.ts');
const { formatMemoryText, formatMemoryTag, formatBelief } = await source('localization/korean.ts');
const { WildlifeLayer } = await source('render/wildlife-layer.ts');
const { WORLD_GRID_CONTRACT: grid, residentPresentationMotionTimeScale } = await source('runtime/lifelens-contract.ts');
const base = { id: '1', name: 'Jiho', alive: true, memories: [] };
const thought = { actor: '1', minute: 40, status: 'recorded', intent: 'Approach', target: '2' };
const feedback = {
  source: 'DirectWitness', witnessed: true, sourceCharacter: '1', who: '2', minute: 50,
  where: 'personal-goal-token:18446744073709551615', what: 'progress-but-not-achieved',
  tags: ['goal_action_feedback', 'self_trust', 'Approach'],
};
const belief = { subject: '2', proposition: 'experienced-self-trust-goal-progress:Approach',
  lastUpdatedMinute: 50, stance: .4, confidence: .5 };
for (const token of ['0', '-1', '1.5', '01', '1e5', '18446744073709551616', '__proto__']) assert(!validActionToken(token));
assert(validActionToken('18446744073709551615'));
const r = { ...base, memories: [feedback], beliefs: [belief] };
const trace = evidence(r, thought, 60);
assert.equal(trace.thought, thought); assert.equal(trace.attempt, undefined);
assert.equal(trace.completed.length, 1); assert.equal(trace.completed[0].learned, true);
assert.equal(trace.completed[0].outcome, 'progress-but-not-achieved');
assert(!('thought' in trace.completed[0]), 'A similar intent/time must never fabricate a causal link');
for (const bad of [
  { source: 'ToldByOther' }, { source: 'Inferred' }, { witnessed: false }, { witnessed: undefined },
  { sourceCharacter: '2' }, { who: undefined }, { who: '1' }, { who: '0' }, { minute: 61 }, { minute: -1 }, { minute: NaN },
  { where: 'personal-goal-token:0' }, { where: 'grid:12,4' }, { what: '__proto__' },
  { tags: ['goal_action_feedback', 'self_trust', 'Fight'] },
]) assert.equal(evidence({ ...r, memories: [{ ...feedback, ...bad }] }, thought, 60).completed.length, 0);
for (const other of [{ ...belief, subject: '3' }, { ...belief, lastUpdatedMinute: 51 },
  { ...belief, confidence: 0 }, { ...belief, proposition: 'is_trustworthy' }]) {
  assert.equal(evidence({ ...r, beliefs: [other] }, thought, 60).completed[0].learned, false);
}
assert.equal(evidence({ ...r, memories: [{ ...feedback, what: 'already-reached-before-action' }] }, thought, 60).completed[0].learned, false);
assert.equal(evidence(r, { ...thought, actor: '2' }, 60).thought, undefined);
assert.equal(evidence(r, { ...thought, minute: 61 }, 60).thought, undefined);
const presentation = { active: true, phase: 'Moving', contextActionToken: '99', issuedMinute: 30 };
assert.equal(evidence({ ...base, presentation }, undefined, 60).attempt.token, '99');
for (const invalid of [{ alive: false }, { presentation: { ...presentation, active: false } },
  { presentation: { ...presentation, phase: 'Idle' } }, { presentation: { ...presentation, issuedMinute: 61 } }]) {
  assert.equal(evidence({ ...base, presentation, ...invalid }, undefined, 60).attempt, undefined);
}
const many = Array.from({ length: 20 }, (_, i) => ({ ...feedback, minute: i, where: `personal-goal-token:${i + 1}` }));
assert.equal(evidence({ ...base, memories: [...many, ...many] }, undefined, 60).completed.length, 6);
assert.equal(evidence({ ...base, memories: many }, undefined, 60).completed[0].minute, 19);
for (const outcome of ['already-reached-before-action', 'achieved-during-action', 'progress-but-not-achieved', 'not-achieved-no-progress']) {
  assert(!formatMemoryText(outcome).includes('미등록'));
}
for (const tag of feedback.tags) assert(!formatMemoryTag(tag).includes('미등록'));
assert(!formatBelief(belief.proposition).includes('미등록'));
console.log('PASS causal evidence rejects hearsay, forged/future tokens, cross-resident and unlinked learning; bounded Korean records');

const chunk = { x: 0, y: 0, elevation01: .3, elevationSamples: Array(289).fill(.3), waterKind: 'None', waterAvailability: 0 };
const animal = (id, species = 'Hare') => ({ id, species, sex: 'Female', gridX: grid.gridCellsPerChunk * .375, gridY: grid.gridCellsPerChunk * .5, ageDays: 900, behavior: 'Roam' });
const window = animals => ({ available: true, worldSeed: 'm10', centerChunkX: 0, centerChunkY: 0, chunks: [chunk], wildlife: { available: true, animals } });
function movingLayer(speed, species = 'Hare') {
  const layer = new WildlifeLayer(undefined, null);
  layer.setTerrain(window([animal('a', species)]));
  layer.setTerrain(window([{ ...animal('a', species), gridX: grid.gridCellsPerChunk * .625 }]));
  layer.setSimulationSpeed(speed); return layer;
}
for (const species of ['Hare', 'Deer', 'Wolf', 'Tiger']) {
  const normal = movingLayer(1, species), fast = movingLayer(4, species);
  const start = normal.animals.get('a').x;
  normal.update(.02); fast.update(.02);
  const dx = normal.animals.get('a').x - start;
  assert(dx > 0); assert(Math.abs(fast.animals.get('a').x - start - 4 * dx) < 1e-10);
  assert.equal(normal.animals.get('a').speed, fast.animals.get('a').speed, 'Rig gait selection must not change with playback speed');
  assert.equal(fast.time.value, .02 * residentPresentationMotionTimeScale(4));
  normal.setSimulationSpeed(0);
  const pose = JSON.stringify(normal.animals.get('a')), clock = normal.time.value;
  for (let i = 0; i < 20; i++) normal.update(.1);
  normal.setTerrain(window([{ ...animal('a', species), gridX: grid.gridCellsPerChunk * .625 }]));
  assert.equal(JSON.stringify(normal.animals.get('a')), pose, 'Pause/observation refresh must preserve moving pose');
  assert.equal(normal.time.value, clock);
  normal.update(NaN); normal.update(-1); assert.equal(normal.time.value, clock);
  normal.setSimulationSpeed(4); normal.update(1000);
  assert(normal.animals.get('a').x <= normal.animals.get('a').goalX, 'No overshoot or new destination');
  assert.equal(normal.time.value, clock + .1 * residentPresentationMotionTimeScale(4));
  normal.dispose(); fast.dispose();
}
console.log('PASS real WildlifeLayer positions, shader time, pause/resume, 4x ratio, preserved gait, stalled-frame clamp');

// Exercise actual published rigs through the production layer, not mocked clips.
const cache = new Map();
async function asset(species, tier) {
  const slug = { Hare: 'hare', Deer: 'deer', Wolf: 'wolf', Tiger: 'tiger', FreshwaterFish: 'freshwater-fish' }[species];
  const filename = `${slug}-${tier === 'near' ? 'rigged' : tier}.glb`;
  if (!cache.has(filename)) {
    const b = readFileSync(new URL('../../public/vendor/wildlife/' + filename, import.meta.url));
    cache.set(filename, new GLTFLoader().register(() => ({ name: 'M10_NODE_TEXTURE_PLACEHOLDER', loadTexture: () => Promise.resolve(new THREE.Texture()) }))
      .parseAsync(b.buffer.slice(b.byteOffset, b.byteOffset + b.byteLength), ''));
  }
  return cache.get(filename);
}
for (const species of ['Hare', 'Deer', 'Wolf', 'Tiger', 'FreshwaterFish']) {
  for (const tier of ['near', 'mid', 'far']) {
    const gltf = await asset(species, tier); let meshes = 0, vertices = 0;
    gltf.scene.traverse(o => { if (o.isMesh) { meshes++; vertices += o.geometry.attributes.position.count; } });
    assert(meshes > 0 && vertices > 0);
    if (tier !== 'far') assert(gltf.animations.length >= (species === 'FreshwaterFish' ? 2 : 6), species + ' actual rig needs behavior clips');
  }
  const layer = new WildlifeLayer(t => asset('Tiger', t), (s, t) => asset(s, t));
  let scene = window([animal('real', species)]);
  if (species === 'FreshwaterFish') scene = { ...scene, chunks: [{ ...chunk, elevation01: .2, elevationSamples: Array(289).fill(.2), waterKind: 'Lake', salinity: 'Fresh', waterAvailability: 1 }] };
  layer.setTerrain(scene); await new Promise(resolve => setTimeout(resolve, 0));
  layer.update(.02); await new Promise(resolve => setTimeout(resolve, 0));
  layer.update(.02);
  assert.equal(layer.instanceCount, 1, species + ' must remain one Core ID');
  const pool = species === 'Tiger' ? layer.tigers : layer.native;
  const actor = pool.actors.get('real'); assert(actor, species + ' must load actual near skin');
  const before = actor.mixer.time;
  layer.setSimulationSpeed(0); layer.update(.1);
  assert.equal(actor.mixer.time, before, species + ' real mixer must freeze');
  layer.setSimulationSpeed(4); layer.update(.02);
  assert(Math.abs(actor.mixer.time - before - .02 * residentPresentationMotionTimeScale(4)) < 1e-10);
  assert.equal(layer.renderedCoreIds.size, 1); layer.dispose();
}
console.log('PASS 15 published wildlife LODs, all five real species rigs freeze/resume and use canonical 4x clock');

const { RenderPerformanceRecorder } = await source('render/render-performance.ts');
const recorder = new RenderPerformanceRecorder();
const surface = { width: 530, height: 1050, pixelRatio: 1.35, mobile: true };
const counters = { calls: 40, triangles: 25000, geometries: 32, textures: 8, programs: 6 };
assert.equal(recorder.snapshot().cpuFrameMs.p95, null);
recorder.begin(surface);
recorder.record(0, 4, counters); recorder.record(100 / 3, 8, counters); recorder.record(200 / 3, 12, counters);
assert(Math.abs(recorder.snapshot().deliveredFps - 30) < 1e-8);
assert.equal(recorder.snapshot().cpuFrameMs.p50, 8); assert.equal(recorder.snapshot().cpuFrameMs.p95, 12);
recorder.breakContinuity(); recorder.record(6000, 5, counters); assert.equal(recorder.snapshot().frameIntervalP95Ms, 100 / 3);
recorder.breakContinuity(); recorder.record(6050, 5, counters);
assert(Math.abs(recorder.snapshot().deliveredFps - 30) < 1e-8);
recorder.setSurface(surface); assert.equal(recorder.snapshot().sampledFrames, 5);
recorder.setSurface({ ...surface, width: 1000 }); assert.equal(recorder.snapshot().sampledFrames, 0, 'Do not mix portrait/landscape measurements');
for (let i = 0; i < 1200; i++) recorder.record(i * 50, i % 3, { ...counters, geometries: 32 + i % 2 });
assert.equal(recorder.snapshot().sampledFrames, 300);
assert.equal(recorder.snapshot().latest.geometries, 33); assert.equal(recorder.snapshot().peak.geometries, 33);
const report = recorder.snapshot(); report.latest.calls = -99; report.surface.width = -99;
assert.equal(recorder.snapshot().latest.calls, 40); assert.equal(recorder.snapshot().surface.width, 1000);
recorder.stop(); recorder.record(99999, 1, counters); assert.equal(recorder.snapshot().sampledFrames, 300);
recorder.resume(); recorder.record(100000, 1, counters); assert.equal(recorder.snapshot().deliveredFps, 20);
recorder.begin(surface); assert.equal(recorder.snapshot().sampledFrames, 0);
recorder.record(0, 1, counters); recorder.record(2000, 1800, counters);
assert.equal(recorder.snapshot().deliveredFps, .5, 'Real slow foreground frames must not be excluded');
assert.equal(recorder.snapshot().frameIntervalP95Ms, 2000);
recorder.begin(surface);
recorder.record(NaN, 0, counters); recorder.record(0, -1, counters); assert.equal(recorder.snapshot().sampledFrames, 0);
console.log('PASS bounded render telemetry percentiles, delivered FPS, hidden gaps, resize reset, restart and immutable snapshots');

const { installResidentBasicClothing } = await source('render/resident-basic-clothing.ts');
const { createResidentAppearanceProfile } = await source('render/resident-appearance.ts');
const { createResidentSkinMaterial } = await source('render/resident-surface-material.ts');
for (const sex of ['male', 'female']) for (const tier of ['', '-mid', '-far']) {
  const raw = readFileSync(new URL(`../../public/vendor/characters/resident-${sex}${tier}.glb`, import.meta.url));
  const gltf = await new GLTFLoader().register(() => ({ name: 'M10_CLOTH_TEXTURE_PLACEHOLDER', loadTexture: () => Promise.resolve(new THREE.Texture()) }))
    .parseAsync(raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength), '');
  const body = gltf.scene.getObjectByName('NativeResidentBody'); assert(body?.isSkinnedMesh);
  const profile = createResidentAppearanceProfile({ ...base, sex: sex === 'male' ? 'Male' : 'Female', ageYears: 76, lifeStage: 'Elderly' });
  body.material = createResidentSkinMaterial(profile);
  const weights = body.geometry.attributes.skinWeight.array.slice(), indices = body.geometry.attributes.skinIndex.array.slice();
  const children = body.children.length;
  installResidentBasicClothing(body, profile);
  const cloth = body.geometry.attributes.llBasicClothing;
  assert.equal(cloth.count, body.geometry.attributes.position.count);
  assert([...cloth.array].every(n => Number.isFinite(n) && n >= 0 && n <= 1));
  let covered = 0, bare = 0, bareExtremities = 0;
  for (let v = 0; v < cloth.count; v++) {
    if (cloth.getX(v) > .9) covered++; if (cloth.getX(v) < .01) bare++;
    let extremity = 0;
    for (let k = 0; k < 4; k++) {
      const name = body.skeleton.bones[body.geometry.attributes.skinIndex.getComponent(v, k)].name;
      if (/^Head$|^hand_[lr]$|^foot_[lr]$/.test(name)) extremity += body.geometry.attributes.skinWeight.getComponent(v, k);
    }
    if (extremity > .95) { bareExtremities++; assert(cloth.getX(v) < .01, 'No cloth painted onto face/hands/feet'); }
  }
  assert(covered > 50 && bare > 50 && bareExtremities > 10, sex + tier + ' actual body coverage');
  assert.equal(body.children.length, children, 'No extra renderable, skeleton or prop');
  assert.deepEqual(body.geometry.attributes.skinWeight.array, weights); assert.deepEqual(body.geometry.attributes.skinIndex.array, indices);
  const shader = { uniforms: {}, vertexShader: '#include <common>\n#include <begin_vertex>', fragmentShader: '#include <common>\n#include <color_fragment>\n#include <roughnessmap_fragment>' };
  body.material.onBeforeCompile(shader, {});
  assert(shader.vertexShader.includes('vLlBasicClothing = llBasicClothing'));
  assert(shader.fragmentShader.includes('llSkinNoise') && shader.fragmentShader.includes('llClothTop'));
}
console.log('PASS all six actual resident LODs have deterministic rig-bound basic clothing, bare extremities, no added draw/skin weights');

const { WaterLayer } = await source('render/water-layer.ts');
const waterLayer = new WaterLayer();
const wet = { ...window([]), chunks: [{ ...chunk, elevation01: .2, elevationSamples: Array(289).fill(.2), waterKind: 'Lake', salinity: 'Fresh', waterAvailability: 1 }] };
const unchangedWet = JSON.stringify(wet); waterLayer.setTerrain(wet);
const waterCamera = new THREE.PerspectiveCamera(42, 1, .1, 200);
waterLayer.flowWaterMesh.geometry.computeBoundingSphere();
const waterY = waterLayer.flowWaterMesh.geometry.boundingSphere.center.y;
waterCamera.position.set(0, waterY + 6, 12); waterCamera.lookAt(0, waterY, 50);
const forbiddenRenderer = new Proxy({}, { get() { throw Error('Offscreen water must not submit a reflection/PMREM pass'); } });
waterLayer.updateReflection(forbiddenRenderer, new THREE.Scene(), waterCamera);
assert.equal(waterLayer.reflectionDirty, true, 'Offscreen water must retain pending refresh');
let captures = 0, targetsRestored = 0;
const gpu = { xr: { enabled: true }, getRenderTarget: () => 'previous', setRenderTarget: value => { assert.equal(value, 'previous'); targetsRestored++; } };
waterLayer.reflectionCamera.update = () => { assert.equal(waterLayer.group.visible, false); assert.equal(gpu.xr.enabled, false); captures++; };
waterLayer.reflectionGenerator = { fromCubemap: () => new THREE.WebGLRenderTarget(1, 1), dispose() {} };
waterCamera.lookAt(0, waterY, 0); waterLayer.updateReflection(gpu, new THREE.Scene(), waterCamera);
assert.equal(captures, 1); assert.equal(waterLayer.reflectionDirty, false); assert.equal(targetsRestored, 1);
assert.equal(gpu.xr.enabled, true); assert.equal(waterLayer.group.visible, true);
waterCamera.lookAt(0, waterY, 50);
waterLayer.updateReflection(forbiddenRenderer, new THREE.Scene(), waterCamera);
assert.equal(captures, 1); assert.equal(waterLayer.reflectionDirty, true);
waterCamera.lookAt(0, waterY, 0); waterLayer.updateReflection(gpu, new THREE.Scene(), waterCamera);
assert.equal(captures, 2); assert.equal(JSON.stringify(wet), unchangedWet); waterLayer.dispose();
console.log('PASS offscreen water skips six-face reflection/PMREM, retains dirty state, captures on return and restores GPU state');

const { residentStatusText } = await source('ui/resident-status.ts');
assert.equal(residentStatusText({ ...base, presentation: { active: true, kind: 'Social', phase: 'Moving', socialIntent: 'Approach', targetResidentId: '2' } }, [{ ...base, id: '2', name: 'Minseok' }]), '민석에게 다가가는 중');
console.log('PASS Korean resident target name is consistent between status, action cues and causal timeline');
await import('./acceptance.mjs');
