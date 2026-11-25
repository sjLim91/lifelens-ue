/** Project a channel between two adjacent, Core-confirmed hydrology samples. */
export interface FieldRiverPoint {x:number;y:number}
export interface FieldRiverStyle {width01:number;bend:number}
export function paintObservedFieldRiver(
  ctx:CanvasRenderingContext2D,from:FieldRiverPoint,to:FieldRiverPoint,
  style:FieldRiverStyle,tile:number,seconds=0,water?:CanvasPattern|null,
):void {
  const dx=to.x-from.x,dy=to.y-from.y,length=Math.hypot(dx,dy);
  if(!Number.isFinite(length)||length<.001)return;
  const nx=-dy/length,ny=dx/length;
  const radius=Math.min(length*.14,Math.max(1.5,tile*style.width01*.98));
  const bend=Math.sin(style.bend*22)*length*.09;
  const point=(t:number,across:number)=>{
    const wave=Math.sin(Math.PI*t);
    const width=radius*(1+.24*wave*Math.sin(t*9+style.bend*37));
    const shelf=wave*.15*Math.sin(t*16+across*3.5+style.bend*19);
    const offset=wave*(bend*Math.cos(t*Math.PI)+radius*.35*Math.sin(t*14))
      +width*(across+shelf);
    return {x:from.x+t*dx+nx*offset,y:from.y+t*dy+ny*offset};
  };
  const band=(lo:number,hi:number,color:string|CanvasPattern)=>{
    ctx.fillStyle=color;ctx.beginPath();
    for(let i=0;i<=20;i++){
      const p=point(i/20,lo);
      if(i===0)ctx.moveTo(p.x,p.y);else ctx.lineTo(p.x,p.y);
    }
    for(let i=20;i>=0;i--){
      const p=point(i/20,hi);ctx.lineTo(p.x,p.y);
    }
    ctx.closePath();ctx.fill();
  };
  // Wet earth and gravel frame the shallow shelf and deeper current.
  band(-1.22,1.18,'#777653');band(-1.11,1.06,'#a29d71');
  band(-1,1,'#719c84');band(-.80,.87,'#518d88');
  band(-.49,.64,'#397b80');
  if(water){
    ctx.save();
    const scale=Math.max(.08,tile/1200);
    water.setTransform(new DOMMatrix().translate(from.x,from.y).scale(scale));
    ctx.globalAlpha*=.56;
    band(-.94,.95,water);
    ctx.restore();
  }
  ctx.lineCap='round';
  for(let i=0;i<16;i++){
    const t=(i+.35)/17+Math.sin(seconds*.8+i)*.009;
    const cross=Math.sin(i*19.3+style.bend)*.78;
    const a=point(t,cross),b=point(t+.015+(i%4)*.005,cross+.018);
    ctx.lineWidth=Math.max(.55,radius*.047);
    ctx.strokeStyle=i%3?'rgba(201,234,214,.55)':'rgba(35,94,98,.60)';
    ctx.beginPath();ctx.moveTo(a.x,a.y);ctx.lineTo(b.x,b.y);ctx.stroke();
  }
}
