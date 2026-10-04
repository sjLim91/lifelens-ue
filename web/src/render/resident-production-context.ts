import type { CivilizationWorldPayload, CivilizationWorldFacility, CivilizationWorldResource,
  CivilizationWorldStorage, ResidentPresentationDirective } from '../runtime/core-types';
import { knownProductionMaterial } from './production-material-presentation';
import { WORLD_PRESENTATION } from './world-presentation-config';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

export const PRODUCTION_INTENTS = ['None','Gather','Store','Retrieve','Craft','Experiment','Explore'] as const;
export const PRODUCTION_ACTIONS = ['None','Plan','DeliverMaterial','Work','Repair','Fuel','Ignite',
  'CollectCharcoal','LoadSmeltCharge','CollectMetal','Plant','Water','Tend','Harvest'] as const;
export function registeredProductionDirective(p: ResidentPresentationDirective): boolean {
  return PRODUCTION_INTENTS.some(v => v === (p.civilizationIntent ?? 'None'))
    && PRODUCTION_ACTIONS.some(v => v === (p.facilityAction ?? 'None'));
}
export interface ProductionTarget { kind: 'resource'|'storage'|'facility'|'site'; id: string;
  gridX: number; gridY: number; accessX: number; accessY: number; }
const FACILITY_KINDS = ['PrimitiveStorage','FirePit','WorkSurface','SleepingPlace','Shelter','Furnace','CultivatedPlot'];
const positioned = (s: {gridX?:number;gridY?:number}|undefined): boolean =>
  !!s && Number.isFinite(s.gridX) && Number.isFinite(s.gridY);
const target = (kind: ProductionTarget['kind'], s: {id:string;gridX:number;gridY:number}, x=s.gridX,y=s.gridY): ProductionTarget =>
  ({kind,id:s.id,gridX:s.gridX,gridY:s.gridY,accessX:x,accessY:y});

/** Current DTO index only, replaced on refresh. No outcome/history/nearest-site inference. */
export class ResidentProductionTargets {
  private resources = new Map<string,CivilizationWorldResource>();
  private facilities = new Map<string,CivilizationWorldFacility>();
  private storages = new Map<string,CivilizationWorldStorage>();
  setSnapshot(world: CivilizationWorldPayload): void {
    this.clear(); if(world.available !== true)return;
    for(const r of world.resources ?? [])if(positioned(r))this.resources.set(r.id,r);
    for(const f of world.facilities ?? [])if(positioned(f))this.facilities.set(f.id,f);
    for(const s of world.storages ?? [])if(positioned(s))this.storages.set(s.id,s);
  }
  clear(): void {this.resources.clear();this.facilities.clear();this.storages.clear();}
  resolve(p: ResidentPresentationDirective|null|undefined): ProductionTarget|null {
    if(!p?.active || p.kind !== 'Civilization' || !registeredProductionDirective(p)
      || !['Moving','Interacting'].includes(p.phase ?? '') || !p.hasTargetGrid || !Number.isFinite(p.targetGridX) || !Number.isFinite(p.targetGridY))return null;
    let result: ProductionTarget|null=null;
    if(p.civilizationIntent === 'Gather') {
      const r=this.resources.get(p.civilizationResourceNode ?? '');
      if(!r || !knownProductionMaterial(p.civilizationMaterial) || r.material !== p.civilizationMaterial
        || !Number.isFinite(r.quantity) || r.quantity<=0)return null;
      const x=r.hasAccessGrid && Number.isFinite(r.accessGridX)?r.accessGridX!:r.gridX;
      const y=r.hasAccessGrid && Number.isFinite(r.accessGridY)?r.accessGridY!:r.gridY;
      result=target('resource',r,x,y);
    } else if(p.civilizationIntent === 'Store' || p.civilizationIntent === 'Retrieve') {
      const s=this.storages.get(p.civilizationStorage ?? '');
      if(!s || !knownProductionMaterial(p.civilizationMaterial))return null;
      result=target('storage',s);
    } else if(p.facilityId && p.facilityId !== '0') {
      const f=this.facilities.get(p.facilityId);
      if(!f || !FACILITY_KINDS.includes(f.kind) || f.kind !== p.facilityKind || !['Planned','UnderConstruction','Operational','Ruined'].includes(f.state))return null;
      const a=p.facilityAction ?? 'None';
      if(['Plant','Water','Tend','Harvest'].includes(a) && (f.kind !== 'CultivatedPlot' || f.state !== 'Operational' || !f.active))return null;
      if(['LoadSmeltCharge','CollectMetal'].includes(a) && (f.kind !== 'Furnace' || f.state !== 'Operational' || !f.active))return null;
      if(['Fuel','Ignite','CollectCharcoal'].includes(a) && (!['FirePit','Furnace'].includes(f.kind) || f.state !== 'Operational' || !f.active))return null;
      if(['Work','DeliverMaterial'].includes(a) && !['Planned','UnderConstruction'].includes(f.state))return null;
      if(a === 'None' && p.civilizationIntent === 'Craft' && (f.kind !== 'WorkSurface' || f.state !== 'Operational' || !f.active))return null;
      result=target('facility',f);
    } else if((p.facilityAction === 'Plan' && FACILITY_KINDS.includes(p.facilityKind ?? '')) || p.civilizationIntent === 'Explore' || p.civilizationIntent === 'Experiment') {
      // Core-authored prospective site/exploration/sanitation locus, not a building.
      result=target('site',{id:'',gridX:p.targetGridX!,gridY:p.targetGridY!});
    }
    if(!result)return null;
    const C=WORLD_PRESENTATION.production;
    if(Math.hypot(p.targetGridX!-result.accessX,p.targetGridY!-result.accessY)>C.targetGridTolerance)return null;
    return result;
  }
  interacting(p: ResidentPresentationDirective|null|undefined, x:number,z:number,centerX:number,centerY:number): boolean {
    if(p?.phase !== 'Interacting')return false;
    const t=this.resolve(p);if(!t)return false;
    const {gridCellsPerChunk:span,worldUnitsPerChunk:size}=WORLD_GRID_CONTRACT;
    const wx=(t.accessX/span-centerX-.5)*size,wz=(t.accessY/span-centerY-.5)*size;
    if(Math.hypot(x-wx,z-wz)>WORLD_PRESENTATION.production.interactionDistance)return false;
    if(t.kind === 'resource' && Math.hypot(t.gridX-t.accessX,t.gridY-t.accessY)*size/span
      > WORLD_PRESENTATION.production.gatherReachDistance)return false;
    return true;
  }
}
