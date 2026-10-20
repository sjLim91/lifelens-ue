// Exercise the actual Web Locks lease coordinator, including races and
// unsupported browsers, without creating two simulated Core authorities.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import ts from 'typescript';

const source = readFileSync(resolve(import.meta.dirname,
  '../../src/runtime/world-writer-lease.ts'), 'utf8');
const exports = {};
new Function('exports', ts.transpileModule(source, {
  compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
}).outputText)(exports);

const original = Object.getOwnPropertyDescriptor(globalThis, 'navigator');
function setNavigator(value) {
  Object.defineProperty(globalThis, 'navigator', {
    value, configurable: true, writable: true,
  });
}
try {
  const { acquireWorldWriterLease } = exports;
  setNavigator({});
  await assert.rejects(acquireWorldWriterLease(), /지원하지 않습니다/);
  let requests = 0;
  let owner = false;
  let releases = 0;
  setNavigator({
    locks: {
      async request(name, options, callback) {
        assert.equal(name, 'lifelens-active-core-world');
        assert.equal(options.mode, 'exclusive');
        assert.equal(options.ifAvailable, true);
        requests++;
        if (owner) return callback(null);
        owner = true;
        try { return await callback({ name, mode: 'exclusive' }); }
        finally { owner = false; releases++; }
      },
    },
  });
  const first = await acquireWorldWriterLease();
  assert.equal(first.active, true);
  await assert.rejects(acquireWorldWriterLease(), /다른 탭에서/);
  assert.equal(requests, 2);
  first.release();
  first.release(); // idempotent cleanup
  // Web Lock release resolves asynchronously before the next owner enters.
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(releases, 1);
  const second = await acquireWorldWriterLease();
  assert.equal(second.active, true);
  second.release();
  await new Promise(resolve => setImmediate(resolve));
  assert.equal(releases, 2);
  console.log('PASS one writer lease: no concurrent Core owner, rejection and release');
} finally {
  if (original) Object.defineProperty(globalThis, 'navigator', original);
  else delete globalThis.navigator;
}
