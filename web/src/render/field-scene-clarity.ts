import type { CivilizationWorldPayload, HumanTrace, Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { ResidentContinuity } from '../runtime/resident-continuity';
import { visibleHumanTraces } from '../state/human-traces';
import { stableNatureUnit } from './isometric-natural-presentation';
import type { FieldSpriteName } from './field-asset-atlas';

/**
 * Independent, well-dispersed visual placement stream.
 * Adjacent salts in raw seeded FNV correlated strongly, producing columns
 * of identical trees; avalanche diffusion fixes that without random calls.
 */
export function fieldVisualUnit(seed:string,x:number,y:number,salt:number):number {
  let h=Math.floor(stableNatureUnit(seed,x,y,salt)*4294967296)>>>0;
  h^=h>>>16;h=Math.imul(h,0x7feb352d);
  h^=h>>>15;h=Math.imul(h,0x846ca68b);
  h^=h>>>16;
  return (h>>>0)/4294967296;
}

export type FacilityTrace = Extract<HumanTrace, {kind:'Facility'}>;
export interface FieldFacilityArt {
  sprite: FieldSpriteName;
  size: number;
  /** Visual-only space to keep residents/items readable. In Core chunks. */
  clearance: number;
  operational: boolean;
}

/** Never describe a storage/worksite/sleeping mat as a completed house. */
export function fieldFacilityArt(trace: FacilityTrace): FieldFacilityArt {
  const kinds: Record<string, {sprite:FieldSpriteName;size:number;clearance:number}> = {
    Shelter:{sprite:'hut',size:.34,clearance:.18},
    PrimitiveStorage:{sprite:'store',size:.26,clearance:.12},
    WorkSurface:{sprite:'workbench',size:.27,clearance:.13},
    SleepingPlace:{sprite:'sleeping',size:.26,clearance:.12},
    FirePit:{sprite:'fire',size:.22,clearance:.11},
    Furnace:{sprite:'furnace',size:.30,clearance:.14},
    CultivatedPlot:{sprite:'farm',size:.34,clearance:.18},
  };
  // Unfinished, ruined or unknown facilities must not visually claim that
  // an operational building exists. The selectable construction site is real.
  const operational = trace.state === 'Operational';
  if(!operational) return {sprite:'site',size:.29,clearance:.12,operational:false};
  return {...(kinds[trace.facilityKind]??{
    sprite:'site' as FieldSpriteName,size:.29,clearance:.12}),operational:true};
}

export interface FieldClearance {
  x:number; y:number; radius:number;
  category:'resident'|'facility'|'settlement'|'animal';
  id:string;
}

/**
 * Protect the *visibility* of actually observed residents/structures; Core
 * positions are never adjusted, and these radii do not affect pathfinding.
 */
export function fieldCoreClearances(
  terrain:TerrainWindow,
  residents:Resident[],
  civilization:CivilizationWorldPayload,
  continuity:ResidentContinuity,
):FieldClearance[] {
  const span=WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const out:FieldClearance[]=[];
  const push=(id:string,category:FieldClearance['category'],gx:number,gy:number,radius:number)=>{
    if(Number.isFinite(gx)&&Number.isFinite(gy))out.push({
      id,category,x:gx/span,y:gy/span,radius,
    });
  };
  for(const trace of visibleHumanTraces(terrain)){
    if(trace.kind!=='Facility')continue;
    push(trace.id,'facility',trace.gridX,trace.gridY,fieldFacilityArt(trace).clearance);
  }
  for(const r of residents){
    const p=continuity.positionFor(r);
    if(p)push(r.id,'resident',p.x,p.y,.13);
  }
  for(const settlement of civilization.available?civilization.settlements??[]:[]){
    if(settlement.active)push(settlement.id,'settlement',settlement.gridX,settlement.gridY,.09);
  }
  for(const animal of terrain.wildlife?.available?terrain.wildlife.animals:[]){
    push(animal.id,'animal',animal.gridX,animal.gridY,.09);
  }
  return out;
}

export function fieldDecorativePositionClear(
  x:number,y:number,clearances:readonly FieldClearance[],spriteRadius:number,
):boolean {
  return clearances.every(site=>{
    const dx=x-site.x,dy=y-site.y;
    const distance=site.radius+spriteRadius;
    return dx*dx+dy*dy>=distance*distance;
  });
}

export function fieldSpriteVisualOcclusion(
  sprite:{x:number;y:number;w:number;h:number;name:FieldSpriteName},
  residents:readonly {x:number;y:number;selected:boolean}[],
): number {
  if(!['pine','cedar','oak','bush','cliff','rock','hut','store',
    'workbench','sleeping','furnace','site'].includes(sprite.name))return 1;
  for(const person of residents){
    // Don't fade scenery behind the person; only foreground elements whose
    // visible projected rectangles obscure the actual resident.
    if(sprite.y < person.y - 8)continue;
    if(Math.abs(person.x-sprite.x)>sprite.w*.42)continue;
    const top=sprite.y-sprite.h*.93;
    if(person.y < top-14 || person.y > sprite.y+12)continue;
    return person.selected?.23:.52;
  }
  return 1;
}
