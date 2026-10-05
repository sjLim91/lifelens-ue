import assert from 'node:assert/strict';
import { mkdtempSync, mkdirSync, readFileSync, writeFileSync, rmSync } from 'node:fs';
import { stripTypeScriptTypes } from 'node:module';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const root = resolve(import.meta.dirname, '../../src');
const temporary = mkdtempSync(join(tmpdir(), 'lifelens-lifecycle-'));
try {
  writeFileSync(join(temporary, 'package.json'), '{"type":"module"}');
  const compiled = new Set();
  function compile(path) {
    const target = join(temporary, path.replace(/\.ts$/, '.js'));
    if (compiled.has(path)) return target;
    compiled.add(path);
    const source = stripTypeScriptTypes(readFileSync(join(root, path), 'utf8'), { mode: 'transform' });
    const code = source.replace(/(from\s+['"])(\.[^'"]+)(['"])/g, (_, prefix, dependency, suffix) => {
      const resolved = resolve(root, dirname(path), dependency + '.ts');
      compile(resolved.slice(root.length + 1));
      return prefix + dependency + '.js' + suffix;
    });
    mkdirSync(dirname(target), { recursive: true });
    writeFileSync(target, code);
    return target;
  }
  const { observerStore } = await import(pathToFileURL(compile('state/observer-store.ts')));
  observerStore.update({ civilization: { available: true, settlements: [{ id: '7' }] } });
  observerStore.selectSettlement('7');
  assert.equal(observerStore.getSnapshot().selectedSettlementId, '7');
  observerStore.resetWorld();
  assert.equal(observerStore.getSnapshot().selectedSettlementId, null, 'a new world cannot retain old settlement selection');

  // Execute the production new-world orchestration with injected runtime and
  // renderer dependencies. No browser/WASM boot or duplicate reset logic.
  const engine = stripTypeScriptTypes(readFileSync(join(root, 'observer-engine.ts'), 'utf8'), { mode: 'transform' });
  const begin = engine.indexOf('function createWorld(');
  const end = engine.indexOf('function selectResident(', begin);
  assert(begin >= 0 && end > begin);
  function fixture(speed, running = false) {
    observerStore.resetWorld();
    observerStore.update({ simulationSpeed: speed });
    const calls = [];
    const clock = {
      speed,
      resetAccumulator() { calls.push('reset-clock'); },
      setSpeed(value) { this.speed = value; calls.push('speed-clock'); },
    };
    const renderer = {
      speed,
      selectedSettlement: '7',
      resetSocialEvents() {}, setSelectedResident() {}, setSelectedHumanTrace() {},
      setSelectedSettlement(value) { this.selectedSettlement = value; },
      setSimulationSpeed(value) { this.speed = value; calls.push('speed-renderer'); },
    };
    const dependencies = {
      worldSession: { createWorld: seed => { calls.push('world:' + seed); } },
      fastForwardRunning: running,
      generateWorldSeed: () => '123',
      characterLayer: null,
      simulationClock: clock,
      observerStore,
      threeWorldRenderer: renderer,
      refresh: () => { calls.push('refresh'); },
    };
    const createWorld = new Function(...Object.keys(dependencies),
      'let centerX,centerY,followResidents,localPanX,localPanZ,residentSnapshot,terrain;\n'
      + engine.slice(begin, end) + '\nreturn createWorld;')(...Object.values(dependencies));
    return { createWorld, clock, renderer, calls };
  }
  for (const speed of [0, 4]) {
    const f = fixture(speed);
    f.createWorld('42');
    assert.equal(f.clock.speed, observerStore.getSnapshot().simulationSpeed, 'clock must match displayed new-world speed');
    assert.equal(f.renderer.speed, f.clock.speed, 'resident animation speed must match simulation');
    assert.equal(f.renderer.selectedSettlement, null);
    assert(f.calls.indexOf('reset-clock') < f.calls.indexOf('speed-clock'), 'old accumulated time must be discarded before changing speed');
  }
  const active = fixture(4, true);
  active.createWorld('43');
  assert.deepEqual(active.calls, [], 'new-world actions must be ignored during yielded fast-forward work');
  assert.equal(observerStore.getSnapshot().simulationSpeed, 4);
  console.log('PASS world lifecycle: selection reset, clock/render speed alignment, fast-forward isolation');
} finally {
  rmSync(temporary, { recursive: true, force: true });
}
