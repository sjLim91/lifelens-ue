import type { DynamicEnvironment, Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { ResidentContinuity } from '../runtime/resident-continuity';
import { runtimeDiagnostics } from '../runtime/runtime-diagnostics';
import { visibleHumanTraces } from '../state/human-traces';
import { createTerrainElevationSampler } from './terrain-geometry';
import { shade } from './terrain-presentation';
import { observedRiverLinks } from './isometric-natural-presentation';
import type { IsometricWorldDrawOptions } from './isometric-canvas-world-renderer';
import { createFieldProjection, type FieldView } from './field-projection';
import { fieldArtworkWidth, FieldSpriteAtlas, type FieldSpriteName, type FieldSpritePainter } from './field-asset-atlas';
import { fieldCoreClearances, fieldDecorativePositionClear, fieldFacilityArt,
  fieldSpriteVisualOcclusion, fieldVisualUnit } from './field-scene-clarity';
import { drawFieldResident, FieldResidentMotion, type FieldResidentFrame } from './field-resident-presentation';
import { fieldLifeStageScale } from './field-resident-actions';
import { paintFieldAtmosphere } from './field-weather-presentation';
import { fieldConnectedFloorColor, fieldForestDisplayClusters,
  fieldObservedRockFace,fieldVisualSpriteTile,
  fieldContinuousGroundNoise,fieldGroundDetailMarks } from './field-natural-art';
import { paintObservedFieldRiver } from './field-river-presentation';
import { FieldTerrainCompositor, paintFieldShore } from './field-terrain-compositor';
import { paintFieldCampGround, type FieldCampAnchor } from './field-settlement-illustration';

export type FieldWorldDrawOptions = IsometricWorldDrawOptions & {
  environment?: DynamicEnvironment | null;
  worldMinute?: number;
  residentCloseup?:boolean;
  pixelRatio?:number;
  paused?:boolean;
};

export type FieldMarkerKind = 'resident' | 'trace' | 'settlement' | 'animal';
export interface FieldMarker {
  kind: FieldMarkerKind;
  id: string;
  x: number;
  y: number;
  radius: number;
  bodyHeight?:number;
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
const facilityHeight:Partial<Record<FieldSpriteName,number>>={
  hut:1.85,store:1.5,site:1.65,workbench:.74,sleeping:.78,
  fire:.43,furnace:.92,farm:.90,
};

export function paintFieldWorld(
  ctx: CanvasRenderingContext2D,
  view: FieldView,
  options: FieldWorldDrawOptions,
  continuity: ResidentContinuity,
  art: FieldSpritePainter,
  motion?: FieldResidentMotion,
  nowMs = 0,
  terrainPainter?:FieldTerrainCompositor,
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
  // Illustration assets should not balloon with a close observer camera.
  // Ground/picking always use the unmodified authoritative projection tile.
  const spriteTile=fieldVisualSpriteTile(tile,width,height);
  // Screen height of a standing adult, shared by fauna, scenery and facilities.
  const ratio=options.pixelRatio??1;
  const adultHeight=options.residentCloseup
    ? Math.min(width*.33,146*ratio)
    : Math.min(86*ratio,spriteTile*.31);
  result.tile=tile;
  const sample=createTerrainElevationSampler(terrain);
  const chunks=[...terrain.chunks].filter(c=>Number.isFinite(c.x+c.y+c.elevation01))
    .sort((a,b)=>a.y-b.y || a.x-b.x);
  const byChunk=new Map(chunks.map(c=>[c.x+':'+c.y,c] as const));
  const byId=new Set(byChunk.keys());
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
    const water=chunk.waterKind==='Ocean'||chunk.waterKind==='Lake';
    const neighbors={
      west:byChunk.get((chunk.x-1)+':'+chunk.y),
      east:byChunk.get((chunk.x+1)+':'+chunk.y),
      north:byChunk.get(chunk.x+':'+(chunk.y-1)),
      south:byChunk.get(chunk.x+':'+(chunk.y+1)),
    };
    const textured=terrainPainter?.paint(ctx,seed,chunk,
      (x,y)=>byChunk.get(x+':'+y),screen,sample)===true;
    if(!textured)for(let iy=0;iy<16;iy+=step)for(let ix=0;ix<16;ix+=step) {
      const u=ix/16,v=iy/16,d=step/16;
      const a=sample(chunk.x,chunk.y,u,v),b=sample(chunk.x,chunk.y,u+d,v);
      const c=sample(chunk.x,chunk.y,u+d,v+d),e=sample(chunk.x,chunk.y,u,v+d);
      const q0=screen(chunk.x+u-.5,chunk.y+v-.5,a);
      const q1=screen(chunk.x+u+d-.5,chunk.y+v-.5,b);
      const q2=screen(chunk.x+u+d-.5,chunk.y+v+d-.5,c);
      const q3=screen(chunk.x+u-.5,chunk.y+v+d-.5,e);
      const slope=Math.max(-.23,Math.min(.23,
        ((a+e)-(b+c))*1.12+((a+b)-(e+c))*.83));
      // Color continuity is derived from actual adjacent Core chunks.
      const base=fieldConnectedFloorColor(chunk,neighbors,u+d*.5,v+d*.5);
      const pigment=fieldContinuousGroundNoise(seed,
        chunk.x+u+d*.5,chunk.y+v+d*.5);
      poly([q0,q1,q2,q3],shade(base,1+slope+pigment*.075));
      // Actual Core elevation differences form gentle land contour shading.
      // Steeper observed surfaces receive a contour edge, not a raised
      // fictional cliff or an additional collision mesh.
      const incline=Math.max(Math.abs(a-c),Math.abs(b-e));
      if(!water&&incline>.042){
        ctx.strokeStyle='rgba(19, 42, 31, .20)';
        ctx.lineWidth=Math.min(2.6,Math.max(.65,tile*.009));
        ctx.beginPath();ctx.moveTo(q1.x,q1.y);ctx.lineTo(q2.x,q2.y);
        ctx.lineTo(q3.x,q3.y);ctx.stroke();
      }
    }
    // Only confirmed Core water boundaries receive a visual shore.
    if(textured)paintFieldShore(ctx,chunk,
      (x,y)=>byChunk.get(x+':'+y),(x,y)=>screen(x,y,.5),sample,seed);
    // Small habitat-dependent strokes replace the large uniform ovals
    // that previously made open ground look like repeated map stickers.
    // The authored terrain texture already contains grass/soil detail.
    // The old V-shaped glyph spray remains only for safe legacy/fake Canvas.
    if(!textured&&!water&&tile>=50){
      const marks=fieldGroundDetailMarks(seed,chunk.x,chunk.y,
        chunk.grassCoverage01,chunk.forestCoverage01,chunk.rockCoverage01);
      const budget=tile<100?Math.ceil(marks.length*.24)
        :tile<190?Math.ceil(marks.length*.44):marks.length;
      for(let i=0;i<budget;i++){
        const mark=marks[i];
        const elevation=sample(chunk.x,chunk.y,mark.u,mark.v);
        const p=screen(chunk.x+mark.u-.5,chunk.y+mark.v-.5,elevation+.001);
        if(!visible(p.x,p.y,9))continue;
        const s=Math.min(9,Math.max(1.4,tile*mark.length));
        ctx.lineWidth=mark.kind==='lichen'?1.5:Math.max(.7,s*.16);
        ctx.strokeStyle=mark.kind==='lichen'
          ?'rgba(170, 166, 139, .31)':mark.kind==='soil'
          ?'rgba(99, 104, 76, .20)':'rgba(177, 208, 129, .40)';
        ctx.beginPath();
        if(mark.kind==='grass'){
          ctx.moveTo(p.x-s*.44,p.y+s*.1);
          ctx.lineTo(p.x+s*mark.rotation*.35,p.y-s*.70);
          ctx.lineTo(p.x+s*.36,p.y+s*.16);
        }else{
          ctx.moveTo(p.x-s*.33,p.y);
          ctx.lineTo(p.x+s*.28,p.y-s*.12);
        }
        ctx.stroke();
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
    paintObservedFieldRiver(ctx,a,b,edge,tile,nowMs/1000,terrainPainter?.waterPattern(ctx));
  }
  // Render faint inhabited ground only at actual observed facilities, before
  // objects/actors. These washes are environmental paint, not new roads.
  const campAnchors:FieldCampAnchor[]=visibleHumanTraces(terrain)
    .filter(t=>t.kind==='Facility'&&Number.isFinite(t.gridX+t.gridY))
    .flatMap(t=>{
      const h=sampleGrid(t.gridX,t.gridY);
      if(h===null)return [];
      const p=projection.gridPoint(t.gridX,t.gridY,h);
      return [{id:t.id,x:p.x,y:p.y,kind:t.kind==='Facility'?t.facilityKind??'Unknown':'Unknown'}];
    });
  paintFieldCampGround(ctx,campAnchors,tile);
  type DrawItem={depth:number;name:FieldSpriteName;x:number;y:number;w:number;h:number;
    marker?:FieldMarker;selected?:boolean;label?:string;
    resident?:Resident;frame?:FieldResidentFrame;variation?:number};
  const items:DrawItem[]=[];
  const put=(name:FieldSpriteName,p:{x:number;y:number},w:number,h:number,
    marker?:FieldMarker,selected=false,label?:string,
    resident?:Resident,frame?:FieldResidentFrame,variation?:number)=>{
    if(!visible(p.x,p.y,Math.max(w,h)))return;
    items.push({name,x:p.x,y:p.y,w,h,depth:p.y,marker,selected,label,resident,frame,variation});
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
    const biome=String(c.biome??'');
    const addScenery=(name:FieldSpriteName,u:number,v:number,h:number)=>{
      const gx=c.x+u,gy=c.y+v;
      const radius=(name==='cliff'?.15:name==='rock'?.075:name==='flowers'?.025
        :name==='bush'?.055:.105);
      if(!fieldDecorativePositionClear(gx,gy,clearance,radius))return;
      const p=screen(c.x+u-.5,c.y+v-.5,sample(c.x,c.y,u,v));
      // Stable world-position variation only: camera movement never
      // changes species, hue or silhouette for a decorative plant.
      const variation=fieldVisualUnit(seed,c.x,c.y,
        20000+Math.floor(u*1024)+Math.floor(v*1024)*1031);
      put(name,p,fieldArtworkWidth(name,h),h,undefined,false,undefined,undefined,undefined,variation);
    };
    // Cohesive observed-forest canopy groups. No extra ecology or
    // navigational obstacles exist in Core because of these decorations.
    const canopy=fieldForestDisplayClusters(seed,c.x,c.y,forest,biome);
    for(const tree of canopy) {
      const matureHeight=tree.name==='cedar'?4.6:tree.name==='pine'?3.6:3.2;
      addScenery(tree.name,tree.u,tree.v,adultHeight*matureHeight*tree.sizeFactor/.48);
    }
    if(rock>.2) {
      const count=Math.min(4,Math.floor(rock*3)+1);
      for(let i=0;i<count;i++) {
        const u=.16+rand(c.x,c.y,220+i*3)*.68;
        const v=.14+rand(c.x,c.y,221+i*3)*.7;
        const size=adultHeight*(.38+rock*.30);
        // A rock-rich flat plain cannot pretend to be a sheer mountain.
        // Inspect the real surface around this visual rock placement.
        const slopeSamples=[
          sample(c.x,c.y,Math.max(0,u-.1),Math.max(0,v-.1)),
          sample(c.x,c.y,Math.min(1,u+.1),Math.max(0,v-.1)),
          sample(c.x,c.y,Math.max(0,u-.1),Math.min(1,v+.1)),
          sample(c.x,c.y,Math.min(1,u+.1),Math.min(1,v+.1)),
        ];
        const steep=i===0&&fieldObservedRockFace(slopeSamples,rock);
        addScenery(steep?'cliff':'rock',u,v,
          steep?size*3.8:size);
      }
    }
    const bushes=Math.min(4,Math.floor(shrub*4));
    for(let i=0;i<bushes;i++) {
      // Shrubs nestle near observed canopy clusters rather than forming
      // unrelated evenly spaced visual wallpaper.
      const anchor=canopy.length?canopy[i%canopy.length]:null;
      const u=anchor?Math.max(.08,Math.min(.92,anchor.u+(rand(c.x,c.y,350+i*2)-.5)*.31))
        :.13+rand(c.x,c.y,350+i*2)*.74;
      const v=anchor?Math.max(.08,Math.min(.92,anchor.v+(rand(c.x,c.y,351+i*2)-.5)*.32))
        :.13+rand(c.x,c.y,351+i*2)*.74;
      addScenery('bush',u,v,adultHeight*(.45+shrub*.22));
    }
    // Flowers are non-interactable art for actual grass habitat only.
    if(grass>.36 && forest<.67) {
      const count=Math.min(3,Math.floor(grass*3));
      const centerU=.23+rand(c.x,c.y,490)*.54;
      const centerV=.23+rand(c.x,c.y,491)*.54;
      for(let i=0;i<count;i++) {
        const u=Math.max(.08,Math.min(.92,centerU+(rand(c.x,c.y,492+i*2)-.5)*.24));
        const v=Math.max(.08,Math.min(.92,centerV+(rand(c.x,c.y,493+i*2)-.5)*.24));
        addScenery('flowers',u,v,adultHeight*.21);
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
    const height=adultHeight*(facilityHeight[name]??1);
    const size=fieldArtworkWidth(name,height);
    const marker:FieldMarker={kind:'trace',id:trace.id,x:p.x,y:p.y,radius:Math.max(17,size*.36)};
    put(name,p,size,height,marker,
      trace.id===options.selectedHumanTraceId);
  }
  for(const settlement of options.civilization.available
    ? options.civilization.settlements??[]:[]){
    if(!settlement.active||!Number.isFinite(settlement.gridX+settlement.gridY))continue;
    const elevation=sampleGrid(settlement.gridX,settlement.gridY);
    if(elevation===null)continue;
    const p=projection.gridPoint(settlement.gridX,settlement.gridY,elevation);
    // A settlement is an administrative site, not an additional physical hut.
    const height=adultHeight*1.16,size=fieldArtworkWidth('flag',height),marker:FieldMarker={
      kind:'settlement',id:settlement.id,x:p.x,y:p.y,radius:Math.max(18,size*.65)};
    put('flag',p,size,height,marker,settlement.id===options.selectedSettlementId);
  }
  // The resident's drawn footpoint and click selection both use the same
  // continuity position, not a decorative/random offset or future target.
  for(const resident of options.residents) {
    const pos=continuity.positionFor(resident);
    if(!pos)continue;
    const elevation=sampleGrid(pos.x,pos.y);
    if(elevation===null)continue;
    const p=projection.gridPoint(pos.x,pos.y,elevation);
    const size=(adultHeight/1.35)
      *fieldLifeStageScale(resident);
    const marker:FieldMarker={kind:'resident',id:resident.id,
      x:p.x,y:p.y-6,bodyHeight:size*1.27,radius:15+size*.22};
    const label=(resident as Resident & {name?:string}).name;
    const frame=motion?.frame(resident,pos.x,pos.y,nowMs);
    put(resident.sex==='Female'?'citizen_f':'citizen_m',p,size,size*1.27,marker,
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
    const height=adultHeight*(name==='deer'?.92:.22);
    const size=fieldArtworkWidth(name,height);
    put(name,p,size,height,
      {kind:'animal',id:animal.id,x:p.x,y:p.y,radius:Math.max(10,size*.44)});
  }
  const people=items.filter(item=>item.name==='citizen_f'||item.name==='citizen_m')
    .map(item=>({x:item.x,y:item.y,selected:Boolean(item.selected)}));
  // Preserve projected depth order; only translucent foreground vegetation
  // and buildings that would actually mask a real resident are softened.
  // The highlighted resident has a final screen-space outline on top.
  items.sort((a,b)=>a.depth-b.depth||a.x-b.x);
  for(const item of items) {
    const sleep=item.resident?.presentation;
    if(sleep?.active && sleep.phase==='Interacting' && sleep.physicalGoal==='Sleep'
      && sleep.sleepContext==='Protected' && sleep.facilityKind==='Shelter'){
      result.sprites[item.name]++;
      continue; // The resident occupies the roofed interior at this Core position.
    }
    const left=item.x-item.w*.5,top=item.y-item.h;
    const previousAlpha=ctx.globalAlpha;
    const opacity=fieldSpriteVisualOcclusion(item,people);
    if(opacity<1)ctx.globalAlpha=opacity;
    if(item.name!=='flowers'&&item.name!=='flag'){
      // Ground-contact ellipse anchors illustrated sprites to Core ground.
      // Its footprint is only a visual shadow, never a Core occupancy cell.
      ctx.fillStyle='rgba(15, 31, 25, .20)';
      ctx.beginPath();ctx.ellipse(item.x+item.w*.07,item.y+item.w*.025,
        item.w*(item.resident?.22:.32),Math.max(2,item.w*.075),
        0,0,Math.PI*2);ctx.fill();
    }
    if(item.selected) {
      ctx.strokeStyle='#f6e9b1';ctx.lineWidth=2.4;ctx.beginPath();
      ctx.ellipse(item.x,item.y,item.w*.54,item.w*.18,0,0,Math.PI*2);ctx.stroke();
    }
    const vectorResident=Boolean(item.resident&&item.frame
      &&typeof ctx.save==='function' &&typeof ctx.translate==='function'
      &&typeof ctx.scale==='function');
    if(vectorResident) drawFieldResident(ctx,item.resident!,item.x,item.y,
      item.w,item.h,item.frame!,art);
    let drawn=vectorResident;
    const varyPlant=item.variation!==undefined
      &&['pine','cedar','oak','bush'].includes(item.name)
      &&typeof ctx.save==='function'&&typeof ctx.restore==='function'
      &&typeof ctx.translate==='function'&&typeof ctx.scale==='function';
    if(!drawn){
      if(varyPlant){
        ctx.save();
        const u=item.variation!;
        // Only geometric mirroring happens per frame; palette variants are
        // pre-rasterized once at SVG load by the local FieldSpriteAtlas.
        if(u>.51){ctx.translate(item.x*2,0);ctx.scale(-1,1);}
      }
      drawn=art.draw(ctx,item.name,left,top,item.w,item.h,item.variation);
      if(varyPlant)ctx.restore();
    }
    result.sprites[item.name]++;
    if(!drawn){
      // Temporary startup placeholder only while an original SVG loads.
      ctx.fillStyle=item.name.startsWith('citizen')?'#ebd3a4':'#426b4d';
      ctx.beginPath();ctx.ellipse(item.x,item.y-item.h*.52,
        item.w*.29,item.h*.38,0,0,Math.PI*2);ctx.fill();
    }
    if(previousAlpha!==undefined)ctx.globalAlpha=previousAlpha;
  }
  // Weather/light is drawn over the world, not over the UI controls.
  paintFieldAtmosphere(ctx,width,height,{
    environment:options.environment,worldMinute:options.worldMinute,
  },nowMs);
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
      const label=item.resident?.presentation?.sleepContext==='Protected' ? item.label+' · 집 안에서 수면 중' : item.label;
      ctx.strokeText(label,item.x,item.y-item.h-7);
      ctx.fillText(label,item.x,item.y-item.h-7);
    }
  }
  return result;
}

/** Field/region mapping share the exact same authoritative WorldSession. */
export class FieldObserverRenderer {
  private readonly ctx:CanvasRenderingContext2D;
  private readonly atlas:FieldSpriteAtlas;
  private readonly motion=new FieldResidentMotion();
  private readonly terrainPainter=new FieldTerrainCompositor();
  private motionTime=0;
  private lastDrawTime=0;
  private last:{view:FieldView;terrain:TerrainWindow;markers:FieldMarker[]}|null=null;
  constructor(private readonly canvas:HTMLCanvasElement,
    private readonly continuity:ResidentContinuity,onAssetsReady:()=>void){
    const ctx=canvas.getContext('2d');
    if(!ctx)throw new Error('현장 관찰 Canvas 2D를 사용할 수 없습니다.');
    this.ctx=ctx;this.atlas=new FieldSpriteAtlas(onAssetsReady);
    this.atlas.warm();
    this.terrainPainter.warm(onAssetsReady);
  }
  draw(options:FieldWorldDrawOptions):void {
    const started=performance.now();
    const view:FieldView={
      width:this.canvas.width,height:this.canvas.height,zoom:options.zoom,
      centerChunkX:options.centerX,centerChunkY:options.centerY,
      panX:options.panX,panZ:options.panZ,
    };
    if(options.residentCloseup){
      const person=options.residents.find(r=>r.id===options.selectedResidentId);
      const pos=person&&this.continuity.positionFor(person);
      if(pos){
        const span=WORLD_GRID_CONTRACT.gridCellsPerChunk;
        const units=WORLD_GRID_CONTRACT.worldUnitsPerChunk;
        view.zoom=3.6;
        view.panX=(pos.x/span-view.centerChunkX-.5)*units;
        view.panZ=(pos.y/span-view.centerChunkY-.5)*units;
      }
    }
    this.motion.prune(options.residents);
    if(!options.paused)this.motionTime+=Math.min(500,started-this.lastDrawTime);
    this.lastDrawTime=started;
    const stats=paintFieldWorld(this.ctx,view,{...options,pixelRatio:this.canvas.width/(this.canvas.clientWidth||this.canvas.width)},this.continuity,this.atlas,
      this.motion,this.motionTime,this.terrainPainter);
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
      &&Math.hypot(m.x-p.x,Math.max(0,p.y-m.y,m.y-(m.bodyHeight??0)-p.y))<=m.radius);
    candidates.sort((a,b)=>Math.hypot(a.x-p.x,a.y-p.y)-Math.hypot(b.x-p.x,b.y-p.y));
    return candidates[0]?.id??null;
  }
  pickResident(x:number,y:number){return this.pick('resident',x,y);}
  pickHumanTrace(x:number,y:number){return this.pick('trace',x,y);}
  pickSettlement(x:number,y:number){return this.pick('settlement',x,y);}
  resetMotion():void {this.motion.reset();}
  dispose(){this.motion.reset();this.terrainPainter.clear();
    this.atlas.dispose();this.last=null;}
}
