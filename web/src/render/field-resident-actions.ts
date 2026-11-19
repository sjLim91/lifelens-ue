import type { PhysicalArtifact, Resident, ResidentPresentationDirective } from '../runtime/core-types';

export type FieldInteractionPose =
  'idle'|'work'|'sleep'|'gather'|'build'|'craft'|'eat'|'drink'|'wash'|'tend';
export interface FieldResidentAction {
  pose:FieldInteractionPose;
  /** Only explicitly selected working artifacts. Inventory ownership alone is not equip evidence. */
  tool:PhysicalArtifact|null;
}

export function fieldLifeStageScale(resident:Resident):number {
  const stages:Record<string,number>={
    Baby:.48,Toddler:.61,Child:.79,Teen:.93,YoungAdult:1,
    Adult:1,MiddleAge:.98,Elderly:.94,
  };
  if(resident.lifeStage) return stages[resident.lifeStage]??1;
  const a=resident.ageYears;
  if(typeof a!=='number'||!Number.isFinite(a)||a<0)return 1;
  if(a<2)return .48;
  if(a<6)return .61;
  if(a<13)return .79;
  if(a<18)return .93;
  return a>=70?.94:1;
}

function nearReportedSite(p:ResidentPresentationDirective,x:number,y:number):boolean {
  // Local actions have no location; site-directed poses must be backed by
  // proximity to the very Core target (not an imaginary nearby workbench).
  if(p.hasTargetGrid!==true||typeof p.targetGridX!=='number'
    ||typeof p.targetGridY!=='number'
    ||!Number.isFinite(p.targetGridX)||!Number.isFinite(p.targetGridY))return false;
  return Number.isFinite(x)&&Number.isFinite(y)
    &&Math.hypot(p.targetGridX-x,p.targetGridY-y)<=3.5;
}

function selectedTool(p:ResidentPresentationDirective):PhysicalArtifact|null {
  const a=p.artifactTool;
  // Match existing activeArtifact() evidence; no implicit equipped inventory.
  if(p.civilizationIntent!=='Gather'
    ||typeof p.contextActionToken!=='string'
    ||!/^([1-9]\d{0,19})$/.test(p.contextActionToken)
    ||!a||typeof a.id!=='string'||!a.id||!Number.isFinite(a.durability)
    ||a.durability<=.02||!Array.isArray(a.parts)||a.parts.length<1
    ||a.parts.length>4
    ||a.parts.some(part=>!Number.isInteger(part.shape)
      ||part.shape<0||part.shape>5||!Number.isInteger(part.size)
      ||part.size<1||part.size>3||typeof part.material!=='string'))return null;
  return a;
}

const none:FieldResidentAction={pose:'idle',tool:null};
/** Core event/phase only: no simulated items, no promised outcomes. */
export function fieldResidentAction(resident:Resident,x:number,y:number):FieldResidentAction {
  const p=resident.presentation;
  if(resident.alive===false||!p?.active||p.phase!=='Interacting')return none;
  if(p.kind==='Physical') {
    switch(p.physicalGoal){
      case 'Sleep':return {pose:'sleep',tool:null};
      case 'Eat':return {pose:'eat',tool:null};
      case 'Drink':return {pose:'drink',tool:null};
      case 'Wash':return {pose:'wash',tool:null};
      default:return none;
    }
  }
  if(p.kind!=='Civilization')return none;
  const proximate=nearReportedSite(p,x,y);
  if(p.civilizationIntent==='Gather') {
    if(!proximate||!p.civilizationResourceNode
      ||!p.civilizationMaterial||p.civilizationMaterial==='Unknown')return none;
    return {pose:'gather',tool:selectedTool(p)};
  }
  if(p.civilizationIntent==='Craft') {
    const action=p.facilityAction;
    if((action==='Work'||action==='Repair')&&proximate
      &&p.facilityId&&p.facilityId!=='0')return {pose:'build',tool:null};
    if(['Plant','Water','Tend','Harvest'].includes(action??'')
      &&proximate&&p.facilityKind==='CultivatedPlot'
      &&p.facilityId&&p.facilityId!=='0')return {pose:'tend',tool:null};
    // Facility craft or explicit local technique without fabricating an asset.
    if(!action||action==='None') {
      const local=p.hasTargetGrid===false && !!p.civilizationTechnique
        &&p.civilizationTechnique!=='None'
        &&typeof p.contextActionToken==='string' &&p.contextActionToken!=='0';
      const facility=proximate&&p.facilityKind==='WorkSurface'
        &&p.facilityId&&p.facilityId!=='0';
      if(local||facility)return {pose:'craft',tool:null};
    }
    if(proximate&&p.facilityId&&p.facilityId!=='0'
      &&['Fuel','Ignite','CollectCharcoal','LoadSmeltCharge',
        'CollectMetal','DeliverMaterial'].includes(action??''))
      return {pose:'work',tool:null};
  }
  if((p.civilizationIntent==='Store'||p.civilizationIntent==='Retrieve')
    &&proximate&&p.civilizationStorage)return {pose:'work',tool:null};
  return none;
}

/** Each part is faithfully 2D-rendered from Core structural shape/material. */
export function drawFieldArtifact(ctx:CanvasRenderingContext2D,a:PhysicalArtifact):void {
  const colors:Record<string,string>={
    Wood:'#715038',Fiber:'#b69c6b',Hide:'#96744c',Clay:'#ad7654',
    Flint:'#65716d',Bronze:'#a58648',CopperMetal:'#b5764f',
  };
  ctx.save();ctx.translate(26,-56);ctx.rotate(-.21);
  for(const [i,p] of a.parts.entries()){
    const s=p.size;
    const y=-i*3.5;
    ctx.fillStyle=colors[p.material]??'#858a89';
    ctx.strokeStyle='#454d47';ctx.lineWidth=1.3;
    ctx.beginPath();
    switch(p.shape){
      case 0:ctx.rect(-3,y-23*s,6,23*s);break;
      case 1:ctx.moveTo(0,y-11*s);ctx.lineTo(10*s,y-2);
        ctx.lineTo(0,y+2);ctx.lineTo(-10*s,y-2);ctx.closePath();break;
      case 2:ctx.moveTo(0,y-17*s);ctx.lineTo(7*s,y+2);
        ctx.lineTo(-7*s,y+2);ctx.closePath();break;
      case 3:ctx.ellipse(0,y-5*s,6*s,6*s,0,0,Math.PI*2);break;
      case 4:ctx.ellipse(0,y-3*s,7*s,2.8*s,0,0,Math.PI*2);break;
      case 5:ctx.arc(0,y-5*s,4*s,0,Math.PI*2);break;
    }
    ctx.fill();ctx.stroke();
  }
  ctx.restore();
}
