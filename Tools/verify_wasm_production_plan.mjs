#!/usr/bin/env node
import assert from 'node:assert/strict';
import { readFile, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
const directory = resolve(process.argv[2] ?? '/tmp/lifelens-production-fixture');
const runtime = resolve(process.argv[3] ?? 'Clients/Web/generated');
const saved = { fetch: globalThis.fetch, window: globalThis.window, document: globalThis.document };
let client, resumed, expectedClient, fetches = 0;
globalThis.fetch = async () => { fetches++; throw Error('Unexpected network fetch'); };
globalThis.window = globalThis; globalThis.document = { currentScript: null };
try {
  const js = await readFile(resolve(runtime, 'lifelens_core.js'), 'utf8');
  const wasmBinary = await readFile(resolve(runtime, 'lifelens_core.wasm'));
  const { default: createModule } = await import('data:text/javascript;base64,' + Buffer.from(js).toString('base64'));
  const module = await createModule({ wasmBinary, locateFile: path => path });
  client = new module.LifeLensWebClient(); resumed = new module.LifeLensWebClient(); expectedClient = new module.LifeLensWebClient();
  const start = await readFile(resolve(directory, 'start.bin'));
  const expected = await readFile(resolve(directory, 'result.bin'));
  const minutes = Number(await readFile(resolve(directory, 'minutes.txt'), 'utf8'));
  assert(minutes > 0 && minutes <= 120);
  assert(client.restoreSnapshotBase64(start.toString('base64')));
  const midpoint = Math.floor(minutes / 2); client.runMinutes(midpoint);
  assert(resumed.restoreSnapshotBase64(client.saveSnapshotBase64()));
  client.runMinutes(minutes - midpoint); resumed.runMinutes(minutes - midpoint);
  assert.equal(resumed.saveSnapshotBase64(), client.saveSnapshotBase64(), 'exact same-backend mid-plan saved continuation');
  const actual = Buffer.from(client.saveSnapshotBase64(), 'base64');
  // The old mt19937 text differs between libstdc++ and libc++; all other
  // physical, personal-plan and extension bytes must be exactly identical.
  assert(expected.subarray(0, 28).equals(actual.subarray(0, 28)));
  assert(expected.subarray(32 + expected.readUInt32LE(28)).equals(actual.subarray(32 + actual.readUInt32LE(28))));
  assert(expectedClient.restoreSnapshotBase64(expected.toString('base64')));
  assert.equal(expectedClient.saveSnapshotBase64(), client.saveSnapshotBase64(), 'whole binary after the same actual codec normalization');
  const residents = JSON.parse(client.residentsJson());
  const actor = residents.residents.find(r => r.civilization?.productionPlan?.state === 2 && r.civilization.productionPlan.steps.length >= 2);
  assert(actor, 'untouched native world really completed a multi-step objective');
  const plan = actor.civilization.productionPlan;
  assert.equal(plan.cognitiveOrigin, false, 'no injected model thought in this natural fixture');
  assert(plan.steps.length <= 6 && plan.expiresMinute - plan.createdMinute === 120);
  assert(plan.steps.some(s => s.result === 2 && s.attempted));
  assert(plan.steps.some(s => s.action === 'Gather' && s.result === 1 && s.stockAfter > s.stockBefore));
  assert(plan.steps.at(-1).stockAfter >= plan.targetStock);
  const checkpoint = client.saveSnapshotBase64();
  client.residentsJson(); client.civilizationWorldJson(32);
  assert.equal(client.saveSnapshotBase64(), checkpoint, 'read-only observer');
  assert.equal(client.restoreSnapshotBase64(actual.subarray(0, actual.length - 1).toString('base64')), false);
  assert.equal(client.saveSnapshotBase64(), checkpoint, 'bad restore is atomic');
  assert.equal(fetches, 0);
  const proof = { profile: 'untouched production New Game; native Core goal checkpoint and real WASM execution',
    nativeWasmPhysicalAndPlanBytesEqual: true, legacyRngTextPlatformSpecific: true, codecNormalizedBinaryEqual: true,
    midPlanReplayEqual: true, failedRestoreAtomic: true, readOnly: true, fetches, minutes,
    world: JSON.parse(client.worldOverviewJson()), actor: actor.id, plan };
  await writeFile(resolve(directory, 'wasm-proof.json'), JSON.stringify(proof, null, 2));
  await writeFile(resolve(directory, 'residents.json'), JSON.stringify(residents, null, 2));
  console.log('PASS untouched native/WASM failed experiment -> real gather -> successful production, exact saved continuation and atomic malformed restore');
} finally {
  client?.delete(); resumed?.delete(); expectedClient?.delete();
  for (const [key, value] of Object.entries(saved)) if (value === undefined) delete globalThis[key]; else globalThis[key] = value;
}
