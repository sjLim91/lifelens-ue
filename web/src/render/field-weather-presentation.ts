import type { DynamicEnvironment } from '../runtime/core-types';

export interface FieldAtmosphere {
  environment?: DynamicEnvironment | null;
  worldMinute?: number;
}
const clamp01=(n:unknown,fallback=0):number=>
  typeof n==='number'&&Number.isFinite(n)?Math.max(0,Math.min(1,n)):fallback;

/**
 * All light and precipitation cues are derived from the observed Core clock
 * and weather, never wall-clock "game time" or generated weather events.
 */
export function fieldLightAndWeather(state:FieldAtmosphere) {
  const env=state.environment?.available ? state.environment : null;
  const minute=Number.isFinite(state.worldMinute)?Number(state.worldMinute):
    Number(env?.simulationMinute??720);
  const minuteOfDay=((minute%1440)+1440)%1440;
  const hour=env?.calendar?.hourOfDay;
  const clockHour=typeof hour==='number'&&Number.isFinite(hour)?hour:
    minuteOfDay/60;
  const derivedDaylight=Math.max(0,Math.sin((clockHour-6)*Math.PI/12));
  const daylight=clamp01(env?.calendar?.daylight01,derivedDaylight);
  const nightOpacity=Math.max(0,Math.min(.53,(1-daylight)*.53));
  const precip=env?.precipitationType??'None';
  const summary=env?.summary;
  const raining=Boolean(env&&(precip==='Rain'||summary==='Rain'||summary==='Storm'));
  const snowing=Boolean(env&&(precip==='Snow'||summary==='Snow'));
  const intensity=clamp01(env?.precipitationIntensity01,
    raining||snowing?.4:0);
  const cloud=clamp01(env?.cloudCover01);
  return {daylight,nightOpacity,raining,snowing,intensity,cloud};
}

/**
 * Lightweight Canvas-only screen-space atmosphere. Repeated marks are a
 * visual weather layer; their count does not represent physical raindrops.
 * Missing Core weather produces no precipitation.
 */
export function paintFieldAtmosphere(
  ctx:CanvasRenderingContext2D,width:number,height:number,
  state:FieldAtmosphere,nowMs=0,
):void {
  const v=fieldLightAndWeather(state);
  if(v.cloud>.5 && v.nightOpacity<.4){
    ctx.fillStyle=`rgba(63, 77, 88, ${((v.cloud-.5)*.16).toFixed(3)})`;
    ctx.fillRect(0,0,width,height);
  }
  if(v.nightOpacity>0){
    ctx.fillStyle=`rgba(8, 18, 34, ${v.nightOpacity.toFixed(3)})`;
    ctx.fillRect(0,0,width,height);
  }
  if(v.raining&&v.intensity>0){
    const count=Math.min(80,Math.max(8,Math.floor(width*height/11000*v.intensity)));
    ctx.lineWidth=1.1;
    ctx.strokeStyle='rgba(189, 220, 236, .43)';
    const frame=Math.floor(nowMs/55);
    for(let i=0;i<count;i++){
      const x=((i*137.63+frame*2.7)%Math.max(1,width)+width)%width;
      const y=((i*91.47+frame*13)%Math.max(1,height)+height)%height;
      ctx.beginPath();ctx.moveTo(x,y);
      ctx.lineTo(x-3.2-v.intensity*4,y+9+v.intensity*9);ctx.stroke();
    }
  }else if(v.snowing&&v.intensity>0){
    const count=Math.min(48,Math.max(5,Math.floor(width*height/15000*v.intensity)));
    const frame=Math.floor(nowMs/150);
    ctx.fillStyle='rgba(244, 251, 253, .72)';
    for(let i=0;i<count;i++){
      const x=((i*131.7+frame*.65)%Math.max(1,width)+width)%width;
      const y=((i*97.3+frame*2.3)%Math.max(1,height)+height)%height;
      ctx.beginPath();ctx.ellipse(x,y,1.6,1.6,0,0,Math.PI*2);ctx.fill();
    }
  }
}
