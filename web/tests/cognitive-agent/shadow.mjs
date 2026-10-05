import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { source } from './compile.mjs';
const { ShadowObserver, SHADOW_BUDGET } = await source('cognition/shadow-observer.ts');
const { CognitionScheduler } = await source('cognition/cognition-scheduler.ts');
const { LocalCognitionProvider } = await source('cognition/local-cognition-provider.ts');
const { parseCognitiveProposal } = await source('cognition/cognitive-contract.ts');
const tick = () => new Promise(resolve => setTimeout(resolve, 0));
const proposal = request => ({ actor: request.actor, intent: request.allowedIntents[0], priority: .6, targetResident: null, rationale: '현재 욕구와 기억을 확인했습니다.' });
const request = (actor, minute) => ({ actor, minute, trigger: 'Reflection', needs: { hunger: .6 }, personality: { empathy: .8 }, emotion: { joy: .4 }, memories: [], beliefs: [], relationships: [], allowedIntents: ['ImproveFoodSecurity'] });
let passed = 0;
async function test(name, fn) { await fn(); console.log('PASS', name); passed++; }
function fixture(provider = { name: 'free-local-test', reason: async req => proposal(req) }, timeoutMs = 20) {
  let wall = 0;
  const world = { worldSeed: '42', populationSeed: '21', generationVersion: 3, minute: 100 };
  const residents = [{ id: '1', name: '가람', alive: true, activityKind: 'Physical', physicalGoal: 'Eat' }, { id: '2', name: '누리', alive: true }];
  let reads = 0;
  const source = {
    worldOverview: () => ({ ...world }),
    residentRuntime: () => ({ residents: structuredClone(residents) }),
    cognitiveRequest: actor => { reads++; return request(actor, world.minute); },
  };
  const controller = new ShadowObserver(() => wall, timeoutMs);
  controller.attach(source);
  controller.setProvider(provider);
  const observe = selected => controller.observe(world, residents, selected ?? '1');
  observe();
  return { controller, world, residents, source, observe, reads: () => reads,
    advance: () => { wall += SHADOW_BUDGET.cadenceMs; },
    enable: () => controller.setEnabled(true),
  };
}
await test('Shadow OFF/no provider leaves Core running; ON reads only Core capabilities', async () => {
  const f = fixture();
  const before = JSON.stringify([f.world, f.residents]);
  await f.controller.sample('1'); assert.equal(f.reads(), 0);
  f.enable(); await f.controller.sample('1');
  assert.equal(f.controller.getSnapshot().records[0].outcome.status, 'validated');
  assert.equal(JSON.stringify([f.world, f.residents]), before);
  assert.deepEqual(Object.keys(f.source).sort(), ['cognitiveRequest', 'residentRuntime', 'worldOverview']);
  f.controller.dispose();
  const unavailable = new ShadowObserver(); unavailable.attach(f.source);
  unavailable.setEnabled(true); assert.equal(unavailable.getSnapshot().enabled, false);
  f.world.minute++; assert.equal(f.world.minute, 101); unavailable.dispose();
});
await test('invalid JSON/schema/extra properties fail closed with observable reject reasons', async () => {
  for (const [content, reason] of [['{', 'invalid_json'], [JSON.stringify({ intent: 'InventReality' }), 'invalid_schema']]) {
    const p = new LocalCognitionProvider({ baseUrl: 'http://localhost:8080', model: 'free' }, async () => new Response(JSON.stringify({ choices: [{ message: { content } }] })));
    const f = fixture(p); f.enable(); await f.controller.sample('1');
    const result = f.controller.getSnapshot().records[0].outcome;
    assert.equal(result.status, 'rejected'); assert.equal(result.reason, reason); assert.equal(result.proposal, null); f.controller.dispose();
  }
  const req = request('1', 100);
  assert.throws(() => parseCognitiveProposal({ ...proposal(req), inventedTruth: true }, req));
});
await test('timeout settles an abort-ignoring provider; provider exceptions are optional failures', async () => {
  for (const [reason, provider] of [
    ['timeout', { name: 'local', reason: () => new Promise(() => {}) }],
    ['provider_error', { name: 'local', reason: () => { throw Error('offline'); } }],
    ['no_proposal', { name: 'local', reason: async () => null }],
  ]) {
    const f = fixture(provider, 5); f.enable(); await f.controller.sample('1');
    assert.equal(f.controller.getSnapshot().records[0].outcome.reason, reason);
    assert.equal(f.controller.getSnapshot().pendingActor, null); f.controller.dispose();
  }
});
await test('reset, dispose, same-seed replacement and late completions cannot repopulate history', async () => {
  for (const action of ['resetWorld', 'dispose']) {
    let finish; const f = fixture({ name: 'local', reason: req => new Promise(resolve => { finish = () => resolve(proposal(req)); }) });
    f.enable(); const pending = f.controller.sample('1'); await tick();
    f.controller[action](); await pending; finish(); await tick();
    assert.deepEqual(f.controller.getSnapshot().records, []);
    if (action === 'dispose') { f.advance(); await f.controller.sample('1'); assert.equal(f.reads(), 1); }
    f.controller.dispose();
  }
});
await test('stale/rewound/nonfinite simulationMinute never displays obsolete proposal', async () => {
  for (const minute of [197, 99, NaN]) {
    let finish; const f = fixture({ name: 'local', reason: req => new Promise(resolve => { finish = () => resolve(proposal(req)); }) });
    f.enable(); const pending = f.controller.sample('1'); await tick(); f.world.minute = minute;
    finish(); await pending; assert.deepEqual(f.controller.getSnapshot().records, []);
    assert.equal(f.controller.getSnapshot().status, 'stale'); f.controller.dispose();
  }
});
await test('world replacement at observation or completion invalidates requests', async () => {
  for (const observeReplacement of [true, false]) {
    let finish; const f = fixture({ name: 'local', reason: req => new Promise(resolve => { finish = () => resolve(proposal(req)); }) });
    f.enable(); const pending = f.controller.sample('1'); await tick(); f.world.worldSeed = 'different';
    if (observeReplacement) f.observe(); finish(); await pending;
    assert.deepEqual(f.controller.getSnapshot().records, []); f.controller.dispose();
  }
});
await test('deleted/deceased actors, including pending actors, and disappeared targets are removed', async () => {
  for (const death of [true, false]) {
    const f = fixture(); f.enable(); await f.controller.sample('1');
    if (death) f.residents[0].alive = false; else f.residents.shift();
    f.observe(); assert.deepEqual(f.controller.getSnapshot().records, []); f.controller.dispose();
    let finish; const g = fixture({ name: 'local', reason: req => new Promise(resolve => { finish = () => resolve(proposal(req)); }) });
    g.enable(); const pending = g.controller.sample('1'); await tick(); g.residents[0].alive = false;
    g.observe(); await pending; finish(); await tick(); assert.deepEqual(g.controller.getSnapshot().records, []); g.controller.dispose();
  }
});
await test('proposal target validation rejects invented/dead residents without action authority', async () => {
  const f = fixture({ name: 'local', reason: async req => ({ ...proposal(req), intent: 'CooperateWithResident', targetResident: 'missing' }) });
  f.source.cognitiveRequest = actor => ({ ...request(actor, f.world.minute), allowedIntents: ['CooperateWithResident'] });
  f.enable(); await f.controller.sample('1');
  assert.equal(f.controller.getSnapshot().records[0].outcome.reason, 'target_unavailable');
  assert.equal(f.controller.getSnapshot().records[0].outcome.proposal, null); f.controller.dispose();
});
await test('same-resident spam, concurrency, resident/history retention and global wall cadence are bounded', async () => {
  let calls = 0, finish;
  const f = fixture({ name: 'local', reason: req => { calls++; return new Promise(resolve => { finish = () => resolve(proposal(req)); }); } });
  f.enable(); const pending = f.controller.sample('1'); await tick();
  await Promise.all(Array.from({ length: 100 }, (_, i) => f.controller.sample(i % 2 ? '1' : '2')));
  assert.equal(calls, 1); finish(); await pending;
  await f.controller.sample('2'); assert.equal(calls, 1); f.controller.dispose();
  const g = fixture(); g.enable();
  for (let i = 0; i < 100; i++) { await g.controller.sample('1'); g.advance(); }
  assert.equal(g.controller.getSnapshot().records.length, SHADOW_BUDGET.historyPerResident);
  for (let i = 2; i < 50; i++) {
    const actor = String(i); g.residents.push({ id: actor, name: actor, alive: true });
    await g.controller.sample(actor); g.advance();
  }
  assert.equal(new Set(g.controller.getSnapshot().records.map(r => r.actor)).size, SHADOW_BUDGET.maxResidents);
  assert(g.controller.getSnapshot().records.length <= SHADOW_BUDGET.maxResidents * SHADOW_BUDGET.historyPerResident);
  const reads = g.reads(); g.controller.setEnabled(false); g.enable(); await g.controller.sample('1');
  // Toggling never bypasses the budget; the last call was one cadence ago here.
  assert.equal(g.reads(), reads + 1); await g.controller.sample('1'); assert.equal(g.reads(), reads + 1);
  g.controller.dispose();
});
await test('automatic sampling uses refresh cadence and selected resident; fast-forward cancels and blocks', async () => {
  const f = fixture(); f.enable(); f.controller.setAutomatic(true); f.observe(); await tick();
  assert.equal(f.reads(), 1); f.advance(); f.observe(); await tick(); assert.equal(f.reads(), 1);
  f.world.minute += 30; f.observe(); await tick(); assert.equal(f.reads(), 2);
  f.advance(); f.world.minute += 30; f.controller.suspend(true); f.observe(); await f.controller.sample('1');
  assert.equal(f.reads(), 2); assert.deepEqual(f.controller.getSnapshot().records, []);
  f.controller.suspend(false); f.controller.observe(f.world, f.residents, null); await tick(); assert.equal(f.reads(), 2);
  f.observe(); await tick(); assert.equal(f.reads(), 3); f.controller.dispose();
});
await test('scheduler observation cancellation/busy statuses preserve legacy null-return contract', async () => {
  const s = new CognitionScheduler({ name: 'local', reason: () => new Promise(() => {}) }, () => 100, { maxConcurrent: 1, maxQueued: 0, timeoutMs: 1000, staleAfterSimulationMinutes: 96 });
  const pending = s.submitObserved(request('1', 100));
  assert.equal((await s.submitObserved(request('2', 100))).status, 'busy');
  s.reset(); assert.equal((await pending).status, 'cancelled');
  s.dispose(); assert.equal(await s.submit(request('1', 100)), null);
});
await test('Core mutation/save APIs and Korean literals are absent from new observation/UI boundary', () => {
  const code = readFileSync(new URL('../../src/cognition/shadow-observer.ts', import.meta.url), 'utf8');
  assert(!/\.(runMinutes|createWorld|newGame|save|load|applyProposal|setNeeds)\(/.test(code));
  const ui = readFileSync(new URL('../../src/ui/cognitive-shadow-panel.tsx', import.meta.url), 'utf8');
  assert(!/[가-힣]/u.test(ui.replace('/[가-힣]/u', '')));
  assert(!/requestAnimationFrame|setInterval/.test(code));
});
console.log(`Cognitive Shadow Mode: ${passed} tests passed`);
