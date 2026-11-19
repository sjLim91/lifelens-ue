import type { Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { ResidentContinuity } from '../runtime/resident-continuity';
import { runtimeDiagnostics } from '../runtime/runtime-diagnostics';
import { visibleHumanTraces } from '../state/human-traces';
import { createTerrainElevationSampler } from './terrain-geometry';
import { shade, terrainColor } from './terrain-presentation';
import { observedRiverLinks } from './isometric-natural-presentation';
import type { IsometricWorldDrawOptions } from './isometric-canvas-world-renderer';
import { createFieldProjection, type FieldView } from './field-projection';
import { FieldSpriteAtlas, type FieldSpriteName, type FieldSpritePainter } from './field-asset-atlas';
import { fieldCoreClearances, fieldDecorativePositionClear, fieldFacilityArt,
  fieldSpriteVisualOcclusion, fieldVisualUnit } from './field-scene-clarity';
import { drawFieldResident, FieldResidentMotion, type FieldResidentFrame } from './field-resident-presentation';

export type FieldMarkerKind = 'resident' | 'trace' | 'settlement' | 'animal';
export interface FieldMarker {
  kind: FieldMarkerKind;
  id: string;
  x: number;
  y: number;
  radius: number;
}
export interface FieldPaintResult {
  chunks: number;
  sprites: Record<FieldSpriteName, number>;
  markers: FieldMarker[];
  tile: number;
}
const blankCounts = (): Record<FieldSpriteName,number> => ({
  pine:0,cedar:0,oak:0,bush:0,cliff:0,rock:0,flowers:0,
  citizen_f:0,citizen_m:0,deer:0,hare:0,hut:0,fire:0,
  workbench:0,store:0,sleeping:0,furnace:0,farm:0,site:0,flag:0,
});
const clamp01 = (n: unknown) => Number.isFinite(Number(n)) ? Math.max(0,Math.min(1,Number(n))) : 0;

/** Strictly read-only art for the same Core window used by the regional map. */
export function paintFieldWorld(
  ctx: CanvasRenderingContext2D,
  view: FieldView,
  options: IsometricWorldDrawOptions,
  continuity: ResidentContinuity,
  art: FieldSpritePainter,
  motion?: FieldResidentMotion,
  nowMs = 0,
): FieldPaintResult {
  const {width,height} = view;
  ctx.clearRect(0,0,width,height);
  ctx.fillStyle = '#89a885';
  ctx.fillRect(0,0,width,height);
  const result: FieldPaintResult = {chunks:0,sprites:blankCounts(),markers:[],tile:0};
  const terrain = options.terrain;
  if (!terrain?.available || !terrain.chunks.length) {
    ctx.fillStyle='#e7eed5';ctx.textAlign='center';ctx.font='15px sans-serif';
    ctx.fillText('현장 관찰 정보를 불러오는 중…',width*.5,height*.5);
    return result;
  }
  const projection=createFieldProjection(view),{tile}=projection;
  result.tile=tile;
  const sample=createTerrainElevationSampler(terrain);
  const chunks=[...terrain.chunks].filter(c=>Number.isFinite(c.x+c.y+c.elevation01))
    .sort((a,b)=>a.y-b.y || a.x-b.x);
  const byId=new Set(chunks.map(c=>c.x+':'+c.y));
  const screen=(x:number,y:number,h:number)=>projection.project(
    x-options.centerX,y-options.centerY,h);
  const visible=(x:number,y:number,r=tile)=>x+r>=0&&x-r<=width&&y+r>=0&&y-r<=height;
  const seed=String(terrain.worldSeed??'0');
  const rand=(x:number,y:number,salt:number)=>fieldVisualUnit(seed,x,y,salt);
  const poly=(points:Array<{x:number;y:number}>,fill:string)=>{
    ctx.fillStyle=fill;ctx.beginPath();ctx.moveTo(points[0].x,points[0].y);
    for(let i=1;i<points.length;i++)ctx.lineTo(points[i].x,points[i].y);
    ctx.closePath();ctx.fill();
  };
  const sampleGrid=(gx:number,gy:number):number|null=>{
    const span=WORLD_GRID_CONTRACT.gridCellsPerChunk,cx=Math.floor(gx/span),cy=Math.floor(gy/span);
    if(!byId.has(cx+':'+cy))return null;
    return sample(cx,cy,gx/span-cx,gy/span-cy);
  };
  // No black 45° map grid. Every ground patch is projected from Core's
  // actual detailed height lattice at low-cost adaptive display resolution.
  const step=tile<100?8:4;
  for(const chunk of chunks) {
    const center=screen(chunk.x,chunk.y,chunk.elevation01);
    if(!visible(center.x,center.y,tile*1.7))continue;
    result.chunks++;
    const water=chunk.waterKind==='Ocean'?'#4b8998':chunk.waterKind==='Lake'?'#6aa49c':null;
    const base=water??(chunk.waterKind==='Coast'?'#c1b48b':terrainColor(chunk));
    for(let iy=0;iy<16;iy+=step)for(let ix=0;ix<16;ix+=step) {
      const u=ix/16,v=iy/16,d=step/16;
      const a=sample(chunk.x,chunk.y,u,v),b=sample(chunk.x,chunk.y,u+d,v);
      const c=sample(chunk.x,chunk.y,u+d,v+d),e=sample(chunk.x,chunk.y,u,v+d);
      const q0=screen(chunk.x+u-.5,chunk.y+v-.5,a);
      const q1=screen(chunk.x+u+d-.5,chunk.y+v-.5,b);
      const q2=screen(chunk.x+u+d-.5,chunk.y+v+d-.5,c);
      const q3=screen(chunk.x+u-.5,chunk.y+v+d-.5,e);
      const slope=Math.max(-.23,Math.min(.23,
        ((a+e)-(b+c))*1.12+((a+b)-(e+c))*.83));
      poly([q0,q1,q2,q3],shade(base,1+slope));
    }
    // Scenic patches follow real observed grass or wetland COVERAGE, not
    // traversable paths, extra resources or phantom farm sites.
    if(!water&&tile>=65) {
      const coverage=Math.max(clamp01(chunk.grassCoverage01),clamp01(chunk.wetlandCoverage01));
      const count=Math.floor(coverage*3);
      for(let i=0;i<count;i++) {
        const u=.13+rand(chunk.x,chunk.y,140+i*3)*.74;
        const v=.13+rand(chunk.x,chunk.y,141+i*3)*.74;
        const p=screen(chunk.x+u-.5,chunk.y+v-.5,sample(chunk.x,chunk.y,u,v)+.001);
        const s=tile*(.09+rand(chunk.x,chunk.y,142+i*3)*.09);
        ctx.fillStyle=chunk.waterKind==='Wetland'
          ?'rgba(54,110,89,.2)':'rgba(132,160,100,.26)';
        ctx.beginPath();ctx.ellipse(p.x,p.y,s*1.3,s*.42,-.08,0,Math.PI*2);ctx.fill();
      }
    }
  }
  // Narrow connected Core water edges; do not draw map-like blue roads.
  for(const edge of observedRiverLinks(terrain)) {
    const a=screen(edge.from.x,edge.from.y,
      sample(edge.from.x,edge.from.y,.5,.5)+.002);
    const b=screen(edge.to.x,edge.to.y,
      sample(edge.to.x,edge.to.y,.5,.5)+.002);
    if(!visible((a.x+b.x)*.5,(a.y+b.y)*.5,tile))continue;
    const dx=b.x-a.x,dy=b.y-a.y,d=Math.max(1,Math.hypot(dx,dy));
    const mx=(a.x+b.x)*.5-dy/d*edge.bend*tile*.55;
    const my=(a.y+b.y)*.5+dx/d*edge.bend*tile*.55;
    for(const [widthScale,color] of [[1.55,'#728d7d'],[1,'#70a9a4'],[.16,'rgba(220,232,196,.58)']] as const) {
      ctx.lineWidth=Math.min(12,Math.max(.6,tile*edge.width01*widthScale));
      ctx.lineCap='round';ctx.strokeStyle=color;ctx.beginPath();
      ctx.moveTo(a.x,a.y);ctx.quadraticCurveTo(mx,my,b.x,b.y);ctx.stroke();
    }
  }
  type DrawItem={depth:number;name:FieldSpriteName;x:number;y:number;w:number;h:number;
    marker?:FieldMarker;selected?:boolean;label?:string;
    resident?:Resident;frame?:FieldResidentFrame};
  const items:DrawItem[]=[];
  const put=(name:FieldSpriteName,p:{x:number;y:number},w:number,h:number,
    marker?:FieldMarker,selected=false,label?:string,
    resident?:Resident,frame?:FieldResidentFrame)=>{
    if(!visible(p.x,p.y,Math.max(w,h)))return;
    items.push({name,x:p.x,y:p.y,w,h,depth:p.y,marker,selected,label,resident,frame});
    if(marker)result.markers.push(marker);
  };
  // The real Core resident/facility/animal locations reserve an *observer
  // sightline*, not simulation space. Decorative forest cover must not make
  // a real person's life impossible to watch.
  const clearance=fieldCoreClearances(terrain,options.residents,
    options.civilization,continuity);
  // Forest decoration follows Core canopy coverage but leaves actual working
  // sites readable. The underlying ecology remains untouched.
  for(const c of chunks) {
    if(c.waterKind==='Ocean'||c.waterKind==='Lake')continue;
    const forest=clamp01(c.forestCoverage01),rock=clamp01(c.rockCoverage01);
    const shrub=clamp01(c.shrubCoverage01),grass=clamp01(c.grassCoverage01);
    const biome=String(c.biome??'').toLowerCase();
    const cold=/taiga|boreal|tundra|conifer/.test(biome);
    const addScenery=(name:FieldSpriteName,u:number,v:number,w:number,h:number)=>{
      const gx=c.x+u,gy=c.y+v;
      const radius=(name==='cliff'?.15:name==='rock'?.075:name==='flowers'?.025
        :name==='bush'?.055:.105);
      if(!fieldDecorativePositionClear(gx,gy,clearance,radius))return;
      const p=screen(c.x+u-.5,c.y+v-.5,sample(c.x,c.y,u,v));
      put(name,p,w,h);
    };
    // F1 legacy generated up to 11 near-identical trees per chunk, saturating
    // mobile frames. Cap canopy sprites while retaining actual Core coverage.
    const trees=forest>=.18?Math.min(6,Math.max(1,Math.ceil(forest*5.3))):0;
    for(let i=0;i<trees;i++) {
      const u=.08+rand(c.x,c.y,1000+i*4)*.84;
      const v=.08+rand(c.x,c.y,1001+i*4)*.84;
      const shape=rand(c.x,c.y,1002+i*4);
      const name:FieldSpriteName=cold?(shape>.47?'cedar':'pine'):
        shape<.33?'pine':shape<.56?'cedar':'oak';
      const size=tile*(.38+rand(c.x,c.y,1003+i*4)*.16);
      addScenery(name,u,v,size,size*(name==='oak'?1.08:1.32));
    }
    if(rock>.2) {
      const count=Math.min(4,Math.floor(rock*3)+1);
      for(let i=0;i<count;i++) {
        const u=.16+rand(c.x,c.y,220+i*3)*.68;
        const v=.14+rand(c.x,c.y,221+i*3)*.7;
        const size=tile*(.22+rock*.16);
        addScenery(rock>.54&&i===0?'cliff':'rock',u,v,
          rock>.54&&i===0?size*2:size,
          rock>.54&&i===0?size*2.1:size*.87);
      }
    }
    const bushes=Math.min(4,Math.floor(shrub*4));
    for(let i=0;i<bushes;i++) {
      const u=.13+rand(c.x,c.y,350+i*2)*.74,v=.13+rand(c.x,c.y,351+i*2)*.74;
      const size=tile*.29;
      addScenery('bush',u,v,size,size*.73);
    }
    // Flowers are non-interactable art for actual grass habitat only.
    if(grass>.36 && forest<.67) {
      const count=Math.min(3,Math.floor(grass*3));
      for(let i=0;i<count;i++) {
        const u=.13+rand(c.x,c.y,490+i*2)*.72,v=.13+rand(c.x,c.y,491+i*2)*.72;
        const size=tile*.28;
        addScenery('flowers',u,v,size,size*.67);
      }
    }
  }
  for(const trace of visibleHumanTraces(terrain)){
    if(trace.kind!=='Facility' || !Number.isFinite(trace.gridX+trace.gridY))continue;
    const elevation=sampleGrid(trace.gridX,trace.gridY);
    if(elevation===null)continue;
    const p=projection.gridPoint(trace.gridX,trace.gridY,elevation);
    const facilityArt=fieldFacilityArt(trace);
    const name=facilityArt.sprite;
    const size=tile*facilityArt.size;
    const marker:FieldMarker={kind:'trace',id:trace.id,x:p.x,y:p.y,radius:Math.max(17,size*.36)};
    put(name,p,size,size*(name==='fire'?.86:.93),marker,
      trace.id===options.selectedHumanTraceId);
  }
  for(const settlement of options.civilization.available
    ? options.civilization.settlements??[]:[]){
    if(!settlement.active||!Number.isFinite(settlement.gridX+settlement.gridY))continue;
    const elevation=sampleGrid(settlement.gridX,settlement.gridY);
    if(elevation===null)continue;
    const p=projection.gridPoint(settlement.gridX,settlement.gridY,elevation);
    // A settlement is an administrative site, not an additional physical hut.
    const size=tile*.17,marker:FieldMarker={
      kind:'settlement',id:settlement.id,x:p.x,y:p.y,radius:Math.max(18,size*.65)};
    put('flag',p,size,size*1.2,marker,settlement.id===options.selectedSettlementId);
  }
  // The resident's drawn footpoint and click selection both use the same
  // continuity position, not a decorative/random offset or future target.
  for(const resident of options.residents) {
    const pos=continuity.positionFor(resident);
    if(!pos)continue;
    const elevation=sampleGrid(pos.x,pos.y);
    if(elevation===null)continue;
    const p=projection.gridPoint(pos.x,pos.y,elevation);
    const size=Math.max(45,Math.min(94,tile*.37));
    const marker:FieldMarker={kind:'resident',id:resident.id,x:p.x,y:p.y-8,radius:size*.54};
    const label=(resident as Resident & {name?:string}).name;
    const frame=motion?.frame(resident,pos.x,pos.y,nowMs);
    put(resident.sex==='Female'?'citizen_f':'citizen_m',p,size,size*1.33,marker,
      resident.id===options.selectedResidentId,typeof label==='string'?label:undefined,
      resident,frame);
  }
  // Visible animals must originate from Core wildlife observations. No monster
  // or encounter is generated by the view. Species without an F1 asset are
  // deliberately not represented as the wrong animal.
  for(const animal of terrain.wildlife?.available?terrain.wildlife.animals:[]){
    if((animal.species!=='Deer'&&animal.species!=='Hare')
      || !Number.isFinite(animal.gridX+animal.gridY))continue;
    const elevation=sampleGrid(animal.gridX,animal.gridY);
    if(elevation===null)continue;
    const p=projection.gridPoint(animal.gridX,animal.gridY,elevation);
    const name:FieldSpriteName=animal.species==='Deer'?'deer':'hare';
    const size=tile*(name==='deer'?.38:.26);
    put(name,p,size,size*.82,
      {kind:'animal',id:animal.id,x:p.x,y:p.y,radius:Math.max(10,size*.44)});
  }
  const people=items.filter(item=>item.name==='citizen_f'||item.name==='citizen_m')
    .map(item=>({x:item.x,y:item.y,selected:Boolean(item.selected)}));
  // Preserve projected depth order; only translucent foreground vegetation
  // and buildings that would actually mask a real resident are softened.
  // The highlighted resident has a final screen-space outline on top.
  items.sort((a,b)=>a.depth-b.depth||a.x-b.x);
  for(const item of items) {
    const left=item.x-item.w*.5,top=item.y-item.h*.93;
    const previousAlpha=ctx.globalAlpha;
    const opacity=fieldSpriteVisualOcclusion(item,people);
    if(opacity<1)ctx.globalAlpha=opacity;
    if(item.selected) {
      ctx.strokeStyle='#f6e9b1';ctx.lineWidth=2.4;ctx.beginPath();
      ctx.ellipse(item.x,item.y,item.w*.54,item.w*.18,0,0,Math.PI*2);ctx.stroke();
    }
    // Detailed per-person vector puppet takes priority in the real Canvas.
    // Tests/nonstandard Canvas without transform support retain the initial
    // authored sprite fallback, preserving no-WebGL recovery.
    const vectorResident=Boolean(item.resident&&item.frame
      &&typeof ctx.save==='function' &&typeof ctx.translate==='function'
      &&typeof ctx.scale==='function');
    if(vectorResident) drawFieldResident(ctx,item.resident!,item.x,item.y,
      item.w,item.h,item.frame!);
    const drawn=vectorResident||art.draw(ctx,item.name,left,top,item.w,item.h);
    result.sprites[item.name]++;
    if(!drawn){
      // Temporary startup placeholder only while an original SVG loads.
      ctx.fillStyle=item.name.startsWith('citizen')?'#ebd3a4':'#426b4d';
      ctx.beginPath();ctx.ellipse(item.x,item.y-item.h*.52,
        item.w*.29,item.h*.38,0,0,Math.PI*2);ctx.fill();
    }
    if(previousAlpha!==undefined)ctx.globalAlpha=previousAlpha;
  }
  for(const item of items){
    if(!item.selected||!item.marker)continue;
    // Draw after scenery to keep the selected Core target readable even
    // when the real foliage would otherwise hide the observation marker.
    ctx.strokeStyle='#fff2af';ctx.lineWidth=2.8;ctx.beginPath();
    ctx.ellipse(item.x,item.y,Math.max(11,item.w*.52),
      Math.max(5,item.w*.18),0,0,Math.PI*2);ctx.stroke();
    if(item.label){
      ctx.font='bold 12px sans-serif';ctx.textAlign='center';
      ctx.fillStyle='#ffffff';ctx.strokeStyle='rgba(24,39,31,.88)';ctx.lineWidth=3;
      ctx.strokeText(item.label,item.x,item.y-item.h-7);
      ctx.fillText(item.label,item.x,item.y-item.h-7);
    }
  }
  return result;
}

/** Field/region mapping share the exact same authoritative WorldSession. */
export class FieldObserverRenderer {
  private readonly ctx:CanvasRenderingContext2D;
  private readonly atlas:FieldSpriteAtlas;
  private readonly motion=new FieldResidentMotion();
  private last:{view:FieldView;terrain:TerrainWindow;markers:FieldMarker[]}|null=null;
  constructor(private readonly canvas:HTMLCanvasElement,
    private readonly continuity:ResidentContinuity,onAssetsReady:()=>void){
    const ctx=canvas.getContext('2d');
    if(!ctx)throw new Error('현장 관찰 Canvas 2D를 사용할 수 없습니다.');
    this.ctx=ctx;this.atlas=new FieldSpriteAtlas(onAssetsReady);
    this.atlas.warm();
  }
  draw(options:IsometricWorldDrawOptions):void {
    const started=performance.now();
    const view:FieldView={
      width:this.canvas.width,height:this.canvas.height,zoom:options.zoom,
      centerChunkX:options.centerX,centerChunkY:options.centerY,
      panX:options.panX,panZ:options.panZ,
    };
    this.motion.prune(options.residents);
    const stats=paintFieldWorld(this.ctx,view,options,this.continuity,this.atlas,
      this.motion,performance.now());
    this.last=options.terrain?.available
      ?{view,terrain:options.terrain,markers:stats.markers}:null;
    runtimeDiagnostics.recordRender(performance.now()-started);
  }
  private clientPoint(clientX:number,clientY:number){
    const rect=this.canvas.getBoundingClientRect();
    if(rect.width<=0||rect.height<=0)return null;
    return{x:(clientX-rect.left)*this.canvas.width/rect.width,
      y:(clientY-rect.top)*this.canvas.height/rect.height};
  }
  groundPointAtClient(clientX:number,clientY:number):{x:number;z:number}|null{
    if(!this.last)return null;
    const p=this.clientPoint(clientX,clientY);
    if(!p)return null;
    const {view,terrain}=this.last;
    const projection=createFieldProjection(view);
    const sample=createTerrainElevationSampler(terrain);
    const cells=new Set(terrain.chunks.map(c=>c.x+':'+c.y));
    let at=projection.inverse(p.x,p.y,.5);
    const size=WORLD_GRID_CONTRACT.worldUnitsPerChunk;
    for(let i=0;i<4;i++){
      const gx=at.x/size+view.centerChunkX+.5;
      const gy=at.z/size+view.centerChunkY+.5;
      const cx=Math.floor(gx),cy=Math.floor(gy);
      if(!cells.has(cx+':'+cy))break;
      at=projection.inverse(p.x,p.y,sample(cx,cy,gx-cx,gy-cy));
    }
    return at;
  }
  private pick(kind:FieldMarkerKind,clientX:number,clientY:number):string|null{
    const p=this.clientPoint(clientX,clientY);
    if(!p||!this.last)return null;
    const candidates=this.last.markers.filter(m=>m.kind===kind
      &&Math.hypot(m.x-p.x,m.y-p.y)<=m.radius);
    candidates.sort((a,b)=>Math.hypot(a.x-p.x,a.y-p.y)-Math.hypot(b.x-p.x,b.y-p.y));
    return candidates[0]?.id??null;
  }
  pickResident(x:number,y:number){return this.pick('resident',x,y);}
  pickHumanTrace(x:number,y:number){return this.pick('trace',x,y);}
  pickSettlement(x:number,y:number){return this.pick('settlement',x,y);}
  resetMotion():void {this.motion.reset();}
  dispose(){this.motion.reset();this.atlas.dispose();this.last=null;}
}
