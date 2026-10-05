// Compare independent instances of the production WASM, never a duplicate JS simulation.
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync, mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { resolve, join } from 'node:path';
import { pathToFileURL } from 'node:url';
import { source } from './compile.mjs';
const { ShadowObserver, SHADOW_BUDGET } = await source('cognition/shadow-observer.ts');
const { LifeLensCoreBridge } = await source('runtime/core-bridge.ts');
const generated = resolve(import.meta.dirname, '../../../Clients/Web/generated');
const temporary = mkdtempSync(join(tmpdir(), 'lifelens-shadow-wasm-'));
try {
  // The runtime is compiled for browsers. Supplying bytes exercises identical WASM
  // without fetching over the network or modifying its production build flags.
  globalThis.window = {};
  globalThis.document = { currentScript: null };
  const modulePath = join(temporary, 'lifelens_core.mjs');
  writeFileSync(modulePath, readFileSync(join(generated, 'lifelens_core.js')));
  const { default: createModule } = await import(pathToFileURL(modulePath));
  const module = await createModule({ wasmBinary: readFileSync(join(generated, 'lifelens_core.wasm')) });
  for (const seed of ['4242001', '874213954']) {
    const off = new module.LifeLensWebClient();
    const on = new module.LifeLensWebClient();
    const bridge = new LifeLensCoreBridge(on);
    let wall = 0, samples = 0;
    const observer = new ShadowObserver(() => wall);
    observer.attach(bridge);
    observer.setProvider({ name: 'deterministic-free-local-fixture', reason: async request => {
      samples++;
      assert(Object.keys(request.personality).length > 0, 'context comes from Core personality');
      return { actor: request.actor, intent: 'ImproveFoodSecurity', priority: .6, targetResident: null, rationale: 'Core의 현재 욕구를 참고했습니다.' };
    } });
    observer.setEnabled(true);
    try {
      assert.equal(off.newGame(seed, '', 3), true); assert.equal(on.newGame(seed, '', 3), true);
      const queries = [
        ['worldOverviewJson', []], ['residentsJson', []], ['residentRuntimeJson', []],
        ['civilizationWorldJson', [32]], ['worldObjectsJson', []],
        ['dynamicEnvironmentJson', [0, 0]], ['recentSocialEventsJson', [32]],
        ['humanTracesWindowJson', [0, 0, 3]], ['terrainWindowJson', [0, 0, 3]],
      ];
      for (let minute = 0; minute <= 1440; minute += 60) {
        if (minute) { off.runMinutes(60); on.runMinutes(60); }
        const residents = bridge.residentRuntime().residents ?? [];
        const actor = residents.find(resident => resident.alive === true && bridge.cognitiveRequest(resident.id, 'Reflection'));
        assert(actor, 'Core must provide a living cognitive actor');
        observer.observe(bridge.worldOverview(), residents, actor.id);
        await observer.sample(actor.id);
        assert(observer.getSnapshot().records.some(record => record.outcome.status === 'validated'));
        // OFF never requests cognition. ON samples before comparing all available
        // Core outputs, including needs, Utility-selected actions, movement,
        // inventory, relationships, emotions, memories/beliefs and civilization.
        for (const [name, args] of queries) {
          assert.equal(typeof off[name], 'function', `missing production query ${name}`);
          assert.equal(on[name](...args), off[name](...args), `${seed} minute ${minute}: Shadow changed ${name}`);
        }
        wall += SHADOW_BUDGET.cadenceMs;
      }
      assert.equal(samples, 25);
      console.log(`PASS real Core WASM ON/OFF seed=${seed}: 1440 minutes, 25 proposals, 9 exact JSON outputs at 25 checkpoints`);
    } finally { observer.dispose(); off.delete(); on.delete(); }
  }
} finally { rmSync(temporary, { recursive: true, force: true }); }
