import { REAL_MS_PER_SIMULATION_MINUTE_AT_1X } from './lifelens-contract';

// One authoritative Core world per browser profile. Saved content remains on
// this device; GitHub Pages cannot process or synchronize it while offline.
export interface PersistedWorld {
  key: 'active-world';
  schemaVersion: 1;
  worldSeed: string;
  minute: number;
  savedAtWallMs: number;
  snapshotBase64: string;
  earthOrigin: { latitudeDeg: number; longitudeDeg: number } | null;
}

const DATABASE_NAME = 'LifeLensContinuingWorld';
const DATABASE_VERSION = 1;
const STORE_NAME = 'checkpoints';
const PRIMARY_KEY = 'active-world';
// LifeLensCore World::minute is a signed C++ int. An oversized JS value
// must never be accepted as a durable authoritative minute or replay cursor.
export const MAX_CORE_WORLD_MINUTE = 2147483647;

function database(): Promise<IDBDatabase> {
  if (typeof indexedDB === 'undefined') {
    return Promise.reject(Error('이 브라우저는 월드 자동 저장소를 지원하지 않습니다.'));
  }
  return new Promise((resolve, reject) => {
    const open = indexedDB.open(DATABASE_NAME, DATABASE_VERSION);
    open.onupgradeneeded = () => {
      if (!open.result.objectStoreNames.contains(STORE_NAME)) {
        open.result.createObjectStore(STORE_NAME, { keyPath: 'key' });
      }
    };
    open.onerror = () => reject(open.error ?? Error('월드 저장소를 열 수 없습니다.'));
    open.onsuccess = () => resolve(open.result);
    open.onblocked = () => reject(Error('다른 탭이 LifeLens 저장소를 사용 중입니다.'));
  });
}

function valid(record: unknown): record is PersistedWorld {
  if (!record || typeof record !== 'object') return false;
  const value = record as Partial<PersistedWorld>;
  return value.key === PRIMARY_KEY
    && value.schemaVersion === 1
    && typeof value.worldSeed === 'string'
    // The canonical Core WorldSeed is an unsigned nonzero 64-bit decimal.
    // Reject "0", aliases with leading zeros, and overflow values before
    // restoring their encoded Core snapshot or comparing save authority.
    && /^[1-9]\d{0,19}$/.test(value.worldSeed)
    && BigInt(value.worldSeed) <= 18446744073709551615n
    && typeof value.minute === 'number'
    && Number.isSafeInteger(value.minute)
    && value.minute >= 0
    && value.minute <= MAX_CORE_WORLD_MINUTE
    && typeof value.savedAtWallMs === 'number'
    && Number.isSafeInteger(value.savedAtWallMs)
    && (value.savedAtWallMs ?? 0) > 0
    && typeof value.snapshotBase64 === 'string'
    && value.snapshotBase64.length >= 16
    && value.snapshotBase64.length <= 64 * 1024 * 1024
    // A snapshot is stored as canonical base64 bytes, not arbitrary text.
    // Structural damage must fail closed before decode/restore or overwrite.
    && value.snapshotBase64.length % 4 === 0
    && /^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$/.test(
      value.snapshotBase64,
    )
    && (value.earthOrigin === null || (
      typeof value.earthOrigin === 'object'
      && Number.isFinite(value.earthOrigin?.latitudeDeg)
      && Number.isFinite(value.earthOrigin?.longitudeDeg)
      // Latitude and longitude are physical Earth coordinates. A finite
      // value outside the globe must never reach restored Core geography.
      && (value.earthOrigin?.latitudeDeg ?? 91) >= -90
      && (value.earthOrigin?.latitudeDeg ?? -91) <= 90
      && (value.earthOrigin?.longitudeDeg ?? 181) >= -180
      && (value.earthOrigin?.longitudeDeg ?? -181) <= 180
    ));
}

export async function readPersistedWorld(): Promise<PersistedWorld | null> {
  const db = await database();
  try {
    return await new Promise((resolve, reject) => {
      const request = db.transaction(STORE_NAME, 'readonly')
        .objectStore(STORE_NAME).get(PRIMARY_KEY);
      request.onerror = () => reject(request.error ?? Error('월드 읽기에 실패했습니다.'));
      request.onsuccess = () => {
        if (request.result === undefined) { resolve(null); return; }
        if (!valid(request.result)) {
          reject(Error('저장된 월드 형식이 손상되었습니다. 기존 데이터를 덮어쓰지 않습니다.'));
          return;
        }
        resolve(request.result);
      };
    });
  } finally {
    db.close();
  }
}

