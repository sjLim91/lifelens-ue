import type { WorldOverview } from '../runtime/core-types';
import type { CoreEarthScenarioObservation } from './core-earth-scenario';

// Read-only Core observation. Never infer local sunshine from browser clock
// or treat astronomical potential as implemented weather/climate effects.
export interface CoreEarthSolarObservation {
  available: boolean;
  source?: 'CoreEarthSolar';
  worldSeed?: string;
  generationVersion?: number;
  regionId?: string;
  minute?: number;
  latitudeDeg?: number;
  longitudeDeg?: number;
  localSolarHour?: number;
  solarElevationDeg?: number;
  daylightHours?: number;
  sunlightAboveHorizon?: boolean;
  sunshinePotential01?: number;
  regionalWeatherApplied?: false;
}

export const UNBOUND_EARTH_SOLAR: CoreEarthSolarObservation = { available: false };

export function readCoreEarthSolar(
  raw: unknown, world: WorldOverview, scenario: CoreEarthScenarioObservation,
): CoreEarthSolarObservation {
  if (!raw || typeof raw !== 'object' || Array.isArray(raw)
    || scenario.available !== true || world.worldSeed == null) {
    return UNBOUND_EARTH_SOLAR;
  }
  const x = raw as Record<string, unknown>;
  const matches = (value: unknown, expected: unknown): boolean =>
    typeof value === 'number' && typeof expected === 'number'
    && Number.isFinite(value) && Number.isFinite(expected)
    && Math.abs(value - expected) < 1e-8;

  if (x.available !== true || x.source !== 'CoreEarthSolar'
    || x.worldSeed !== String(world.worldSeed)
    || x.worldSeed !== scenario.worldSeed
    || x.generationVersion !== world.generationVersion
    || x.generationVersion !== scenario.generationVersion
    || x.regionId !== scenario.regionId
    || typeof x.minute !== 'number'
    || !Number.isSafeInteger(x.minute) || x.minute !== world.minute
    || !matches(x.latitudeDeg, scenario.latitudeDeg)
    || !matches(x.longitudeDeg, scenario.longitudeDeg)
    || typeof x.localSolarHour !== 'number'
    || !Number.isFinite(x.localSolarHour)
    || x.localSolarHour < 0 || x.localSolarHour >= 24
    || typeof x.solarElevationDeg !== 'number'
    || !Number.isFinite(x.solarElevationDeg)
    || x.solarElevationDeg < -90 || x.solarElevationDeg > 90
    || typeof x.daylightHours !== 'number'
    || !Number.isFinite(x.daylightHours)
    || x.daylightHours < 0 || x.daylightHours > 24
    || typeof x.sunlightAboveHorizon !== 'boolean'
    || typeof x.sunshinePotential01 !== 'number'
    || !Number.isFinite(x.sunshinePotential01)
    || x.sunshinePotential01 < 0 || x.sunshinePotential01 > 1
    || x.regionalWeatherApplied !== false) {
    return UNBOUND_EARTH_SOLAR;
  }
  return x as unknown as CoreEarthSolarObservation;
}

export function formatSolarHour(value: number): string {
  if (!Number.isFinite(value) || value < 0 || value >= 24) return '—';
  const totalMinutes = Math.floor(value * 60 + 1e-7);
  return `${String(Math.floor(totalMinutes / 60)).padStart(2, '0')}:${String(totalMinutes % 60).padStart(2, '0')}`;
}
