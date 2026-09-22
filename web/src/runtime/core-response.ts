import type { ResidentsPayload, WorldOverview } from './core-types';

type JsonObject = Record<string, unknown>;
function isObject(value: unknown): value is JsonObject {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

export function readCoreObject(text: string, label: string): JsonObject {
  let value: unknown;
  try { value = JSON.parse(text); }
  catch { throw new Error(`LifeLensCore ${label}: invalid JSON`); }
  if (!isObject(value)) throw new Error(`LifeLensCore ${label}: expected an object`);
  return value;
}

/** A missing overview is not evidence that every resident has died. */
export function readWorldOverview(text: string): WorldOverview {
  const value = readCoreObject(text, 'world overview');
  if (value.available === false
    || !Number.isSafeInteger(value.livingResidents) || Number(value.livingResidents) < 0
    || !Number.isSafeInteger(value.minute) || Number(value.minute) < 0) {
    throw new Error('LifeLensCore world overview: unavailable or invalid counts/time');
  }
  return value as WorldOverview;
}

/** Reject damaged snapshots atomically; preserve Core IDs without Number conversion. */
export function readResidents(text: string): ResidentsPayload {
  const value = readCoreObject(text, 'residents');
  if (value.available === false) return { available: false, residents: [] };
  if (!Array.isArray(value.residents)) throw new Error('LifeLensCore residents: missing array');
  const ids = new Set<string>();
  for (const resident of value.residents) {
    if (!isObject(resident) || typeof resident.id !== 'string' || !resident.id
      || typeof resident.name !== 'string' || ids.has(resident.id)
      || (resident.hasPosition === true
        && (!Number.isFinite(resident.gridX) || !Number.isFinite(resident.gridY)))) {
      throw new Error('LifeLensCore residents: invalid identity or position');
    }
    ids.add(resident.id);
  }
  return value as ResidentsPayload;
}
