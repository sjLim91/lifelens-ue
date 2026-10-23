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
    elapsedOfflineMinutes, wallTimeAfterAdvance, MAX_CORE_WORLD_MINUTE,
    assertCoreMinuteAdvanceCapacity } = exports;
  assert.equal(await readPersistedWorld(), null);
  const t0 = 1790000000000;
  assert.equal(elapsedOfflineMinutes(t0, t0 - 60000), 0,
    'clock rollback cannot reverse life');
  assert.equal(elapsedOfflineMinutes(t0, t0), 0);
  assert.equal(elapsedOfflineMinutes(t0, t0 + 120000), 1440,
    'two real minutes are one Core day at the established 1x pace');
  assert.equal(elapsedOfflineMinutes(t0, t0 + 120000 + 500), 1446);
  assert.equal(wallTimeAfterAdvance(t0, 1440), t0 + 120000);
  assert.equal(elapsedOfflineMinutes(Number.NaN, t0), 0);
  assert.equal(elapsedOfflineMinutes(t0, Number.POSITIVE_INFINITY), 0);
  assert.equal(elapsedOfflineMinutes(t0, t0 + 1.5), 0);
  assert.equal(elapsedOfflineMinutes(t0, t0 + 42), 0,
    'fractional Core minutes are never rounded up');
  const hugeOffline = elapsedOfflineMinutes(0, Number.MAX_SAFE_INTEGER);
  assert(Number.isSafeInteger(hugeOffline) && hugeOffline > 0,
    'absurd wall-clock jumps cannot escape safe integer Core minutes');
  assert.equal(wallTimeAfterAdvance(t0, 0), t0);
  assert.equal(wallTimeAfterAdvance(Number.MAX_SAFE_INTEGER - 5, 1440),
    Number.MAX_SAFE_INTEGER, 'overflowing wall time saturates at a safe timestamp');
  assert.throws(() => wallTimeAfterAdvance(t0, Number.NaN), /유효하지 않은/);
  assert.throws(() => wallTimeAfterAdvance(t0, Number.POSITIVE_INFINITY), /유효하지 않은/);
  assert.throws(() => wallTimeAfterAdvance(t0, -1), /유효하지 않은/);
  assert.throws(() => wallTimeAfterAdvance(t0, 0.5), /유효하지 않은/);
  assert.throws(() => wallTimeAfterAdvance(Number.NaN, 100), /유효하지 않은/);

  const record = {
    key: 'active-world', schemaVersion: 1, worldSeed: '18446744073709551615',
    minute: 338, savedAtWallMs: t0, earthOrigin: null,
    snapshotBase64: 'QUJDREVGR0hJSktMTU5PUA==',
  };
  assert.equal(MAX_CORE_WORLD_MINUTE, 2147483647,
    'the durable minute must match the authoritative signed Core field');
  assert.doesNotThrow(() => assertCoreMinuteAdvanceCapacity(0, 0));
  assert.doesNotThrow(() => assertCoreMinuteAdvanceCapacity(
    MAX_CORE_WORLD_MINUTE - 1440, 1440
  ), 'exact maximum Core minute remains allowed');
  for (const [minute, advance] of [
    [MAX_CORE_WORLD_MINUTE, 1],
    [MAX_CORE_WORLD_MINUTE - 100, 1440],
    [MAX_CORE_WORLD_MINUTE + 1, 0],
    [-1, 1440],
    [100, -1],
    [100, 0.5],
    [Number.NaN, 1],
    [100, Number.POSITIVE_INFINITY],
    [100, Number.MAX_SAFE_INTEGER],
  ]) {
    assert.throws(() => assertCoreMinuteAdvanceCapacity(minute, advance),
      /Core 월드 시간/, 'native Core time overflow must fail BEFORE replay');
  }

  assert.doesNotThrow(() => assertCheckpointProgress(null, {
    ...record, minute: MAX_CORE_WORLD_MINUTE,
  }));
  for (const minute of [MAX_CORE_WORLD_MINUTE + 1, Number.MAX_SAFE_INTEGER,
    Number.POSITIVE_INFINITY, 1.5, -1]) {
    const forged = { ...record, minute };
    assert.throws(() => assertCheckpointProgress(null, forged),
      /유효하지 않은/, 'an empty save cannot authorize forged Core time');
    await assert.rejects(writePersistedWorld(forged),
      /유효하지 않은/, 'oversized/fractional time cannot become a checkpoint');
  }
  assert.throws(() => assertCheckpointProgress(null, {
    ...record, snapshotBase64: '!!!!',
  }), /유효하지 않은/, 'direct comparison must validate incoming Core bytes');

  for (const invalidSeed of [
    '0', '00', '00042', '', '-1', '1.0', 'abc', '18446744073709551616',
    '999999999999999999999999999999999',
  ]) {
    await assert.rejects(writePersistedWorld({
      ...record, worldSeed: invalidSeed,
    }), /유효하지 않은/, `invalid unsigned Core WorldSeed ${invalidSeed}`);
  }
  for (const earthOrigin of [
    { latitudeDeg: 91, longitudeDeg: 0 },
    { latitudeDeg: -91, longitudeDeg: 0 },
    { latitudeDeg: 0, longitudeDeg: 181 },
    { latitudeDeg: 0, longitudeDeg: -181 },
    { latitudeDeg: Number.NaN, longitudeDeg: 20 },
    { latitudeDeg: 0, longitudeDeg: Number.POSITIVE_INFINITY },
  ]) {
    await assert.rejects(writePersistedWorld({ ...record, earthOrigin }),
      /유효하지 않은/, 'outside Earth coordinates cannot enter a Core save');
  }
  const validEdgeOrigin = { latitudeDeg: -90, longitudeDeg: 180 };
  assert.doesNotThrow(() => assertCheckpointProgress(null, {
    ...record, earthOrigin: validEdgeOrigin,
  }));
  for (const snapshotBase64 of [
    '!!!!!!!!!!!!!!!!!!!!', 'QUJDREVGR0hJSktMTU5PUA=',
    'QUJDREVGR0hJSktMTU5PUA\n=', 'QUJDREVGR0hJSktMTU5PUA=!',
  ]) {
    await assert.rejects(writePersistedWorld({ ...record, snapshotBase64 }),
      /유효하지 않은/, 'malformed base64 cannot become authoritative Core bytes');
  }
  await writePersistedWorld(record);
  assert.deepEqual(await readPersistedWorld(), record);
  const later = { ...record, minute: 1778,
    savedAtWallMs: wallTimeAfterAdvance(t0, 1440) };
  await writePersistedWorld(later);
  assert.deepEqual(await readPersistedWorld(), later,
    'newer Core snapshot must atomically supersede the older checkpoint');
  // Competing tabs cannot silently claim different Core truth for the very
  // same minute and wall timestamp. Exact repeats are harmless.
  await writePersistedWorld({ ...later });
  const divergentBytes = {
    ...later, snapshotBase64: 'RkFLRS1DT1JFLVNOQVBTSE9ULURBVEE=',
  };
  await assert.rejects(writePersistedWorld(divergentBytes), /충돌/);
  await assert.rejects(writePersistedWorld({
    ...later, earthOrigin: { latitudeDeg: 10, longitudeDeg: 20 },
  }), /충돌/);
  assert.deepEqual(await readPersistedWorld(), later,
    'rejected same-instant divergent snapshot must preserve prior Core truth');
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
  // A later timestamp cannot disguise the very same world's Earth-origin
  // mutation. A new origin belongs to an explicitly new game.
  await assert.rejects(writePersistedWorld({
    ...newer, minute: newer.minute + 1,
    savedAtWallMs: newer.savedAtWallMs + 1000,
    earthOrigin: { latitudeDeg: 10, longitudeDeg: 20 },
  }), /충돌/);
  assert.deepEqual(await readPersistedWorld(), newer);
  // An explicitly different WorldSeed is a new game; leases and multi-tab
  // authority will be handled separately, not faked by this monotonic check.
  assert.doesNotThrow(() => assertCheckpointProgress(newer,
    { ...newer, worldSeed: '44', minute: 0 }));

  await assert.rejects(writePersistedWorld({ ...later, minute: -1 }), /유효하지 않은/);
  persistent.set('active-world', {
    ...later, worldSeed: '18446744073709551616',
  });
  await assert.rejects(readPersistedWorld(), /손상/);
  await assert.rejects(writePersistedWorld({
    ...later, minute: later.minute + 1,
    savedAtWallMs: later.savedAtWallMs + 1000,
  }), /손상/, 'do not overwrite corrupt WorldSeed with a fresh receipt');
  persistent.set('active-world', { ...later, snapshotBase64: '!!!!!!!!!!!!!!!!!' });
  await assert.rejects(readPersistedWorld(), /손상/);
  persistent.set('active-world', { ...later, snapshotBase64: '' });
  await assert.rejects(readPersistedWorld(), /손상/);
  assert.equal(persistent.get('active-world').minute, 1778,
    'corrupt checkpoint cannot be silently overwritten by reading');
  console.log('PASS offline time accounting, Core checkpoint durability and corruption rejection');
} finally {
  if (oldDB === undefined) delete globalThis.indexedDB;
  else globalThis.indexedDB = oldDB;
}
