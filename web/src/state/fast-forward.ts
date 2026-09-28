import type {
  CivilizationDiscovery,
  CivilizationWorldFacility,
  CivilizationWorldPayload,
  Resident,
  ResidentLifeEvent,
  WorldObjectsPayload,
  WorldOverview,
} from '../runtime/core-types';

export const MAX_FAST_FORWARD_DAYS = 3650;
export const FAST_FORWARD_CHUNK_MINUTES = 360;
export const MINUTES_PER_DAY = 1440;

export type FastForwardStatus = 'idle' | 'running' | 'complete' | 'error';

export interface FastForwardFacilityChange {
  id: string;
  kind: string;
  fromState?: string;
  toState: string;
  completedMinute?: number;
}

export interface FastForwardResourceChange {
  material: string;
  before: number;
  after: number;
  delta: number;
}

export interface FastForwardLifeEvent {
  residentId: string;
  residentName: string;
  type: string;
  minute: number;
}

export interface FastForwardSummary {
  requestedDays: number;
  startMinute: number;
  endMinute: number;
  totalResidentsBefore: number;
  totalResidentsAfter: number;
  livingResidentsBefore: number;
  livingResidentsAfter: number;
  deceasedResidentsBefore: number;
  deceasedResidentsAfter: number;
  householdsBefore: number;
  householdsAfter: number;
  couplesBefore: number;
  couplesAfter: number;
  pregnanciesBefore: number;
  pregnanciesAfter: number;
  majorLifeEventsBefore: number;
  majorLifeEventsAfter: number;
  storedUnitsBefore: number;
  storedUnitsAfter: number;
  facilitiesBefore: number;
  facilitiesAfter: number;
  techniqueFactsBefore: number;
  techniqueFactsAfter: number;
  newResidents: Array<{ id: string; name: string }>;
  newlyDeceased: Array<{ id: string; name: string }>;
  facilityChanges: FastForwardFacilityChange[];
  newDiscoveries: CivilizationDiscovery[];
  lifeEvents: FastForwardLifeEvent[];
  resourceChanges: FastForwardResourceChange[];
  sanitationSitesBefore: number;
  sanitationSitesAfter: number;
}

export interface FastForwardState {
  status: FastForwardStatus;
  requestedDays: number;
  completedMinutes: number;
  totalMinutes: number;
  progress01: number;
  summary: FastForwardSummary | null;
  errorMessage: string | null;
}

export const INITIAL_FAST_FORWARD_STATE: FastForwardState = {
  status: 'idle',
  requestedDays: 0,
  completedMinutes: 0,
  totalMinutes: 0,
  progress01: 0,
  summary: null,
  errorMessage: null,
};

export interface FastForwardCapture {
  world: WorldOverview;
  residents: Resident[];
  civilization: CivilizationWorldPayload;
  worldObjects: WorldObjectsPayload;
}

function numeric(value: unknown): number {
  return Number.isFinite(Number(value)) ? Number(value) : 0;
}

function lifeEventKey(event: ResidentLifeEvent): string {
  return [
    event.type ?? '',
    numeric(event.minute),
    ...(event.relatedCharacterIds ?? []),
    numeric(event.value),
  ].join(':');
}

function resourceTotals(
  payload: CivilizationWorldPayload,
): Map<string, number> {
  const totals = new Map<string, number>();
  for (const resource of payload.resources ?? []) {
    const material = resource.material || 'Unknown';
    totals.set(
      material,
      (totals.get(material) ?? 0) + numeric(resource.quantity),
    );
  }
  return totals;
}

function discoveryKey(discovery: CivilizationDiscovery): string {
  return discovery.factId
    || [
      discovery.technique,
      discovery.discovererId,
      discovery.minute,
    ].join(':');
}

function facilityById(
  facilities: CivilizationWorldFacility[] | undefined,
): Map<string, CivilizationWorldFacility> {
  return new Map((facilities ?? []).map((facility) => [facility.id, facility]));
}

export function captureFastForwardState(input: FastForwardCapture): FastForwardCapture {
  return {
    world: structuredClone(input.world),
    residents: structuredClone(input.residents),
    civilization: structuredClone(input.civilization),
    worldObjects: structuredClone(input.worldObjects),
  };
}

