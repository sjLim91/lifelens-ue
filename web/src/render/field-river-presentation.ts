/** Painter for Core-confirmed adjacent water links. The endpoints come
 * from observed hydrology; bends are illustration only, not flow geometry. */
export interface FieldRiverPoint {x:number;y:number}
export interface FieldRiverStyle {width01:number;bend:number}
export function paintObservedFieldRiver(
  ctx:CanvasRenderingContext2D,
  from:FieldRiverPoint,to:FieldRiverPoint,style:FieldRiverStyle,
  tile:number,
):void {
  const dx=to.x-from.x,dy=to.y-from.y;
  const length=Math.hypot(dx,dy);
  if(!Number.isFinite(length)||length<.001)return;
  const nx=-dy/length,ny=dx/length;
  // One gentle S curve avoids ruler-straight blue lines between chunk centers.
  // It keeps the exact Core-confirmed start/end and stays within the corridor.
  const swing=Math.min(tile*.16,length*.19);
  const signed=style.bend>=0?1:-1;
  const variance=Math.min(1,Math.abs(style.bend)*3.2);
  const sway=(.52+variance*.35)*swing*signed;
  const mid={x:(from.x+to.x)*.5+nx*sway*.16,
    y:(from.y+to.y)*.5+ny*sway*.16};
  const a={x:from.x+dx*.26+nx*sway,y:from.y+dy*.26+ny*sway};
  const b={x:from.x+dx*.74-nx*sway*.82,
    y:from.y+dy*.74-ny*sway*.82};
  // No artificial banks as thick, bright-blue map route lines.
  for(const [multiplier,stroke] of [
    [2.15,'rgba(49, 92, 82, .32)'],
    [1.18,'rgba(102, 155, 144, .84)'],
    [.23,'rgba(206, 225, 194, .28)'],
  ] as const){
    ctx.lineWidth=Math.min(11,Math.max(.8,tile*style.width01*multiplier));
    ctx.lineCap='round';ctx.strokeStyle=stroke;
    ctx.beginPath();ctx.moveTo(from.x,from.y);
    ctx.quadraticCurveTo(a.x,a.y,mid.x,mid.y);
    ctx.quadraticCurveTo(b.x,b.y,to.x,to.y);
    ctx.stroke();
  }
}
