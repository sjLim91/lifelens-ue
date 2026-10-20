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
    && /^\d+$/.test(value.worldSeed)
    && typeof value.minute === 'number'
    && Number.isSafeInteger(value.minute)
    && value.minute >= 0
    && typeof value.savedAtWallMs === 'number'
    && Number.isSafeInteger(value.savedAtWallMs)
    && (value.savedAtWallMs ?? 0) > 0
    && typeof value.snapshotBase64 === 'string'
    && value.snapshotBase64.length >= 16
    && value.snapshotBase64.length <= 64 * 1024 * 1024
    && (value.earthOrigin === null || (
      typeof value.earthOrigin === 'object'
      && Number.isFinite(value.earthOrigin?.latitudeDeg)
      && Number.isFinite(value.earthOrigin?.longitudeDeg)
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
  if (existing === null) return;
  if (!valid(existing)) {
    throw Error('기존 월드 기록이 손상되어 새 저장본으로 덮어쓰지 않습니다.');
  }
  if (existing.worldSeed !== incoming.worldSeed) return;
  if (incoming.minute < existing.minute
      || incoming.savedAtWallMs < existing.savedAtWallMs) {
    throw Error('더 오래된 월드 체크포인트가 최신 저장본을 덮어쓰지 못하도록 차단했습니다.');
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

export function elapsedOfflineMinutes(savedAtWallMs: number, nowWallMs: number): number {
  if (!Number.isSafeInteger(savedAtWallMs)
    || !Number.isSafeInteger(nowWallMs)
    || nowWallMs <= savedAtWallMs) return 0;
  return Math.max(0, Math.floor(
    (nowWallMs - savedAtWallMs) / REAL_MS_PER_SIMULATION_MINUTE_AT_1X,
  ));
}

export function wallTimeAfterAdvance(startWallMs: number, minutes: number): number {
  if (!Number.isSafeInteger(startWallMs) || minutes <= 0) return startWallMs;
  return Math.floor(startWallMs
    + minutes * REAL_MS_PER_SIMULATION_MINUTE_AT_1X);
}