export function buildFastForwardSummary(
  requestedDays: number,
  before: FastForwardCapture,
  after: FastForwardCapture,
): FastForwardSummary {
  const beforeResidents = new Map(before.residents.map((resident) => [resident.id, resident]));
  const afterResidents = new Map(after.residents.map((resident) => [resident.id, resident]));

  const newResidents = after.residents
    .filter((resident) => !beforeResidents.has(resident.id))
    .map((resident) => ({ id: resident.id, name: resident.name }));

  const newlyDeceased = after.residents
    .filter((resident) => {
      const previous = beforeResidents.get(resident.id);
      return previous?.alive !== false && resident.alive === false;
    })
    .map((resident) => ({ id: resident.id, name: resident.name }));

  const beforeFacilities = facilityById(before.civilization.facilities);
  const facilityChanges: FastForwardFacilityChange[] = [];
  for (const facility of after.civilization.facilities ?? []) {
    const previous = beforeFacilities.get(facility.id);
    if (!previous) {
      facilityChanges.push({
        id: facility.id,
        kind: facility.kind,
        toState: facility.state,
        completedMinute: facility.completedMinute,
      });
      continue;
    }
    if (previous.state !== facility.state) {
      facilityChanges.push({
        id: facility.id,
        kind: facility.kind,
        fromState: previous.state,
        toState: facility.state,
        completedMinute: facility.completedMinute,
      });
    }
  }

  const beforeDiscoveries = new Set(
    (before.civilization.recentDiscoveries ?? []).map(discoveryKey),
  );
  const newDiscoveries = (after.civilization.recentDiscoveries ?? [])
    .filter((discovery) => !beforeDiscoveries.has(discoveryKey(discovery)));

  const beforeLifeEvents = new Map<string, Set<string>>();
  for (const resident of before.residents) {
    beforeLifeEvents.set(
      resident.id,
      new Set((resident.lifeHistory ?? []).map(lifeEventKey)),
    );
  }

  const lifeEvents: FastForwardLifeEvent[] = [];
  for (const resident of after.residents) {
    const previousEvents = beforeLifeEvents.get(resident.id) ?? new Set<string>();
    for (const event of resident.lifeHistory ?? []) {
      if (previousEvents.has(lifeEventKey(event))) continue;
      lifeEvents.push({
        residentId: resident.id,
        residentName: resident.name,
        type: event.type ?? '',
        minute: numeric(event.minute),
      });
    }
  }
  lifeEvents.sort((left, right) => left.minute - right.minute);

  const beforeResources = resourceTotals(before.civilization);
  const afterResources = resourceTotals(after.civilization);
  const materials = new Set([
    ...beforeResources.keys(),
    ...afterResources.keys(),
  ]);
  const resourceChanges: FastForwardResourceChange[] = [];
  for (const material of materials) {
    const beforeQuantity = beforeResources.get(material) ?? 0;
    const afterQuantity = afterResources.get(material) ?? 0;
    const delta = afterQuantity - beforeQuantity;
    if (Math.abs(delta) < 0.5) continue;
    resourceChanges.push({
      material,
      before: beforeQuantity,
      after: afterQuantity,
      delta,
    });
  }
  resourceChanges.sort(
    (left, right) => Math.abs(right.delta) - Math.abs(left.delta),
  );

  return {
    requestedDays,
    startMinute: numeric(before.world.minute),
    endMinute: numeric(after.world.minute),
    totalResidentsBefore: numeric(before.world.totalResidents),
    totalResidentsAfter: numeric(after.world.totalResidents),
    livingResidentsBefore: numeric(before.world.livingResidents),
    livingResidentsAfter: numeric(after.world.livingResidents),
    deceasedResidentsBefore: numeric(before.world.deceasedResidents),
    deceasedResidentsAfter: numeric(after.world.deceasedResidents),
    householdsBefore: numeric(before.world.households),
    householdsAfter: numeric(after.world.households),
    couplesBefore: numeric(before.world.activeCouples),
    couplesAfter: numeric(after.world.activeCouples),
    pregnanciesBefore: numeric(before.world.activePregnancies),
    pregnanciesAfter: numeric(after.world.activePregnancies),
    majorLifeEventsBefore: numeric(before.world.majorLifeEvents),
    majorLifeEventsAfter: numeric(after.world.majorLifeEvents),
    storedUnitsBefore: numeric(before.civilization.totalStoredUnits),
    storedUnitsAfter: numeric(after.civilization.totalStoredUnits),
    facilitiesBefore: numeric(before.civilization.facilityCount),
    facilitiesAfter: numeric(after.civilization.facilityCount),
    techniqueFactsBefore: numeric(before.civilization.techniqueFactCount),
    techniqueFactsAfter: numeric(after.civilization.techniqueFactCount),
    newResidents,
    newlyDeceased,
    facilityChanges,
    newDiscoveries,
    lifeEvents,
    resourceChanges,
    sanitationSitesBefore: (before.worldObjects.sanitationSites ?? []).length,
    sanitationSitesAfter: (after.worldObjects.sanitationSites ?? []).length,
  };
}