// Existing and incoming Core checkpoints are compared under the SAME
// IndexedDB readwrite transaction, avoiding a separate read/write TOCTOU.
// This only guards stale same-seed writes; it is NOT a multi-tab writer lease.
export function assertCheckpointProgress(
  existing: PersistedWorld | null,
  incoming: PersistedWorld,
): void {
  // This assertion is also exported for direct callers, not just the
  // IndexedDB writer. A malformed candidate is never an admissible Core
  // checkpoint even when there is no previous save to compare against.
  if (!valid(incoming)) {
    throw Error('유효하지 않은 Core 체크포인트 비교 요청입니다.');
  }
  if (existing === null) return;
  if (!valid(existing)) {
    throw Error('기존 월드 기록이 손상되어 새 저장본으로 덮어쓰지 않습니다.');
  }
  if (existing.worldSeed !== incoming.worldSeed) return;
  // One seed and a continuing Core world must not silently change its
  // geographic origin between checkpoints, even at a later saved minute.
  // Changing the starting Earth origin requires an explicit new-game seed.
  if ((existing.earthOrigin === null) !== (incoming.earthOrigin === null)
      || existing.earthOrigin?.latitudeDeg !== incoming.earthOrigin?.latitudeDeg
      || existing.earthOrigin?.longitudeDeg !== incoming.earthOrigin?.longitudeDeg) {
    throw Error('같은 월드의 지구 시작 위치가 달라 체크포인트 충돌을 차단했습니다.');
  }
  if (incoming.minute < existing.minute
      || incoming.savedAtWallMs < existing.savedAtWallMs) {
    throw Error('더 오래된 월드 체크포인트가 최신 저장본을 덮어쓰지 못하도록 차단했습니다.');
  }
  // Equal Core time AND equal wall clock must be byte-for-byte idempotent.
  // A divergent same-instant snapshot implies competing writers or damage,
  // not a new authoritative world tick. Fail closed rather than overwrite.
  if (incoming.minute === existing.minute
      && incoming.savedAtWallMs === existing.savedAtWallMs
      && (incoming.snapshotBase64 !== existing.snapshotBase64
          || incoming.earthOrigin?.latitudeDeg !== existing.earthOrigin?.latitudeDeg
          || incoming.earthOrigin?.longitudeDeg !== existing.earthOrigin?.longitudeDeg
          || (incoming.earthOrigin === null) !== (existing.earthOrigin === null))) {
    throw Error('동일 시각의 서로 다른 Core 체크포인트 충돌을 차단했습니다.');
  }
}

export async function writePersistedWorld(record: PersistedWorld): Promise<void> {
  if (!valid(record)) throw Error('유효하지 않은 월드 저장 요청입니다.');
  const db = await database();
  try {
    await new Promise<void>((resolve, reject) => {
      const tx = db.transaction(STORE_NAME, 'readwrite');
      const store = tx.objectStore(STORE_NAME);
      tx.oncomplete = () => resolve();
      tx.onabort = () => reject(tx.error ?? Error('월드 저장이 취소됐습니다.'));
      tx.onerror = () => reject(tx.error ?? Error('월드 저장에 실패했습니다.'));
      const get = store.get(PRIMARY_KEY);
      get.onerror = () => {
        reject(get.error ?? Error('기존 월드 기록을 확인하지 못했습니다.'));
        tx.abort();
      };
      get.onsuccess = () => {
        try {
          assertCheckpointProgress(
            get.result === undefined ? null : get.result as PersistedWorld,
            record,
          );
          store.put(record);
        } catch (error) {
          reject(error);
          tx.abort();
        }
      };
    });
  } finally {
    db.close();
  }
}

// Native LifeLensCore World::minute is a signed 32-bit integer. Validate
// the requested advance BEFORE invoking Core, even if the wall-time debt is a
// safe JavaScript integer. Both interactive skip-days and offline replay use
// the same exclusive authority; never wrap a living world into negative time.
export function assertCoreMinuteAdvanceCapacity(
  currentMinute: number,
  advanceMinutes: number,
): void {
  if (!Number.isSafeInteger(currentMinute) || currentMinute < 0
    || currentMinute > MAX_CORE_WORLD_MINUTE
    || !Number.isSafeInteger(advanceMinutes) || advanceMinutes < 0
    || advanceMinutes > MAX_CORE_WORLD_MINUTE - currentMinute) {
    throw Error('Core 월드 시간이 허용된 범위를 초과하여 진행을 차단했습니다.');
  }
}

// Only finite, bounded wall-clock math may feed Core catch-up decisions.
// Browsers can report a rolled-back time, corrupted storage values, or a
// gigantic clock jump. None should produce NaN/Infinity/fractional ticks.
export function elapsedOfflineMinutes(savedAtWallMs: number, nowWallMs: number): number {
  if (!Number.isSafeInteger(savedAtWallMs)
    || !Number.isSafeInteger(nowWallMs)
    || nowWallMs <= savedAtWallMs
    || !Number.isFinite(REAL_MS_PER_SIMULATION_MINUTE_AT_1X)
    || REAL_MS_PER_SIMULATION_MINUTE_AT_1X <= 0) return 0;
  const elapsed = (nowWallMs - savedAtWallMs) / REAL_MS_PER_SIMULATION_MINUTE_AT_1X;
  if (!Number.isFinite(elapsed) || elapsed <= 0) return 0;
  return Math.min(Number.MAX_SAFE_INTEGER, Math.floor(elapsed));
}

export function wallTimeAfterAdvance(startWallMs: number, minutes: number): number {
  if (!Number.isSafeInteger(startWallMs) || startWallMs < 0
    || !Number.isSafeInteger(minutes) || minutes < 0
    || !Number.isFinite(REAL_MS_PER_SIMULATION_MINUTE_AT_1X)
    || REAL_MS_PER_SIMULATION_MINUTE_AT_1X <= 0) {
    throw Error('월드 시간 복원에 유효하지 않은 Core 분·시계 값이 전달됐습니다.');
  }
  if (minutes === 0) return startWallMs;
  const advanced = startWallMs + minutes * REAL_MS_PER_SIMULATION_MINUTE_AT_1X;
  if (!Number.isFinite(advanced) || advanced > Number.MAX_SAFE_INTEGER) {
    return Number.MAX_SAFE_INTEGER;
  }
  return Math.max(startWallMs, Math.floor(advanced));
}
