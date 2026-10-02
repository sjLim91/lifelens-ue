import type { CivilizationSettlement, CivilizationWorldPayload, HumanTrace } from '../runtime/core-types';

export function settlementLabel(id: string): string { return `정착지 ${id}`; }

function nodeKey(id: string, storage = false): string | null {
  if (!/^[1-9]\d*$/.test(id)) return null;
  return ((BigInt(id) << 1n) | (storage ? 1n : 0n)).toString();
}

/** Exact representative-node join, not a nearest-anchor membership guess.
 * SettlementNetwork.h defines cluster ID as its minimum encoded node key.
 * Other facility/resident/storage membership is not exposed by the binding. */
export function representativeSettlementForTrace(
  trace: HumanTrace | undefined, civilization: CivilizationWorldPayload,
): CivilizationSettlement | undefined {
  if (civilization.available !== true || trace?.kind !== 'Facility') return undefined;
  const facility = civilization.facilities?.find(entry => trace.id === `facility:${entry.id}`
    && trace.gridX === entry.gridX && trace.gridY === entry.gridY
    && trace.facilityKind === entry.kind && entry.state !== 'Ruined');
  if (!facility) return undefined;
  const keys = [nodeKey(facility.id), nodeKey(facility.linkedStorage, true)];
  return civilization.settlements?.find(entry => keys.includes(entry.id));
}

export function representativeStorage(id: string, civilization: CivilizationWorldPayload) {
  if (civilization.available !== true) return undefined;
  const direct = civilization.storages?.find(entry => nodeKey(entry.id, true) === id);
  if (direct) return direct;
  const facility = civilization.facilities?.find(entry => nodeKey(entry.id) === id && entry.state !== 'Ruined');
  return facility && civilization.storages?.find(entry => entry.id === facility.linkedStorage);
}
