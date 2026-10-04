import assert from 'node:assert/strict';
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { stripTypeScriptTypes } from 'node:module';
import { dirname, resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import * as THREE from 'three';

const root = resolve(import.meta.dirname, '../..');
const output = resolve(import.meta.dirname, 'review/src');
const compiled = new Set();
function compile(path) {
  const target = resolve(output, path.replace(/\.ts$/, '.mjs'));
  if (compiled.has(path)) return target;
  compiled.add(path);
  let code = stripTypeScriptTypes(readFileSync(resolve(root, 'src', path), 'utf8'), { mode: 'transform' });
  code = code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g, (_, prefix, dependency, suffix) => {
    compile(resolve(dirname(path), dependency + '.ts').slice(resolve('.').length + 1));
    return prefix + dependency + '.mjs' + suffix;
  });
  mkdirSync(dirname(target), { recursive: true });
  writeFileSync(target, code);
  return target;
}
const source = async path => import(pathToFileURL(compile(path)));
const { SOCIAL_EVENT_PRESENTATION_CONTRACT: C } = await source('runtime/lifelens-contract.ts');
const { SocialEventPresentation, classifySocialEvent } = await source('render/social-event-presentation.ts');
const { SocialEventLayer, socialEventStrokes } = await source('render/social-event-layer.ts');
const { residentSocialCuePairs } = await source('render/resident-social-cues.ts');
const types = ['PositiveInteraction', 'Help', 'Comfort', 'Conflict', 'Betrayal', 'Rejection', 'Apology', 'Intimacy', 'Commitment'];
const event = (extra = {}) => ({
  sequence: '1', actorId: 'a', targetId: 'b', type: 'PositiveInteraction',
  intensity: .8, importance: .5, minute: 100, where: 'site',
  presentationLevel: 'Everyday', successful: true, ...extra,
});
const payload = events => ({ available: true, events });
let now = 0;
const residents = new Map([['a', { x: 0, y: 0, z: 0 }], ['b', { x: 2, y: 0, z: 0 }]]);
const anchor = id => residents.get(id) ?? null;
const timeline = () => new SocialEventPresentation(() => now);
let passed = 0;
function test(name, run) { now = 0; run(); passed++; console.log('PASS', name); }
test('all nine exact types and unknown future enum safely skipped', () => {
  for (const type of types) assert.equal(classifySocialEvent(event({ type })).kind, type);
  for (const type of ['FutureType', '__proto__', 'toString']) assert.equal(classifySocialEvent(event({ type })), null);
  assert.notEqual(classifySocialEvent(event({ type: 'Conflict' })).color, classifySocialEvent(event({ type: 'Intimacy' })).color);
});
test('same sequence ten refreshes triggers once; new sequence independently admitted', () => {
  const t = timeline();
  for (let i = 0; i < 10; i++) t.observe(payload([event()]), 100, anchor);
  assert.equal(t.activeCues.length, 1);
  t.observe(payload([event({ sequence: '2' })]), 100, anchor);
  assert.equal(t.activeCues.length, 2);
  now = 4; t.update(anchor); t.observe(payload([event(), event({ sequence: '2' })]), 100, anchor);
  assert.equal(t.activeCues.length, 0);
});
test('compatibility identity preserves actor target type minute and where', () => {
  const t = timeline();
  for (let i = 0; i < 10; i++) t.observe(payload([event({ sequence: '0' })]), 100, anchor);
  assert.equal(t.activeCues.length, 1);
  t.observe(payload([event({ sequence: '', where: 'other' })]), 100, anchor);
  assert.equal(t.activeCues.length, 2);
});
test('initial history drops three/five-hour-old events and caps fresh initial burst', () => {
  const t = timeline();
  t.observe(payload([event({ sequence: '1', minute: 0 }), event({ sequence: '2', minute: 120 }),
    ...Array.from({ length: 20 }, (_, i) => event({ sequence: String(i + 3), minute: 300 }))]), 300, anchor);
  assert.equal(t.activeCues.length, C.maxInitialCues);
  assert(t.activeCues.every(c => c.minute === 300));
  assert.deepEqual(t.activeCues.map(c => c.sequence), [22n, 21n]);
});
test('replay window derived from maximum refresh cadence; future and invalid minutes skipped', () => {
  const t = timeline(); t.observe(payload([]), 100, anchor);
  t.observe(payload([event({ sequence: '1', minute: 100 - C.replayWindowMinutes }),
    event({ sequence: '2', minute: 99 - C.replayWindowMinutes }), event({ sequence: '3', minute: 101 }),
    event({ sequence: '4', minute: NaN })]), 100, anchor);
  assert.deepEqual(t.activeCues.map(c => c.sequence), [1n]);
});
test('twenty events choose Important then Meaningful then Everyday; importance and recency break ties', () => {
  const t = timeline(); t.observe(payload([]), 100, anchor);
  const events = Array.from({ length: 20 }, (_, i) => event({
    sequence: String(i + 1), presentationLevel: i < 5 ? 'Important' : i < 10 ? 'Meaningful' : 'Everyday',
    importance: i / 20,
  }));
  t.observe(payload(events.reverse()), 100, anchor);
  assert.equal(t.activeCues.length, C.maxActiveCues);
  assert.deepEqual(t.activeCues.map(c => c.sequence), [5n, 4n, 3n, 2n, 1n, 10n, 9n, 8n]);
  t.observe(payload([event({ sequence: '21', presentationLevel: 'Important', importance: 1, type: 'Betrayal' })]), 100, anchor);
  assert.equal(t.activeCues[0].visual.kind, 'Betrayal');
});
test('missing dead unpositioned and invalid anchors skip without allocating a cue', () => {
  for (const missing of ['a', 'b']) {
    const t = timeline(); t.observe(payload([event()]), 100, id => id === missing ? null : anchor(id));
    assert.equal(t.activeCues.length, 0);
  }
  const t = timeline(); t.observe(payload([event()]), 100, () => ({ x: NaN, y: 0, z: 0 }));
  assert.equal(t.activeCues.length, 0);
});
test('distance and same-resident guards prohibit giant connections', () => {
  const t = timeline();
  t.observe(payload([event()]), 100, id => id === 'b' ? { x: 100, y: 0, z: 0 } : anchor(id));
  assert.equal(t.activeCues.length, 0);
  t.observe(payload([event({ sequence: '2', actorId: 'b' })]), 100, anchor);
  assert.equal(t.activeCues.length, 0);
});
test('snapshots do not follow movement; origin shifts preserve the event site; deletion clears immediately', () => {
  const t = timeline(); t.observe(payload([event()]), 100, anchor);
  const before = { ...t.activeCues[0].actor };
  residents.get('a').x = 1;
  t.update(anchor); assert.deepEqual(t.activeCues[0].actor, before);
  t.rebase(-8, 16);
  assert.equal(t.activeCues[0].actor.x, before.x - 8);
  assert.equal(t.activeCues[0].target.z, 16);
  t.update(id => id === 'a' ? null : anchor(id)); assert.equal(t.activeCues.length, 0);
  residents.get('a').x = 0;
});
test('bounded wall-clock lifetime expires even during pause/background and ignores simulation speed', () => {
  const t = timeline(); t.observe(payload([event()]), 100, anchor);
  now = .6; t.observe(payload([event()]), 10000, anchor); assert.equal(t.activeCues.length, 1);
  now = 100; t.update(anchor); assert.equal(t.activeCues.length, 0);
  t.reset(); now = 0; t.observe(payload([event()]), 100, anchor); assert.equal(t.activeCues.length, 1);
});
test('importance intensity presentation level and unsuccessful modifier preserve exact semantics', () => {
  const ordinary = classifySocialEvent(event());
  const important = classifySocialEvent(event({ type: 'Betrayal', presentationLevel: 'Important', importance: 1 }));
  assert(important.durationSeconds > ordinary.durationSeconds);
  const failed = classifySocialEvent(event({ successful: false }));
  assert.equal(failed.kind, ordinary.kind); assert(failed.opacity < ordinary.opacity);
  assert(failed.durationSeconds < ordinary.durationSeconds);
  assert(classifySocialEvent(event({ intensity: 1 })).scale > classifySocialEvent(event({ intensity: 0 })).scale);
});
test('no events means no effects regardless of relationship/proximity; unavailable payload fails closed', () => {
  const t = timeline(); t.observe(payload([]), 100, anchor); assert.equal(t.activeCues.length, 0);
  t.observe({ available: false, events: [event()] }, 100, anchor); assert.equal(t.activeCues.length, 0);
});
test('all nine visual geometries differ and fit fixed segment budget', () => {
  const t = timeline();
  const geometries = new Set();
  for (const type of types) {
    t.reset(); t.observe(payload([event({ type })]), 100, anchor);
    const strokes = []; socialEventStrokes(t.activeCues[0], .6, (...s) => strokes.push(s));
    assert(strokes.length > 0 && strokes.length <= C.segmentsPerCue);
    assert(strokes.flat().every(Number.isFinite));
    geometries.add(JSON.stringify(strokes));
  }
  assert.equal(geometries.size, 9);
});
test('Help travels actor to target and Rejection chevron reverses toward actor', () => {
  const t = timeline();
  t.observe(payload([event({ type: 'Help' })]), 100, anchor);
  const center = progress => {
    const strokes = []; socialEventStrokes(t.activeCues[0], progress, (...s) => strokes.push(s));
    return strokes.reduce((sum, s) => sum + s[0], 0) / strokes.length;
  };
  assert(center(.6) > center(.1));
  t.reset(); t.observe(payload([event({ type: 'Rejection' })]), 100, anchor);
  for (const [progress, sign] of [[.2, 1], [.65, -1]]) {
    const strokes = []; socialEventStrokes(t.activeCues[0], progress, (...s) => strokes.push(s));
    assert.equal(Math.sign(strokes[0][0] - strokes[0][3]), sign);
  }
});
test('ongoing Social Comfort Repair Teaching Parenting contract is unchanged', () => {
  for (const [kind, socialIntent, expected] of [
    ['Social', undefined, 'Social'], ['Social', 'Comfort', 'Comfort'], ['Social', 'Repair', 'Repair'],
    ['KnowledgeTeaching', undefined, 'KnowledgeTeaching'], ['Parenting', undefined, 'Parenting'],
  ]) {
    const pairs = residentSocialCuePairs([{ id: 'a', presentation: {
      active: true, phase: 'Interacting', kind, socialIntent, targetResidentId: 'b',
    } }, { id: 'b' }]);
    assert.equal(pairs.length, 1); assert.equal(pairs[0].kind, expected);
  }
});
test('1000 refreshes retain one mesh geometry material and bounded active/seen state; dispose releases', () => {
  const l = new SocialEventLayer(anchor, () => now);
  const mesh = l.group.children[0], geometry = mesh.geometry, material = mesh.material;
  const camera = new THREE.PerspectiveCamera(); camera.position.set(4, 8, 10);
  let geometryDisposed = 0, materialDisposed = 0;
  geometry.addEventListener('dispose', () => geometryDisposed++);
  material.addEventListener('dispose', () => materialDisposed++);
  l.observe(payload([]), 100);
  for (let i = 1; i <= 1000; i++) {
    now += .02; l.observe(payload([event({ sequence: String(i) })]), 100); l.update(camera);
    assert.equal(l.group.children.length, 1); assert.equal(mesh.geometry, geometry); assert.equal(mesh.material, material);
    assert(l.presentation.activeCues.length <= C.maxActiveCues);
    assert(l.presentation.seenCount <= C.maxSeenEvents);
    assert(geometry.drawRange.count <= C.maxActiveCues * C.segmentsPerCue * 6);
  }
  assert.equal(l.presentation.seenCount, C.maxSeenEvents);
  now = 100; l.update(camera);
  l.observe(payload([event({ sequence: '1' })]), 100); assert.equal(l.presentation.activeCues.length, 0);
  l.dispose(); l.dispose();
  assert.equal(geometryDisposed, 1); assert.equal(materialDisposed, 1); assert.equal(l.group.children.length, 0);
  assert.equal(l.presentation.seenCount, 0);
});
test('production data flow and living rendered anchor guard remain read only', () => {
  const resident = readFileSync(resolve(root, 'src/render/resident-world-layer.ts'), 'utf8');
  assert(resident.includes('resident.alive !== false && resident.hasPosition'));
  assert(resident.includes('actor.current.x, y: actor.current.y, z: actor.current.z'));
  assert(!readFileSync(resolve(root, 'src/render/social-event-layer.ts'), 'utf8').includes('core-bridge'));
  assert(readFileSync(resolve(root, 'src/observer-engine.ts'), 'utf8').includes('snapshot.socialEvents,'));
});
console.log(`Social events: ${passed} tests passed; 1 mesh / 1 geometry / 1 material / at most 1 draw call`);
