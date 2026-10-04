import type { CivilizationSettlement, CivilizationTradeRoute, CivilizationWorldPayload, Resident } from '../runtime/core-types';

const validId = (id: unknown): id is string => typeof id === 'string' && id.trim().length > 0 && id !== '0';
const coordinate = (value: unknown): value is number => typeof value === 'number' && Number.isFinite(value);
const count = (value: unknown) => coordinate(value) && Number.isInteger(value) && value >= 0;

/** Reject ambiguous duplicate identities; never join by proximity. No history or gameplay writes. */
function unique<T extends { id: string }>(entries: T[]): T[] {
  const counts = new Map<string, number>();
  for (const entry of entries) if (entry && validId(entry.id)) counts.set(entry.id, (counts.get(entry.id) ?? 0) + 1);
  return entries.filter(entry => entry && counts.get(entry.id) === 1);
}

export function observedSettlements(payload: CivilizationWorldPayload): CivilizationSettlement[] {
  if (payload?.available !== true || !Array.isArray(payload.settlements)) return [];
  return unique(payload.settlements).filter(entry => validId(entry.id)
    && coordinate(entry.gridX) && coordinate(entry.gridY)
    && [entry.residentCount, entry.facilityCount, entry.operationalFacilityCount,
      entry.plannedFacilityCount, entry.storageSiteCount].every(count)
    && typeof entry.active === 'boolean' && typeof entry.established === 'boolean');
}

export function observedTradeRoutes(payload: CivilizationWorldPayload,
  settlements = observedSettlements(payload)): CivilizationTradeRoute[] {
  if (payload?.available !== true || !Array.isArray(payload.tradeRoutes)) return [];
  const ids = new Set(settlements.map(entry => entry.id));
  return unique(payload.tradeRoutes).filter(route => validId(route.id)
    && ids.has(route.firstSettlement) && ids.has(route.secondSettlement)
    && route.firstSettlement !== route.secondSettlement
    && [route.firstGridX, route.firstGridY, route.secondGridX, route.secondGridY].every(coordinate)
    && (route.firstGridX !== route.secondGridX || route.firstGridY !== route.secondGridY)
    && count(route.partnerCount) && count(route.exchangeCount)
    && coordinate(route.distanceGrid) && route.distanceGrid >= 0 && typeof route.active === 'boolean');
}

/** A direction towards a Core frontier, never an arrival, planned settlement or navigation path. */
export function selectedFrontier(resident: Resident | undefined): { x: number; y: number } | null {
  const migration = resident?.migration;
  if (!resident || resident.alive !== true || resident.hasPosition !== true
    || !coordinate(resident.gridX) || !coordinate(resident.gridY)
    || migration?.candidate !== true || migration.hasFrontierTarget !== true
    || !coordinate(migration.frontierGridX) || !coordinate(migration.frontierGridY)) return null;
  return { x: migration.frontierGridX, y: migration.frontierGridY };
}
