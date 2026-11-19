import type { Resident } from '../runtime/core-types';
import { fieldVisualUnit } from './field-scene-clarity';

export type FieldFacing = 'E'|'SE'|'S'|'SW'|'W'|'NW'|'N'|'NE';
export type FieldActionPose = 'idle'|'walk'|'work'|'sleep';
export interface FieldResidentFrame {
  facing:FieldFacing;
  pose:FieldActionPose;
  /** 0..1, only active immediately after a real observed Core displacement. */
  step:number;
}
export interface FieldResidentPalette {
  skin:string;
  skinShadow:string;
  hair:string;
  hairLight:string;
  outfit:string;
  outfitShadow:string;
  trim:string;
  hairStyle:number;
  build:number;
  height:number;
}

const dirs:readonly FieldFacing[]=['E','SE','S','SW','W','NW','N','NE'];
const finite=(v:unknown):v is number=>typeof v==='number'&&Number.isFinite(v);
const safe01=(v:unknown,defaultValue=.5)=>finite(v)?Math.max(0,Math.min(1,v)):defaultValue;
const choose=<T>(list:readonly T[],unit:number)=>list[Math.min(list.length-1,Math.floor(unit*list.length))];
/** Sprite direction is discrete; Core movement and target remain untouched. */
export function fieldHeading(dx:number,dy:number):FieldFacing|null {
  if(!finite(dx)||!finite(dy)||Math.hypot(dx,dy)<.0001)return null;
  const sector=(Math.round(Math.atan2(dy,dx)*4/Math.PI)+8)%8;
  return dirs[sector];
}

/**
 * Rendered appearance is a stable, local interpretation of Core identity
 * and optional genetics: not a simulation of phenotypes/ethnicity.
 * No random() or clock dependency, stable across camera/reload/observer.
 */
export function fieldResidentPalette(r:Resident):FieldResidentPalette {
  const h=(salt:number)=>fieldVisualUnit(r.id,0,0,salt);
  const g=r.genetics??{};
  const skin=choose([
    ['#edd3b0','#cda27b'],['#e6bb94','#bd8b69'],
    ['#d8a47e','#af7457'],['#b98766','#926549'],
    ['#986d51','#79513f'],['#71503f','#593b31'],
  ],safe01(g.skinTone,h(111)));
  const hair=choose([
    ['#252a2b','#59615b'],['#39302b','#766253'],
    ['#5b3d2b','#977354'],['#75503a','#b38960'],
    ['#9b815f','#c8a97d'],['#5c5154','#a09a98'],
  ],safe01(g.hairPigment,h(112)));
  const clothing=choose([
    ['#687c61','#435d54','#bdc39a'],['#bd8167','#875a51','#e6c7a0'],
    ['#688a92','#426779','#c0d0bf'],['#9c8a62','#74664a','#e1cd98'],
    ['#98768a','#715b7a','#d8b4bc'],['#668b70','#3f6853','#e0d19c'],
    ['#8e6b54','#62503e','#c4b58f'],['#78829a','#4e607b','#c9d1e0'],
  ],h(113));
  return {
    skin:skin[0],skinShadow:skin[1],hair:hair[0],hairLight:hair[1],
    outfit:clothing[0],outfitShadow:clothing[1],trim:clothing[2],
    hairStyle:Math.floor(h(114)*4),
    build:.9+safe01(g.buildPotential,h(115))*.25,
    height:.92+safe01(g.heightPotential,h(116))*.19,
  };
}
interface MotionMemory {
  x:number;y:number;facing:FieldFacing;movementAt:number;lastSeenAt:number;
}
/**
 * A new Core position is evidence of displacement, but target/intent alone
 * is NOT evidence of walking. Large unseen jumps never animate as steps.
 * The short visual pulse expires even if the Core view is drawn every frame.
 */
