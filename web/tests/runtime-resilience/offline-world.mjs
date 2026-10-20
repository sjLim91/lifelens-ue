// Offline time and persistence must use canonical Core minutes, not a UI
// clock or a regenerated copy of the same WorldSeed.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import ts from 'typescript';

const root = resolve(import.meta.dirname, '../../src');
const source = readFileSync(resolve(root, 'runtime/offline-world-store.ts'), 'utf8');
const js = ts.transpileModule(source, {
  compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
}).outputText;
const actualMinuteMs = 120000 / 1440;
const persistent = new Map();
const oldDB = globalThis.indexedDB;
globalThis.indexedDB = {
  open(name, version) {
    assert.equal(name, 'LifeLensContinuingWorld');
    assert.equal(version, 1);
    const db = {
      objectStoreNames: { contains: () => false },
      createObjectStore() {},
      close() {},
      transaction(_store, mode) {
        const tx = {
          aborted: false,
          abort() {
            this.aborted = true;
            queueMicrotask(() => this.onabort?.());
          },
          objectStore() {
            return {
              get(key) {
                const request = {};
                queueMicrotask(() => {
                  request.result = persistent.get(key);
                  request.onsuccess?.();
                });
                return request;
              },
              put(record) {
                assert.equal(mode, 'readwrite');
                queueMicrotask(() => {
                  if (tx.aborted) return;
                  persistent.set(record.key, record);
                  tx.oncomplete?.();
                });
              },
            };
          },
        };
        return tx;
      },
    };
    const request = { result: db };
    queueMicrotask(() => {
      request.onupgradeneeded?.();
      request.onsuccess?.();
    });
    return request;
  },
};
try {
  const exports = {};
  new Function('exports', 'require', js)(exports, name => {
    assert.equal(name, './lifelens-contract');
    return { REAL_MS_PER_SIMULATION_MINUTE_AT_1X: actualMinuteMs };
  });
  const { readPersistedWorld, writePersistedWorld, assertCheckpointProgress,
    elapsedOfflineMinutes, wallTimeAfterAdvance } = exports;
  assert.equal(await readPersistedWorld(), null);
  const t0 = 1790000000000;
  assert.equal(elapsedOfflineMinutes(t0, t0 - 60000), 0,
    'clock rollback cannot reverse life');
  assert.equal(elapsedOfflineMinutes(t0, t0), 0);
  assert.equal(elapsedOfflineMinutes(t0, t0 + 120000), 1440,
    'two real minutes are one Core day at the established 1x pace');
  assert.equal(elapsedOfflineMinutes(t0, t0 + 120000 + 500), 1446);
  assert.equal(wallTimeAfterAdvance(t0, 1440), t0 + 120000);

  const record = {
    key: 'active-world', schemaVersion: 1, worldSeed: '18446744073709551615',
    minute: 338, savedAtWallMs: t0, earthOrigin: null,
    snapshotBase64: 'QUJDREVGR0hJSktMTU5PUA==',
  };
  await writePersistedWorld(record);
  assert.deepEqual(await readPersistedWorld(), record);
  const later = { ...record, minute: 1778,
    savedAtWallMs: wallTimeAfterAdvance(t0, 1440) };
  await writePersistedWorld(later);
  assert.deepEqual(await readPersistedWorld(), later,
    'newer Core snapshot must atomically supersede the older checkpoint');
  // Two browser tabs may capture the same seed at different minutes. An
  // earlier write must never roll back a durable later Core checkpoint.
  const staleMinute = { ...later, minute: later.minute - 1,
    savedAtWallMs: later.savedAtWallMs + 1000 };
  const staleWall = { ...later, minute: later.minute + 1,
    savedAtWallMs: later.savedAtWallMs - 1000 };
  assert.throws(() => assertCheckpointProgress(later, staleMinute), /더 오래된/);
  assert.throws(() => assertCheckpointProgress(later, staleWall), /더 오래된/);
  await assert.rejects(writePersistedWorld(staleMinute), /더 오래된/);
  await assert.rejects(writePersistedWorld(staleWall), /더 오래된/);
  assert.deepEqual(await readPersistedWorld(), later,
    'stale same-seed transactions must preserve the newest Core save');
  const newer = { ...later, minute: later.minute + 1,
    savedAtWallMs: later.savedAtWallMs + 1000 };
  await writePersistedWorld(newer);
  assert.deepEqual(await readPersistedWorld(), newer);
  // An explicitly different WorldSeed is a new game; leases and multi-tab
  // authority will be handled separately, not faked by this monotonic check.
  assert.doesNotThrow(() => assertCheckpointProgress(newer,
    { ...newer, worldSeed: '44', minute: 0 }));

  await assert.rejects(writePersistedWorld({ ...later, minute: -1 }), /유효하지 않은/);
  persistent.set('active-world', { ...later, snapshotBase64: '' });
  await assert.rejects(readPersistedWorld(), /손상/);
  assert.equal(persistent.get('active-world').minute, 1778,
    'corrupt checkpoint cannot be silently overwritten by reading');
  console.log('PASS offline time accounting, Core checkpoint durability and corruption rejection');
} finally {
  if (oldDB === undefined) delete globalThis.indexedDB;
  else globalThis.indexedDB = oldDB;
}
