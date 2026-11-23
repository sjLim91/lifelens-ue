/**
 * Settlement art is strictly a translucent footprint on observed facility
 * coordinates. It never relocates a shelter, creates a road, changes a
 * resident's bed use, or claims a new Core settlement structure.
 */
export interface FieldCampAnchor {
  id:string;x:number;y:number;kind:string;
}
export interface FieldCampWash {
  x:number;y:number;radius:number;strength:number;
}
export function fieldObservedCampWashes(
  anchors:readonly FieldCampAnchor[],tile:number,
):FieldCampWash[] {
  const positions=anchors.filter(a=>Number.isFinite(a.x)&&Number.isFinite(a.y)
    &&a.kind!=='CultivatedPlot').slice(0,72);
  return positions.map((a)=>{
    const nearby=positions.reduce((count,b)=>
      count+(a.id!==b.id&&Math.hypot(a.x-b.x,a.y-b.y)<tile*.76?1:0),0);
    const radius=tile*(a.kind==='FirePit'?.32:a.kind==='Shelter'?.41:.27);
    return {x:a.x,y:a.y,radius:Math.min(tile*.47,radius*(1+Math.min(3,nearby)*.11)),
      strength:.14+Math.min(3,nearby)*.025};
  });
}
export function paintFieldCampGround(
  ctx:CanvasRenderingContext2D,anchors:readonly FieldCampAnchor[],tile:number,
):void {
  if(typeof ctx.createRadialGradient!=='function')return;
  for(const wash of fieldObservedCampWashes(anchors,tile)){
    const r=wash.radius;
    const g=ctx.createRadialGradient(wash.x,wash.y,1,wash.x,wash.y,r);
    g.addColorStop(0,`rgba(169, 143, 89, ${wash.strength.toFixed(3)})`);
    g.addColorStop(.55,`rgba(156, 133, 87, ${(wash.strength*.64).toFixed(3)})`);
    g.addColorStop(1,'rgba(156, 133, 87, 0)');
    ctx.fillStyle=g;
    ctx.beginPath();ctx.ellipse(wash.x,wash.y,r,r*.48,0,0,Math.PI*2);
    ctx.fill();
  }
}
