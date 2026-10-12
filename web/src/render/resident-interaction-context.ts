import type { Resident, ResidentPresentationDirective, TerrainWindow, WorldObjectsPayload, WorldSmartObject } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';

/** Current physical facts only. Unknown/absent supplies suppress target-bound
 * gestures; playing a clip cannot acknowledge a token or consume an item. */
export class ResidentPhysicalTargets {
  private readonly objects = new Map<string, WorldSmartObject>();
  private readonly chunks = new Map<string, TerrainWindow['chunks'][number]>();
  setObjects(world: WorldObjectsPayload): void {
    this.objects.clear();if(world.available!==true)return;
    for(const o of world.smartObjects??[])this.objects.set(o.id,o);
  }
  setTerrain(terrain: TerrainWindow): void {
    this.chunks.clear();if(terrain.available!==true)return;
    for(const c of terrain.chunks)this.chunks.set(`${c.x}:${c.y}`,c);
  }
  allows(resident: Resident, p: ResidentPresentationDirective | null, x: number,z: number,centerX: number,centerY: number): boolean {
    if(!p?.active || p.kind!=='Physical' || p.phase!=='Interacting')return false;
    const {gridCellsPerChunk:span,worldUnitsPerChunk:size}=WORLD_GRID_CONTRACT;
    if(!p.hasTargetGrid || !Number.isFinite(p.targetGridX) || !Number.isFinite(p.targetGridY))return false;
    const tx=(p.targetGridX!/span-centerX-.5)*size,tz=(p.targetGridY!/span-centerY-.5)*size;
    if(Math.hypot(x-tx,z-tz)>.65)return false;
    if(p.hasObjectTarget){
      const object=this.objects.get(p.objectId??'');
      return !!object && object.kind===p.objectKind && Number.isFinite(object.gridX+object.gridY)
        && Math.hypot(object.gridX-p.targetGridX!,object.gridY-p.targetGridY!)<=1.5
        && (!object.reservedById || object.reservedById==='0' || object.reservedById===resident.id);
    }
    const hasMaterial=(material:string):boolean=>(resident.civilization?.inventory??[]).some(i=>i.item==='RawMaterial' && i.material===material && Number.isFinite(i.quantity) && i.quantity!>0);
    if(p.physicalGoal==='Eat')return hasMaterial('PlantFood');
    if(p.physicalGoal==='Drink' || p.physicalGoal==='Wash'){
      if(!p.directNaturalWaterSource)return hasMaterial('Water');
      const chunk=this.chunks.get(`${Math.floor(p.targetGridX!/span)}:${Math.floor(p.targetGridY!/span)}`);
      return !!chunk && !['None','Ocean','Coast'].includes(chunk.waterKind) && chunk.salinity!=='Salt' && chunk.salinity!=='Brackish'
        && Number.isFinite(chunk.waterAvailability) && chunk.waterAvailability>0;
    }
    if(p.physicalGoal==='Sleep')return true;
    return p.physicalGoal==='UseToilet' && (p.designatedSanitationSite===true || p.emergencyFallback===true);
  }
}
