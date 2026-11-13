import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { source } from '../procedural-environment/compile.mjs';

const { residentPersonalityFacts, PERSONALITY_LABELS, goalConditionText } = await source('ui/resident-observation.ts');
const { CORE_MATERIAL_NAMES } = await source('runtime/generated-core-contract.ts');
const { KOREAN_MATERIAL_LABELS } = await source('localization/korean.ts');
const personality = Object.fromEntries(Object.keys(PERSONALITY_LABELS).map((key, i) => [key, i / 14]));
const resident = { id: '18446744073709551615', name: 'Jiho', personality };
const before = JSON.stringify(resident);
assert.equal(residentPersonalityFacts(resident).length, 14);
assert.equal(JSON.stringify(resident), before);
assert.equal(residentPersonalityFacts({ ...resident, personality: { curiosity: NaN, patience: -1, empathy: 2 } }).length, 0);
const block = readFileSync(new URL('../../../Source/LifeLensCore/include/lifelens/Civilization.h', import.meta.url), 'utf8')
  .match(/enum class MaterialKind\s*\{([\s\S]*?)\};/)[1].replace(/\/\/[^\n]*/g, '').split(',').map(s => s.trim()).filter(Boolean);
assert.deepEqual(Object.values(CORE_MATERIAL_NAMES), block);
for (const [code, material] of Object.entries(CORE_MATERIAL_NAMES)) {
  assert(KOREAN_MATERIAL_LABELS[material], 'Core material missing Korean label: ' + material);
  assert(goalConditionText({ metric: 'ActorRawMaterialCount', materialCode: Number(code), threshold: 2, operator: 'AtLeast' }, []).includes(KOREAN_MATERIAL_LABELS[material]));
}
console.log('PASS M10-02: all 14 actual personality fields and every Core material goal have Korean labels; source data unchanged');

const { residentLocalCraft } = await source('render/resident-production-context.ts');
const { resolveResidentSemanticMotion } = await source('render/resident-semantic-motion.ts');
const craft = { active: true, kind: 'Civilization', phase: 'Interacting', civilizationIntent: 'Craft',
  civilizationTechnique: 'DiggingStick', facilityId: '0', facilityAction: 'None', hasTargetGrid: false,
  contextActionToken: '18446744073709551615' };
assert(residentLocalCraft(craft));
assert.equal(resolveResidentSemanticMotion(craft, { moving: false, nearbyResident: false }), 'craft');
for (const bad of [{ active: false }, { phase: 'Moving' }, { civilizationTechnique: 'None' },
  { facilityId: '5' }, { facilityAction: 'Repair' }, { hasTargetGrid: true }, { contextActionToken: '0' },
  { contextActionToken: '18446744073709551616' }]) assert(!residentLocalCraft({ ...craft, ...bad }));
console.log('PASS M10-05: real non-spatial Core hand craft is visible; invalid tokens, remote targets and facility jobs cannot claim it');

class Target {
  listeners = new Map(); style = {}; clientHeight = 400; captures = new Set();
  addEventListener(name, fn) { if (!this.listeners.has(name)) this.listeners.set(name, new Set()); this.listeners.get(name).add(fn); }
  removeEventListener(name, fn) { this.listeners.get(name)?.delete(fn); }
  setPointerCapture(id) { this.captures.add(id); }
  hasPointerCapture(id) { return this.captures.has(id); }
  releasePointerCapture(id) { this.captures.delete(id); }
  emit(name, event = {}) {
    for (const fn of this.listeners.get(name) ?? []) fn(event);
    if (['pointerup', 'pointercancel', 'lostpointercapture'].includes(name)) this.captures.delete(event.pointerId);
  }
  count() { return [...this.listeners.values()].reduce((n, set) => n + set.size, 0); }
}
const oldWindow = globalThis.window, oldDocument = globalThis.document;
const win = new Target(), doc = new Target(); doc.hidden = false;
globalThis.window = win; globalThis.document = doc;
try {
  const { GlobeInput } = await source('input/globe-input.ts');
  const canvas = new Target(), rotations = [], zooms = [], selected = [];
  const input = new GlobeInput(canvas, { rotate: (...v) => rotations.push(v), zoom: v => zooms.push(v), select: (...v) => selected.push(v) });
  const emit = (name, id, x, y) => canvas.emit(name, { pointerId: id, clientX: x, clientY: y, pointerType: 'touch' });
  emit('pointerdown', 1, 10, 20); emit('pointerup', 1, 10, 20); assert.deepEqual(selected, [[10, 20]]);
  emit('pointerdown', 1, 10, 20); emit('pointermove', 1, 50, 25); emit('pointerup', 1, 50, 25);
  assert.deepEqual(rotations.at(-1), [40, 5]); assert.equal(selected.length, 1);
  emit('pointerdown', 1, 0, 0); emit('pointerdown', 2, 100, 0); emit('pointermove', 2, 120, 0);
  assert(Math.abs(zooms.at(-1) + .18) < 1e-10);
  emit('pointerup', 2, 120, 0); emit('pointermove', 1, 10, 5);
  assert.deepEqual(rotations.at(-1), [10, 5]);
  emit('pointerup', 1, 10, 5); assert.equal(selected.length, 1, 'pinch must never select a region');
  emit('pointerdown', 1, 0, 0); emit('pointerdown', 2, 100, 0); emit('pointerdown', 3, 300, 0);
  const prior = zooms.length; emit('pointermove', 3, 400, 0); assert.equal(zooms.length, prior);
  emit('pointerup', 3, 400, 0); emit('pointermove', 2, 110, 0); assert(Math.abs(zooms.at(-1) + .09) < 1e-10);
  canvas.emit('lostpointercapture', { pointerId: 2 }); emit('pointerup', 1, 0, 0); assert.equal(selected.length, 1);
  emit('pointerdown', 1, 0, 0); doc.hidden = true; doc.emit('visibilitychange'); doc.hidden = false;
  const n = rotations.length; emit('pointermove', 1, 90, 90); assert.equal(rotations.length, n);
  emit('pointerdown', 1, 0, 0); win.emit('blur'); emit('pointerup', 1, 0, 0); assert.equal(selected.length, 1);
  let prevented = false; canvas.emit('wheel', { deltaY: 1, deltaMode: 1, preventDefault: () => { prevented = true; } });
  assert(prevented); assert.equal(zooms.at(-1), 16 * .0035);
  input.dispose(); assert.equal(canvas.count() + win.count() + doc.count(), 0); assert.equal(canvas.captures.size, 0);
  console.log('PASS M10-01: globe orbit, pinch, 2→1/3→2 transitions, cancel/capture loss, hidden/blur, wheel and complete teardown');
} finally {
  if (oldWindow === undefined) delete globalThis.window; else globalThis.window = oldWindow;
  if (oldDocument === undefined) delete globalThis.document; else globalThis.document = oldDocument;
}
