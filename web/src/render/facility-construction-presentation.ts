import type { CivilizationWorldFacility, CivilizationWorldPayload, HumanTrace, Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { WORLD_PRESENTATION } from './world-presentation-config';
import { visibleHumanTraces, MAX_VISIBLE_HUMAN_TRACES } from '../state/human-traces';

export type FacilityTrace = Extract<HumanTrace, { kind: 'Facility' }>;
export type FacilityVisualPhase = 'Planned' | 'AwaitingMaterials' | 'MaterialsArriving'
  | 'ReadyForWork' | 'EarlyConstruction' | 'Framing' | 'Finishing'
  | 'Operational' | 'Worn' | 'SeverelyWorn' | 'Ruined';
const C = WORLD_PRESENTATION.construction;
const positive = (v: unknown): number => Number.isFinite(Number(v)) ? Math.max(0, Number(v)) : 0;
export const facilityClamp01 = (v: unknown): number => Math.min(1, positive(v));

export function partReveal(progress: number, start: number, finish: number): number {
  const t = facilityClamp01((progress - start) / Math.max(0.001, finish - start));
  return t * t * (3 - 2 * t);
}
export function matchingFacility(trace: FacilityTrace, facility?: CivilizationWorldFacility): CivilizationWorldFacility | undefined {
  return facility && trace.id === `facility:${facility.id}` && trace.facilityKind === facility.kind
    && trace.gridX === facility.gridX && trace.gridY === facility.gridY ? facility : undefined;
}

// Trace is the fast current read model; detail supplies only fields it lacks.
// Both Core progress fields are constructionWork / requiredWork. Never overwrite
// a current trace using the slower civilization-detail snapshot.
export function deriveFacilityVisualState(trace: FacilityTrace, detail?: CivilizationWorldFacility, activity?: string) {
  const facility = matchingFacility(trace, detail);
  const operational = trace.state === 'Operational', ruined = trace.state === 'Ruined';
  const workProgress = operational || ruined ? 1 : facilityClamp01(trace.progress01);
  const visualProgress = Math.floor(workProgress * C.progressSteps) / C.progressSteps;
  const required = positive(trace.requiredMaterialUnits), delivered = positive(trace.deliveredMaterialUnits);
  const materialProgress = required > 0 ? facilityClamp01(delivered / required) : 0;
  const knownDurability = facility?.state === trace.state && Number.isFinite(facility.durability);
  const durability = knownDurability ? facilityClamp01(facility!.durability) : 1;
  const durabilityBand = !operational ? 0 : durability > C.freshDurability ? 0
    : durability > C.wornDurability ? 1 : durability > C.severeDurability ? 2 : 3;
  const repairing = activity === 'Repair';
  let phase: FacilityVisualPhase;
  if (ruined) phase = 'Ruined';
  else if (operational) phase = durabilityBand >= 2 ? 'SeverelyWorn' : durabilityBand === 1 ? 'Worn' : 'Operational';
  else if (workProgress > 0) phase = workProgress < C.earlyWorkEnd ? 'EarlyConstruction'
    : workProgress < C.framingEnd ? 'Framing' : 'Finishing';
  else if (required > 0 && delivered >= required) phase = 'ReadyForWork';
  else if (delivered > 0) phase = 'MaterialsArriving';
  else phase = trace.state === 'Planned' ? 'Planned' : 'AwaitingMaterials';
  return { phase, workProgress, visualProgress, materialProgress, durabilityBand,
    repairing, operational, ruined, active: trace.active === true };
}

export interface ConstructionPile { material: string; fill: number }
export function constructionMaterialPiles(trace: FacilityTrace, detail?: CivilizationWorldFacility): ConstructionPile[] {
  if (trace.state === 'Operational' || trace.state === 'Ruined') return [];
  const required = positive(trace.requiredMaterialUnits), delivered = positive(trace.deliveredMaterialUnits);
  if (required <= 0 || delivered <= 0) return [];
  const facility = matchingFacility(trace, detail), rows = facility?.requirements ?? [];
  // Detail can lag trace delivery. Use typed materials only when exact totals
  // and state agree; otherwise keep a conservative generic raw-material pile.
  const exact = facility?.state === trace.state && rows.length > 0
    && rows.reduce((n, r) => n + positive(r.required), 0) === required
    && rows.reduce((n, r) => n + positive(r.delivered), 0) === delivered;
  const portions = exact ? rows.filter(r => positive(r.delivered) > 0)
    .map(r => ({ material: r.material, units: Math.min(positive(r.delivered), positive(r.required)) }))
    .sort((a, b) => a.material.localeCompare(b.material))
    : [{ material: 'Unknown', units: Math.min(delivered, required) }];
  const piles: ConstructionPile[] = [];
  for (const portion of portions) {
    let remaining = portion.units;
    while (remaining > 0 && piles.length < C.pileSlots) {
      const units = Math.min(remaining, required / C.pileSlots);
      piles.push({ material: portion.material, fill: units / (required / C.pileSlots) });
      remaining -= units;
    }
  }
  return piles;
}

// Core omits unstarted plans from HumanTrace; a real civilization facility
// can still supply its known site. This is a DTO projection, not creation.
export function facilityPresentationTraces(window: TerrainWindow, civilization: CivilizationWorldPayload): FacilityTrace[] {
  const traces = visibleHumanTraces(window).filter((t): t is FacilityTrace => t.kind === 'Facility');
  const ids = new Set(traces.map(t => t.id));
  if (window.available !== true || civilization.available !== true) return traces;
  const chunks = new Set(window.chunks.map(c => `${c.x}:${c.y}`));
  for (const f of civilization.facilities ?? []) {
    if (traces.length >= MAX_VISIBLE_HUMAN_TRACES) break;
    const id = `facility:${f.id}`;
    if (ids.has(id) || f.state !== 'Planned' || f.constructionWork > 0 || f.deliveredMaterialUnits > 0
      || !Number.isFinite(f.gridX) || !Number.isFinite(f.gridY)) continue;
    const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
    if (!chunks.has(`${Math.floor(f.gridX / span)}:${Math.floor(f.gridY / span)}`)) continue;
    traces.push({ id, kind: 'Facility', gridX: f.gridX, gridY: f.gridY, sourceResidentId: f.initiatedBy,
      facilityKind: f.kind, state: f.state, progress01: f.workProgress,
      deliveredMaterialUnits: f.deliveredMaterialUnits, requiredMaterialUnits: f.requiredMaterialUnits,
      active: f.active, lit: f.lit, cropPlanted: f.cropPlanted, cropGrowth01: f.cropGrowth01,
      cropMoisture01: f.cropMoisture01, cropCare01: f.cropCare01, cropHarvestUnits: f.cropHarvestUnits });
    ids.add(id);
  }
  return traces;
}

export function facilityActivitySites(residents: Resident[]): Map<string, 'Work' | 'Repair' | 'DeliverMaterial'> {
  const sites = new Map<string, 'Work' | 'Repair' | 'DeliverMaterial'>();
  for (const resident of residents) {
    const p = resident.presentation;
    if (resident.alive === false || !resident.hasPosition || !p?.active || p.kind !== 'Civilization'
      || p.phase !== 'Interacting' || !p.facilityId || p.facilityId === '0') continue;
    if (p.facilityAction !== 'Work' && p.facilityAction !== 'Repair' && p.facilityAction !== 'DeliverMaterial') continue;
    const id = `facility:${p.facilityId}`;
    const previous = sites.get(id);
    if (previous !== 'Repair' && (previous !== 'Work' || p.facilityAction === 'Repair')) {
      sites.set(id, p.facilityAction);
    }
  }
  return sites;
}
