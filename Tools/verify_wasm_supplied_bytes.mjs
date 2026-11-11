#!/usr/bin/env node
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

// Exercise the generated browser factory itself, not a fake bridge. A build
// which ignores Module.wasmBinary must fail even if its relative URL exists.
const directory = process.argv[2]
  ? resolve(process.argv[2])
  : fileURLToPath(new URL('../Clients/Web/generated/', import.meta.url));
const timeout = setTimeout(() => {
  console.error('WASM supplied-bytes smoke: initialization timed out');
  process.exit(1);
}, 30_000);
const originalFetch = globalThis.fetch;
let fetchAttempts = 0;
globalThis.fetch = async () => {
  fetchAttempts += 1;
  throw new Error('Unexpected network fetch: runtime must use supplied WASM bytes');
};

// Older Emscripten web builds inspect these host names during startup. They
// contain no simulation data and do not replace any generated Core binding.
const originalWindow = globalThis.window;
const originalDocument = globalThis.document;
globalThis.window = globalThis;
globalThis.document = { currentScript: null };
let client;
try {
  const [javascript, wasm] = await Promise.all([
    readFile(resolve(directory, 'lifelens_core.js'), 'utf8'),
    readFile(resolve(directory, 'lifelens_core.wasm')),
  ]);
  assert.ok(javascript.trim() && !/^\s*</.test(javascript), 'generated JavaScript is empty or HTML');
  assert.deepEqual([...wasm.subarray(0, 8)], [0, 97, 115, 109, 1, 0, 0, 0], 'invalid WASM header');
  // Import the unmodified ESM bytes regardless of the repository's package
  // type. This also exercises the browser's fallback module-loading shape.
  const { default: createModule } = await import(
    `data:text/javascript;base64,${Buffer.from(javascript).toString('base64')}`
  );
  assert.equal(typeof createModule, 'function', 'missing Emscripten module factory');
  const module = await createModule({
    wasmBinary: Uint8Array.from(wasm),
    locateFile: path => path,
  });
  assert.equal(typeof module.LifeLensWebClient, 'function', 'missing actual Core binding');
  client = new module.LifeLensWebClient();
  assert.equal(client.hasSimulation(), false);
  assert.equal(client.newGame('874213954', '', 4), true);
  assert.equal(client.hasSimulation(), true);
  const overview = JSON.parse(client.worldOverviewJson());
  const residents = JSON.parse(client.residentsJson());
  assert.equal(overview.available, true);
  assert.equal(overview.totalResidents, 4);
  assert.equal(overview.livingResidents, 4);
  assert.equal(residents.available, true);
  assert.equal(residents.residents.length, 4);
  assert.equal(new Set(residents.residents.map(resident => resident.id)).size, 4);
  client.runMinutes(1);
  assert.equal(JSON.parse(client.worldOverviewJson()).minute, overview.minute + 1);
  assert.equal(fetchAttempts, 0, 'generated runtime attempted an additional download');
  console.log('WASM supplied-bytes smoke: PASS (real Core, population=4, elapsed_minutes=1, fetches=0)');
} catch (error) {
  // A data-URL stack includes the entire generated module. Report the actual
  // failure concisely so CI logs remain useful.
  console.error(`WASM supplied-bytes smoke: FAIL (${error instanceof Error ? error.message : String(error)}; fetches=${fetchAttempts})`);
  process.exitCode = 1;
} finally {
  client?.delete();
  clearTimeout(timeout);
  globalThis.fetch = originalFetch;
  if (originalWindow === undefined) delete globalThis.window;
  else globalThis.window = originalWindow;
  if (originalDocument === undefined) delete globalThis.document;
  else globalThis.document = originalDocument;
}
