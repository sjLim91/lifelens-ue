#!/usr/bin/env node
import assert from 'node:assert/strict';
import { readFile, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
const directory = resolve(process.argv[2] ?? '/tmp/lifelens-artifact-fixture');
const runtime = resolve(process.argv[3] ?? 'Clients/Web/generated');
const previous = { fetch: globalThis.fetch, window: globalThis.window, document: globalThis.document };
let client, reloaded, expectedClient, fetches = 0;
globalThis.fetch = async () => { fetches++; throw Error('Unexpected network fetch'); };
globalThis.window = globalThis;
globalThis.document = { currentScript: null };
try {
  const js = await readFile(resolve(runtime, 'lifelens_core.js'), 'utf8');
  const wasmBinary = await readFile(resolve(runtime, 'lifelens_core.wasm'));
  const { default: createModule } = await import('data:text/javascript;base64,' + Buffer.from(js).toString('base64'));
  const module = await createModule({ wasmBinary, locateFile: p => p });
  client = new module.LifeLensWebClient();
  const start = await readFile(resolve(directory, 'start.bin'));
  const result = await readFile(resolve(directory, 'result.bin'));
  const minutes = Number(await readFile(resolve(directory, 'minutes.txt'), 'utf8'));
  assert(client.restoreSnapshotBase64(start.toString('base64')));
  client.runMinutes(Math.floor(minutes / 2));
  reloaded = new module.LifeLensWebClient();
  assert(reloaded.restoreSnapshotBase64(client.saveSnapshotBase64()));
  const remaining = minutes - Math.floor(minutes / 2);
  client.runMinutes(remaining); reloaded.runMinutes(remaining);
  const actualBinary=Buffer.from(client.saveSnapshotBase64(), 'base64');
  // Legacy v8 persists std::mt19937 as implementation-specific stream text:
  // libstdc++ writes a trailing index, libc++ normalizes its ring and omits it.
  // Require every byte outside that existing field to match, then let the
  // actual WASM codec normalize the native checkpoint and compare all bytes.
  const nativeRngLength=result.readUInt32LE(28), wasmRngLength=actualBinary.readUInt32LE(28);
  assert(result.subarray(0,28).equals(actualBinary.subarray(0,28)), 'exact native/WASM pre-RNG fields');
  assert(result.subarray(32+nativeRngLength).equals(actualBinary.subarray(32+wasmRngLength)),
    'all physical/knowledge/runtime/extension bytes match across native and WASM');
  expectedClient=new module.LifeLensWebClient();
  assert(expectedClient.restoreSnapshotBase64(result.toString('base64')));
  assert.equal(client.saveSnapshotBase64(),expectedClient.saveSnapshotBase64(),
    'entire snapshots match after the same real codec normalization');
  assert.equal(reloaded.saveSnapshotBase64(), client.saveSnapshotBase64(), 'mid-trial saved continuation');
  const residents = JSON.parse(client.residentsJson());
  const inventor = residents.residents.find(c => c.civilization.invention.events.length);
  assert(inventor?.civilization.invention.artifacts.length > 0);
  assert(inventor.civilization.invention.techniques.some(t => t.reproducible));
  const unchanged = client.saveSnapshotBase64();
  client.civilizationWorldJson(32); client.residentsJson();
  assert.equal(client.saveSnapshotBase64(), unchanged, 'observation cannot invent or alter state');
  assert.equal(fetches, 0);
  const proof = { profile: 'controlled real production Core; supplied initial raw materials and one paid token',
    nativeWasmPhysicalAndKnowledgeBytesEqual: true, legacyRngTextPlatformSpecific: true,
    codecNormalizedBinaryEqual: true, midTrialReplayEqual: true, readOnly: true, fetches, minutes,
    artifacts: inventor.civilization.invention.artifacts };
  await writeFile(resolve(directory, 'wasm-proof.json'), JSON.stringify(proof, null, 2));
  console.log('PASS WASM emergent artifact physical bytes, codec-normalized binary equality and mid-trial replay');
} finally {
  client?.delete(); reloaded?.delete(); expectedClient?.delete();
  for (const [key, value] of Object.entries(previous)) {
    if (value === undefined) delete globalThis[key]; else globalThis[key] = value;
  }
}
