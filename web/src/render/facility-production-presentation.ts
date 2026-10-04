import type { CivilizationWorldFacility } from '../runtime/core-types';
import { knownProductionMaterial } from './production-material-presentation';
import { WORLD_PRESENTATION } from './world-presentation-config';
export interface FacilityProductionStock { material:string; fill:number; }
/** Actual processing buffers only. No recipe inference or cosmetic consumption. */
export function facilityProductionStocks(f: CivilizationWorldFacility|undefined): FacilityProductionStock[] {
  if(!f || f.state !== 'Operational' || !['FirePit','Furnace'].includes(f.kind))return [];
  const rows = [
    {material:'Fuel',quantity:f.fuelUnits},
    {material:'Charcoal',quantity:f.charcoalUnits},
    ...(f.kind === 'Furnace' ? [{material:f.furnaceChargeMaterial,quantity:f.oreUnits},
      {material:f.furnaceOutputMaterial,quantity:f.metalUnits}]:[]),
  ];
  const C=WORLD_PRESENTATION.production;
  return rows.filter(r=>(r.material === 'Fuel' || knownProductionMaterial(r.material))
    && Number.isFinite(r.quantity) && r.quantity>0).slice(0,C.processingSlots)
    .map(r=>({material:r.material,
      fill:Math.ceil(Math.min(1,r.quantity/C.processingUnitsPerSlot)*C.processingSteps)/C.processingSteps}));
}