export class FieldResidentMotion {
  private readonly positions=new Map<string,MotionMemory>();
  frame(r:Resident,x:number,y:number,nowMs:number):FieldResidentFrame {
    const validPosition=finite(x)&&finite(y);
    const prev=this.positions.get(r.id);
    const p=r.presentation;
    const targeting=p?.active===true&&p.phase==='Moving'
      &&p.hasTargetGrid===true&&finite(p.targetGridX)&&finite(p.targetGridY);
    const goal=targeting?fieldHeading(p!.targetGridX!-x,p!.targetGridY!-y):null;
    let facing=prev?.facing??goal??'S';
    let movementAt=prev?.movementAt??Number.NEGATIVE_INFINITY;
    if(validPosition) {
      if(prev) {
        const dx=x-prev.x,dy=y-prev.y,dist=Math.hypot(dx,dy);
        if(dist>.025 && dist<=3 && nowMs-prev.lastSeenAt<=5000) {
          facing=fieldHeading(dx,dy)??facing;
          movementAt=nowMs;
        } else if(dist>3 || nowMs-prev.lastSeenAt>5000) {
          // Teleport/fast-forward/stale observation: do not invent footsteps.
          movementAt=Number.NEGATIVE_INFINITY;
          facing=goal??facing;
        } else if(goal && nowMs-movementAt>360) facing=goal;
      }
      this.positions.set(r.id,{x,y,facing,movementAt,lastSeenAt:nowMs});
    }
    const recentMove=validPosition&&r.hasPosition!==false
      &&nowMs-movementAt>=0&&nowMs-movementAt<=330;
    if(recentMove) return {
      facing,pose:'walk',
      step:Math.min(1,Math.max(0,(nowMs-movementAt)/330)),
    };
    if(p?.active===true&&p.phase==='Interacting') {
      if(p.kind==='Physical'&&p.physicalGoal==='Sleep')
        return {facing,pose:'sleep',step:0};
      if(p.kind==='Civilization')
        return {facing,pose:'work',step:0};
    }
    return {facing,pose:'idle',step:0};
  }
  prune(residents:readonly Resident[]):void {
    const present=new Set(residents.filter(r=>r.alive!==false).map(r=>r.id));
    for(const id of this.positions.keys())if(!present.has(id))this.positions.delete(id);
  }
  reset():void {this.positions.clear();}
  get size():number{return this.positions.size;}
}

const ellipse=(ctx:CanvasRenderingContext2D,x:number,y:number,rx:number,ry:number,
  color:string,outline?:string)=>{
  ctx.beginPath();ctx.ellipse(x,y,rx,ry,0,0,Math.PI*2);
  ctx.fillStyle=color;ctx.fill();
  if(outline){ctx.strokeStyle=outline;ctx.lineWidth=1.8;ctx.stroke();}
};
const path=(ctx:CanvasRenderingContext2D,points:ReadonlyArray<readonly [number,number]>,
  fill:string,stroke?:string)=>{
  ctx.beginPath();ctx.moveTo(points[0][0],points[0][1]);
  for(let i=1;i<points.length;i++)ctx.lineTo(points[i][0],points[i][1]);
  ctx.closePath();ctx.fillStyle=fill;ctx.fill();
  if(stroke){ctx.lineWidth=2;ctx.strokeStyle=stroke;ctx.stroke();}
};
/**
 * Original close-up vector puppet. Eight facings have explicit front/back/
 * profile and diagonal face shapes, not just rotating a front-facing icon.
 * Apparent locomotion is a pose, NEVER movement of the Core footpoint.
 */
