// Exercise actual production pause/resume handlers: a 45-day absence runs zero Core ticks.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import ts from 'typescript';

const root = resolve(import.meta.dirname, '../../src');
const engine = ts.transpileModule(readFileSync(resolve(root, 'observer-engine.ts'), 'utf8'), {
  compilerOptions: { module: ts.ModuleKind.ESNext, target: ts.ScriptTarget.ES2020 },
}).outputText;
const start = engine.indexOf('function onWorldVisibility()');
const end = engine.indexOf('let angle ', start);
assert(start >= 0 && end > start, 'production page lifecycle handlers must exist');
assert(!engine.includes('resumeOfflineWorld('));
assert(!engine.includes('elapsedOfflineMinutes('));
assert(engine.includes('worldSession.restoreWorld(saved.snapshotBase64)'),
  'a saved Core snapshot must still restore exactly');
const code = engine.slice(start, end);
const documentStub = { visibilityState: 'visible' };
let now = 1790000000000;
const calls = [], checkpointTimes = [];
const world = {
  runMinutes() { throw Error('A hidden/reopened world must never fast-forward!'); },
};
const clockStub = {
  stop() { calls.push('stop'); },
  start() { calls.push('start'); },
  resetAccumulator() { calls.push('reset'); },
};
const factory = new Function(
  'document', 'Date', 'worldSession', 'simulationClock', 'saveCheckpoint',
  `let stopped = false, fastForwardRunning = false;
  ${code}
  return { onWorldVisibility, onWorldPageHide, onWorldPageShow };`,
);
const lifecycle = factory(documentStub, { now: () => now }, world, clockStub,
  time => { checkpointTimes.push(time); return Promise.resolve(); });
documentStub.visibilityState = 'hidden';
lifecycle.onWorldVisibility();
assert.deepEqual(calls, ['stop']);
assert.deepEqual(checkpointTimes, [1790000000000]);
now += 45 * 86400000;
documentStub.visibilityState = 'visible';
lifecycle.onWorldVisibility();
assert.deepEqual(calls, ['stop', 'reset', 'start'],
  'visibility return must discard the paused accumulator');
assert.deepEqual(checkpointTimes, [1790000000000],
  'resume cannot fabricate a progressed checkpoint');
lifecycle.onWorldPageHide();
assert.equal(calls.at(-1), 'stop');
assert.equal(checkpointTimes.at(-1), now);
lifecycle.onWorldPageShow();
assert.deepEqual(calls.slice(-2), ['reset', 'start'],
  'BFCache pageshow must restart at the stored Core minute without replay');
assert.equal(checkpointTimes.length, 2);


// Independently exercise production SimulationClock under throttled mobile timers.
const source = ts.transpileModule(
  readFileSync(resolve(root, 'runtime/simulation-clock.ts'), 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
  },
).outputText;
const oldDocument = globalThis.document;
const oldWindow = globalThis.window;
const oldNow = Date.now;
const events = new Map(), timers = new Map();
let id = 0, advanced = 0;
try {
  globalThis.document = {
    visibilityState: 'visible',
    addEventListener(name, handler) { events.set(name, handler); },
    removeEventListener(name) { events.delete(name); },
  };
  globalThis.window = {
    setInterval(callback) { const n = ++id; timers.set(n, callback); return n; },
    clearInterval(n) { timers.delete(n); },
  };
  Date.now = () => now;
  const exports = {};
  new Function('require', 'exports', source)(name => {
    assert.equal(name, './lifelens-contract');
    return {
      REAL_MS_PER_SIMULATION_MINUTE_AT_1X: 100,
      SIMULATION_TIME_CONTRACT: {
        tickIntervalMs: 10, refreshIntervalMs: 20,
        maxCatchupMs: 9000000000, maxAdvanceMinutesPerTick: 100000,
        defaultSpeed: 1,
      },
      normalizeSimulationSpeed: speed => speed,
    };
  }, exports);
  const clock = new exports.SimulationClock({
    onAdvance: minutes => { advanced += minutes; },
    onRefresh: () => {},
  });
  clock.start();
  const tick = [...timers.values()][0];
  now += 1000;
  tick();
  assert.equal(advanced, 10, 'foreground should advance Core normally');
  globalThis.document.visibilityState = 'hidden';
  now += 30 * 86400000;
  tick();
  assert.equal(advanced, 10, 'hidden timer callbacks must not advance Core');
  globalThis.document.visibilityState = 'visible';
  events.get('visibilitychange')?.();
  tick();
  assert.equal(advanced, 10, 'reopening must drop all prior wall time');
  now += 500;
  tick();
  assert.equal(advanced, 15, 'subsequent foreground ticks run normally');
  clock.stop();
  assert.equal(timers.size, 0);
} finally {
  Date.now = oldNow;
  if (oldDocument === undefined) delete globalThis.document;
  else globalThis.document = oldDocument;
  if (oldWindow === undefined) delete globalThis.window;
  else globalThis.window = oldWindow;
}
console.log('PASS paused world: no implicit replay after page exit or hidden tab');
