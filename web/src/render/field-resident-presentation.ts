import type { PhysicalArtifact, Resident } from '../runtime/core-types';
import { fieldVisualUnit } from './field-scene-clarity';
import { drawFieldArtifact, fieldResidentAction, type FieldInteractionPose } from './field-resident-actions';

export type FieldFacing = 'E'|'SE'|'S'|'SW'|'W'|'NW'|'N'|'NE';
export type FieldActionPose = FieldInteractionPose | 'walk';
export interface FieldResidentFrame {
  facing:FieldFacing;
  pose:FieldActionPose;
  /** 0..1, only active immediately after a real observed Core displacement. */
  step:number;
  tool?:PhysicalArtifact|null;
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
    const action=fieldResidentAction(r,x,y);
    if(action.pose!=='idle') return {
      facing,pose:action.pose,
      // Repeating motion is presentation only, and exists exclusively while
      // the Core is actively reporting that exact interaction.
      step:nowMs>=0?((nowMs%920)/920):0,
      tool:action.tool,
    };
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
  r:Resident,x:number,y:number,width:number,height:number,
  frame:FieldResidentFrame,
):void {
  const a=fieldResidentPalette(r);
  const back=frame.facing==='N'||frame.facing==='NE'||frame.facing==='NW';
  const side=frame.facing==='E'||frame.facing==='W';
  const diagonal=['NE','NW','SE','SW'].includes(frame.facing);
  const left=frame.facing==='W'||frame.facing==='SW'||frame.facing==='NW';
  const moving=frame.pose==='walk';
  const gather=frame.pose==='gather'||frame.pose==='tend';
  const working=['work','build','craft'].includes(frame.pose);
  const handToFace=['eat','drink','wash'].includes(frame.pose);
  const sleep=frame.pose==='sleep';
  const stride=moving?Math.sin(frame.step*Math.PI*4)*5.5:0;
  const lift=working?Math.sin(frame.step*Math.PI*2)*3.5:0;
  const bend=gather?5:handToFace?2:0;
  const breathe=1+Math.sin(frame.step*Math.PI*2)*.012;
  ctx.save();
  ctx.translate(x,y);
  ctx.scale(width/80*(left?-1:1),height/118*a.height);
  // Ground point never moves: shadow, feet and selection all share Core xy.
  ellipse(ctx,0,-1,26,5.5,'rgba(19,42,30,.22)');
  if(sleep){
    // The only rotation is visual. The body stays centered at Core xy,
    // never jumps to a nearby unverified sleeping mat.
    ctx.rotate(-Math.PI/2);
    ctx.translate(0,55);
  }
  ctx.translate(0,-(moving?Math.abs(stride)*.31:0));
  const outline='#455045',boot='#51483e';
  const broad=21*a.build;
  const calfA=-3+stride,calfB=3-stride;
  // Legs are tapered and offset in a human walk direction.
  path(ctx,[[-broad*.60,-44],[-3,-46],[calfA-3,-12],
    [calfA-15,-10],[-broad*.70,-32]],a.outfitShadow,outline);
  path(ctx,[[3,-45],[broad*.7,-44],[calfB+14,-11],
    [calfB+2,-11],[4,-31]],a.outfitShadow,outline);
  path(ctx,[[calfA-17,-14],[calfA-1,-12],[calfA+1,-6],
    [calfA-14,-5],[calfA-19,-8]],boot,outline);
  path(ctx,[[calfB+1,-14],[calfB+16,-13],[calfB+20,-7],
    [calfB+4,-5],[calfB, -9]],boot,outline);
  path(ctx,[[calfA-13,-13],[calfA-8,-10]],'#aa9271');
  path(ctx,[[calfB+7,-13],[calfB+12,-10]],'#a28b68');

  // In a side-facing frame the far shoulder partly hides behind the body.
  const farOffset=side?4:0;
  path(ctx,[[-19,-70],[-29,-65],[-28-bend,-44+stride*.22],
    [-22-bend,-36+stride*.13],[-14,-54]],a.outfitShadow,outline);
  path(ctx,[[19,-70],[28+farOffset,-64],
    [30+bend,-43-stride*.23],[23+bend,-36-stride*.2],
    [12,-52]],a.outfitShadow,outline);
  ellipse(ctx,-24-bend,-37+stride*.13,6,6,a.skinShadow);
  ellipse(ctx,25+bend,-37-stride*.2,6,6,a.skin);
  // Woven sash and layered mantle match the illustrated natural asset set.
  ctx.save();
  ctx.translate(0,bend);
  ctx.scale(breathe,1);
  path(ctx,[[-broad*.92,-76],[-8,-78],[2,-73],[broad*.86,-75],
    [broad+3,-58],[broad*.82,-36],[-broad*.87,-36],
    [-broad-2,-58]],a.outfit,outline);
  path(ctx,[[-broad*.9,-73],[-5,-75],[-2,-68],[-9,-48],
    [-broad*.75,-41],[-broad-2,-56]],a.outfitShadow);
  path(ctx,[[broad*.66,-73],[broad*.95,-59],
    [broad*.6,-38],[broad*.27,-39],[broad*.40,-68]],a.outfitShadow);
  // Pleated collar and cloth overlays.
  path(ctx,[[-14,-75],[-3,-66],[2,-68],[11,-75],
    [15,-67],[1,-57],[-13,-67]],a.trim);
  path(ctx,[[-broad*.85,-44],[broad*.84,-44],[broad*.83,-37],
    [-broad*.82,-37]],a.trim);
  path(ctx,[[-11,-61],[-7,-54],[-10,-43],[-15,-45]],a.outfitShadow);
  path(ctx,[[8,-62],[12,-56],[9,-44],[4,-45]],a.hairLight);
  ctx.strokeStyle=a.outfitShadow;ctx.lineWidth=1.15;
  for(let n=0;n<4;n++){
    ctx.beginPath();ctx.moveTo(-broad*.75+n*3,-40);
    ctx.lineTo(-broad*.68+n*3,-37);ctx.stroke();
  }
  ctx.restore();

  // Core-confirmed interaction creates hand motion, never imagined tools.
  if(gather){
    path(ctx,[[18,-68],[26,-58],[31+lift,-29],[22+lift,-26],
      [11,-47]],a.outfitShadow,outline);
    ellipse(ctx,27+lift,-29,5.5,5.5,a.skin);
    path(ctx,[[-16,-44],[-9,-28],[-18,-20],[-27,-30]],a.outfitShadow);
  }else if(working){
    const swing=frame.pose==='craft'?0:lift;
    path(ctx,[[15,-68],[22,-64],[30+swing,-80-swing],
      [24+swing,-86-swing],[15,-76]],a.outfit,outline);
    ellipse(ctx,25+swing,-83-swing,5.8,6,a.skin);
    if(frame.pose==='craft'){
      path(ctx,[[-17,-67],[-21,-58],[-3,-53],[1,-60]],a.outfitShadow);
      path(ctx,[[18,-67],[24,-57],[5,-53],[1,-60]],a.outfitShadow);
      ellipse(ctx,0,-54,5,5,a.skin);
    }
  }else if(handToFace){
    path(ctx,[[18,-70],[26,-62],[19+lift*.15,-84],
      [10,-79],[9,-64]],a.outfit,outline);
    ellipse(ctx,16+lift*.14,-83,5.2,5.5,a.skin);
    if(frame.pose==='wash') {
      path(ctx,[[-18,-67],[-26,-61],[-16,-85],[-5,-82],
        [-10,-63]],a.outfitShadow,outline);
    }
  }

  // Face: softened silhouette, side/back geometry and stable genetic palette.
  ellipse(ctx,0,-80,8,10,a.skinShadow);
  const faceX=side?7:diagonal?3:0;
  const faceWidth=side?18:21;
  ellipse(ctx,faceX,-96,faceWidth+1,23,back?a.hair:a.skinShadow,outline);
  if(!back){
    ellipse(ctx,faceX+2,-94,faceWidth,20,a.skin);
    ellipse(ctx,faceX+2,-81,12,5,a.skinShadow);
    if(sleep){
      ctx.strokeStyle='#6b4f42';ctx.lineWidth=1.9;
      ctx.beginPath();ctx.moveTo(faceX-8,-93);
      ctx.quadraticCurveTo(faceX-4,-90,faceX+1,-94);ctx.stroke();
      ctx.beginPath();ctx.moveTo(faceX+8,-93);
      ctx.quadraticCurveTo(faceX+12,-90,faceX+15,-94);ctx.stroke();
    }else if(side){
      ellipse(ctx,faceX+14,-94,2.1,3.1,'#332e2b');
      ellipse(ctx,faceX+14.5,-95.2,.7,.9,'#eee6d4');
      path(ctx,[[faceX+17,-87],[faceX+20,-85],[faceX+16,-82]],a.skinShadow);
    }else{
      const ex=diagonal?2:0;
      ellipse(ctx,faceX-8+ex,-95,2.1,3,'#3b342f');
      ellipse(ctx,faceX+8+ex,-95,2.1,3,'#3b342f');
      ellipse(ctx,faceX-7.3+ex,-96,.7,.8,'#f2e3cf');
      ellipse(ctx,faceX+8.8+ex,-96,.7,.8,'#f2e3cf');
      path(ctx,[[faceX-2,-85],[faceX+4,-84],
        [faceX+1,-82]],a.skinShadow);
    }
    ellipse(ctx,faceX-17,-84,4,2.4,'rgba(170,100,87,.16)');
    ellipse(ctx,faceX+16,-84,3,2.2,'rgba(170,100,87,.16)');
  }
  // Hair has hand-painted overlapping masses and lighter strands.
  if(a.hairStyle===0){
    ellipse(ctx,faceX-2,-111,22,12,a.hair,outline);
    path(ctx,[[faceX-22,-112],[faceX-14,-105],
      [faceX-18,-86],[faceX-24,-86]],a.hair);
    path(ctx,[[faceX+3,-118],[faceX+21,-112],
      [faceX+16,-99],[faceX+7,-104]],a.hair);
  }else if(a.hairStyle===1){
    ellipse(ctx,faceX,-111,23,13,a.hair,outline);
    path(ctx,[[faceX-22,-109],[faceX-16,-98],
      [faceX-17,-73],[faceX-24,-76]],a.hair,outline);
    path(ctx,[[faceX+19,-110],[faceX+23,-84],
      [faceX+13,-75],[faceX+12,-100]],a.hair,outline);
  }else if(a.hairStyle===2){
    ellipse(ctx,faceX-1,-112,21,11,a.hair,outline);
    path(ctx,[[faceX-20,-115],[faceX-10,-103],
      [faceX-1,-111],[faceX+8,-100],
      [faceX+20,-109]],a.hair);
    ellipse(ctx,faceX+18,-104,9,10,a.hair);
  }else{
    ellipse(ctx,faceX,-113,22,12,a.hair,outline);
    path(ctx,[[faceX-20,-113],[faceX-13,-105],
      [faceX-6,-111],[faceX+2,-102],
      [faceX+9,-111],[faceX+20,-106]],a.hair);
  }
  if(back){
    path(ctx,[[faceX-19,-105],[faceX+17,-105],
      [faceX+18,-80],[faceX-14,-79]],a.hair,outline);
  }
  ctx.strokeStyle=a.hairLight;ctx.lineWidth=1.4;
  ctx.beginPath();ctx.moveTo(faceX-10,-118);
  ctx.quadraticCurveTo(faceX+1,-120,faceX+11,-114);ctx.stroke();
  // Decorative woven garment trim is not a physical Core inventory item.
  ctx.strokeStyle=a.trim;ctx.lineWidth=1.6;
  ctx.beginPath();ctx.moveTo(-7,-74);
  ctx.quadraticCurveTo(0,-68,8,-74);ctx.stroke();
  if(frame.pose==='gather'&&frame.tool)drawFieldArtifact(ctx,frame.tool);
  ctx.restore();
}