export function drawFieldResident(
  ctx:CanvasRenderingContext2D,
  r:Resident,
  x:number,y:number,width:number,height:number,
  frame:FieldResidentFrame,
):void {
  const a=fieldResidentPalette(r);
  const back=frame.facing==='N'||frame.facing==='NE'||frame.facing==='NW';
  const side=frame.facing==='E'||frame.facing==='W';
  const diagonal=['NE','NW','SE','SW'].includes(frame.facing);
  const left=frame.facing==='W'||frame.facing==='SW'||frame.facing==='NW';
  const walk=frame.pose==='walk';
  const swing=walk?Math.sin(frame.step*Math.PI*4)*5:0;
  const bob=walk?Math.abs(Math.sin(frame.step*Math.PI*4))*2:0;
  ctx.save();
  ctx.translate(x,y);
  ctx.scale(width/80*(left?-1:1),height/118);
  // An actual Core resident remains planted to the same ground coordinate.
  ellipse(ctx,0,-2,30,6,'rgba(22,37,29,.24)');
  if(frame.pose==='sleep') {
    // Sleeping occurs only when Core currently reports active Sleep/Interacting.
    ctx.rotate(-Math.PI*.39);
    ctx.translate(-12,-19);
  }
  ctx.translate(0,-bob);
  const outline='#3f4841',sole='#504c46';
  const calf1=-2+swing,calf2=-2-swing;
  // Rear leg / boot, articulated only on an observed short move.
  path(ctx,[[-17,-42],[-3,-41],[calf1-2,-8],[calf1-14,-7]],a.outfitShadow,outline);
  path(ctx,[[7,-42],[20,-41],[calf2+13,-8],[calf2+2,-7]],a.outfitShadow,outline);
  ellipse(ctx,calf1-8,-7,10,5,sole);
  ellipse(ctx,calf2+8,-7,10,5,sole);
  // Arms behind the torso, alternating swing for a genuine move.
  const work=frame.pose==='work';
  path(ctx,[[-19,-65],[-27,-62],[-29-swing*.35,-38],[-23-swing*.35,-32],
    [-13,-45]],a.skinShadow,outline);
  path(ctx,[[18,-65],[27,-61],[29+swing*.35,-39],[23+swing*.35,-34],
    [12,-45]],a.skin,outline);
  if(work){
    // Core-reported civilization interaction, no fabricated equipment.
    path(ctx,[[18,-64],[26,-63],[29,-78],[19,-82],[13,-65]],
      a.outfitShadow,outline);
    ellipse(ctx,24,-80,5.5,6,a.skin);
  }
  // Primary tunic and mantle; body type comes from stable genetic build.
  const spread=24*a.build;
  path(ctx,[[-spread*.7,-73],[spread*.7,-73],[spread,-39],
    [spread*.78,-32],[-spread*.75,-32],[-spread,-39]],
    a.outfit,outline);
  path(ctx,[[-spread*.69,-70],[-4,-74],[-2,-35],[-spread*.55,-36]],
    a.outfitShadow);
  path(ctx,[[spread*.7,-69],[spread*.84,-50],[spread*.62,-40],
    [spread*.32,-40],[spread*.35,-68]],a.outfitShadow);
  path(ctx,[[-spread*.75,-41],[spread*.75,-41],[spread*.75,-35],
    [-spread*.75,-35]],a.trim);
  path(ctx,[[-5,-71],[6,-71],[2,-55],[-1,-55]],a.trim);
  // Neck / facial plane change by octant.
  ellipse(ctx,0,-78,8,9,a.skinShadow);
  const faceX=side?8:diagonal?4:0;
  const headRX=side?19:22;
  ellipse(ctx,faceX,-94,headRX,23,back?a.hair:a.skin,outline);
  if(!back){
    ellipse(ctx,faceX+3,-89,headRX*.8,17,a.skin);
    if(side){
      ellipse(ctx,faceX+14,-94,2.1,3.2,'#3f3936');
      ctx.strokeStyle=a.skinShadow;ctx.lineWidth=2;
      ctx.beginPath();ctx.moveTo(faceX+18,-85);ctx.lineTo(faceX+14,-83);ctx.stroke();
    } else if(diagonal){
      ellipse(ctx,faceX+4,-94,2.7,3,'#30332d');
      ellipse(ctx,faceX+14,-92,1.8,2.6,'#34332e');
    } else {
      ellipse(ctx,-8,-94,2.4,3,'#30332d');
      ellipse(ctx,9,-94,2.4,3,'#30332d');
      path(ctx,[[-2,-85],[4,-85],[1,-83]],a.skinShadow);
    }
  }
  // Distinct hairstyle silhouettes, with cheek/temple strands.
  if(a.hairStyle===0){
    ellipse(ctx,faceX-3,-110,21,11,a.hair,outline);
    path(ctx,[[faceX-23,-109],[faceX-13,-103],[faceX-17,-87],
      [faceX-23,-87]],a.hair);
    path(ctx,[[faceX+6,-117],[faceX+21,-110],[faceX+14,-99],
      [faceX+4,-103]],a.hair);
  } else if(a.hairStyle===1){
    ellipse(ctx,faceX,-110,22,12,a.hair,outline);
    path(ctx,[[faceX-22,-109],[faceX-15,-98],[faceX-18,-74],
      [faceX-26,-76]],a.hair,outline);
    path(ctx,[[faceX+18,-107],[faceX+23,-84],[faceX+15,-75],
      [faceX+12,-95]],a.hair);
  } else if(a.hairStyle===2) {
    ellipse(ctx,faceX,-111,22,11,a.hair,outline);
    path(ctx,[[faceX-21,-115],[faceX-7,-98],[faceX+3,-105],
      [faceX+10,-98],[faceX+20,-110]],a.hair);
    ellipse(ctx,faceX+18,-102,9,11,a.hair);
  } else {
    ellipse(ctx,faceX,-112,23,13,a.hair,outline);
    path(ctx,[[faceX-17,-112],[faceX-12,-103],[faceX-5,-109],
      [faceX+2,-101],[faceX+9,-109],[faceX+19,-105]],
      a.hair);
  }
  if(back) {
    path(ctx,[[faceX-18,-103],[faceX+16,-103],[faceX+18,-82],
      [faceX+6,-76],[faceX-15,-79]],a.hair,outline);
    path(ctx,[[faceX-14,-104],[faceX-6,-83],[faceX+4,-83],
      [faceX+13,-106]],a.hairLight);
  } else {
    ctx.strokeStyle=a.hairLight;ctx.lineWidth=2;
    ctx.beginPath();ctx.moveTo(faceX-10,-117);
    ctx.quadraticCurveTo(faceX+4,-120,faceX+11,-113);ctx.stroke();
  }
  // Small cloth trim and collar, no name labels baked into the asset.
  path(ctx,[[-9,-73],[0,-68],[9,-73],[7,-70],[0,-64],[-7,-70]],a.trim);
  ctx.restore();
}
