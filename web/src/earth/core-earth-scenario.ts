import type { WorldOverview } from '../runtime/core-types';

export interface CoreEarthScenarioObservation {
  available: boolean;
  worldSeed?: string;
  generationVersion?: number;
  regionId?: string;
  latitudeDeg?: number;
  longitudeDeg?: number;
  originGridX?: number;
  originGridY?: number;
  earthDataVersion?: string;
  source?: string;
  singleOrigin?: boolean;
  terrainGeographicallyConditioned?: boolean;
  metricScaleProvisional?: boolean;
  snapshotPersistence?: boolean;
}

export const UNBOUND_EARTH_SCENARIO: CoreEarthScenarioObservation =
  { available: false };

// Fail closed. Observer-only A/B preview points are never accepted as Core
// positions; the authoritative active world seed/generation must also match.
export function readCoreEarthScenario(
  raw: unknown,
  world: WorldOverview,
): CoreEarthScenarioObservation {
  if (!raw || typeof raw !== 'object') return UNBOUND_EARTH_SCENARIO;
  const entry = raw as Record<string, unknown>;
  if (entry.available !== true || world.worldSeed == null ||
      String(world.worldSeed) !== entry.worldSeed ||
      entry.generationVersion !== world.generationVersion ||
      entry.source !== 'CoreEarthScenario' ||
      typeof entry.regionId !== 'string' ||
      !/^\d{1,20}$/.test(entry.regionId) ||
      typeof entry.earthDataVersion !== 'string' ||
      !/^[-:._a-zA-Z0-9]{1,128}$/.test(entry.earthDataVersion) ||
      typeof entry.latitudeDeg !== 'number' ||
      !Number.isFinite(entry.latitudeDeg) ||
      entry.latitudeDeg < -90 || entry.latitudeDeg > 90 ||
      typeof entry.longitudeDeg !== 'number' ||
      !Number.isFinite(entry.longitudeDeg) ||
      entry.longitudeDeg < -180 || entry.longitudeDeg > 180 ||
      !Number.isSafeInteger(entry.originGridX) ||
      !Number.isSafeInteger(entry.originGridY) ||
      entry.singleOrigin !== true ||
      entry.terrainGeographicallyConditioned !== false ||
      entry.metricScaleProvisional !== true ||
      typeof entry.snapshotPersistence !== 'boolean') {
    return UNBOUND_EARTH_SCENARIO;
  }
  return entry as unknown as CoreEarthScenarioObservation;
}
