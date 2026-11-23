import type { TerrainChunk } from '../runtime/core-types';
import { terrainColor } from './terrain-presentation';
import { fieldVisualUnit } from './field-scene-clarity';
import type { FieldSpriteName } from './field-asset-atlas';

const clamp=(n:unknown):number=>typeof n==='number'&&Number.isFinite(n)
  ?Math.max(0,Math.min(1,n)):0;

/** Illustrated surface palette: every hint derives from real Core coverage. */
export function fieldNaturalFloorColor(chunk:TerrainChunk):string {
  const water=chunk.waterKind;
  if(water==='Ocean')return '#508c9a';
  if(water==='Lake')return '#75aaa5';
  if(water==='Coast')return '#b9b58b';
  const initial=terrainColor(chunk);
  const grass=clamp(chunk.grassCoverage01),rock=clamp(chunk.rockCoverage01);
  const wet=clamp(chunk.wetlandCoverage01),wood=clamp(chunk.forestCoverage01);
  const target=wet>.54?'#699a80':rock>.65?'#969680'
    :wood>.6?'#648b5b':'#83ad70';
  const strength=.18+grass*.32+wet*.14;
  const from=Number.parseInt(initial.slice(1),16);
  const to=Number.parseInt(target.slice(1),16);
  const color=(shift:number)=>Math.round(((from>>shift)&255)*(1-strength)
    +((to>>shift)&255)*strength).toString(16).padStart(2,'0');
  return '#'+color(16)+color(8)+color(0);
}

export interface FieldSurfaceNeighbors {
  west?:TerrainChunk; east?:TerrainChunk; north?:TerrainChunk; south?:TerrainChunk;
}
const blendHex=(a:string,b:string,t:number):string=>{
  const alpha=Math.max(0,Math.min(1,t));
  const from=Number.parseInt(a.slice(1),16),to=Number.parseInt(b.slice(1),16);
  const channel=(shift:number)=>Math.round(((from>>shift)&255)*(1-alpha)
    +((to>>shift)&255)*alpha).toString(16).padStart(2,'0');
  return '#'+channel(16)+channel(8)+channel(0);
};

/**
 * Adjacent observed land chunks transition in tone rather than looking like
 * isolated geometric tiles. Never smear land across Core water borders.
 */
export function fieldConnectedFloorColor(
  chunk:TerrainChunk,neighbor:FieldSurfaceNeighbors,u:number,v:number,
):string {
  let tone=fieldNaturalFloorColor(chunk);
  const water=(c:TerrainChunk)=>c.waterKind==='Ocean'||c.waterKind==='Lake';
  const same=(other:TerrainChunk|undefined)=>Boolean(other
    &&water(other)===water(chunk));
  const x=Math.max(0,Math.min(1,u)),y=Math.max(0,Math.min(1,v));
  if(x<.5&&same(neighbor.west))tone=blendHex(tone,
    fieldNaturalFloorColor(neighbor.west!),(.5-x));
  if(x>.5&&same(neighbor.east))tone=blendHex(tone,
    fieldNaturalFloorColor(neighbor.east!),(x-.5));
  if(y<.5&&same(neighbor.north))tone=blendHex(tone,
    fieldNaturalFloorColor(neighbor.north!),(.5-y));
  if(y>.5&&same(neighbor.south))tone=blendHex(tone,
    fieldNaturalFloorColor(neighbor.south!),(y-.5));
  return tone;
}

export interface FieldCanopySprite {
  u:number;v:number;name:Extract<FieldSpriteName,'pine'|'cedar'|'oak'>;
  sizeFactor:number;
}
const canopyLayoutCache=new Map<string,FieldCanopySprite[]>();
const groundLayoutCache=new Map<string,FieldGroundMark[]>();
const MAX_VISUAL_LAYOUTS=384;
function cachedLayout<T>(cache:Map<string,T>,key:string,compute:()=>T):T {
  if(cache.has(key))return cache.get(key)!;
  const value=compute();
  if(cache.size>=MAX_VISUAL_LAYOUTS){
    const oldest=cache.keys().next().value;
    if(oldest!==undefined)cache.delete(oldest);
  }
  cache.set(key,value);
  return value;
}

/**
 * A few spatially coherent canopy groups, instead of independently scattering
 * every sprite over the whole chunk. This is decorative foliage only:
 * never a Core tree/resource, obstacle or evidence of new wildlife.
 */
export function fieldForestDisplayClusters(
  seed:string,chunkX:number,chunkY:number,
  coverage:number,biome:string,
):FieldCanopySprite[] {
  const forest=clamp(coverage);
  if(forest<.18)return [];
  return cachedLayout(canopyLayoutCache,
    JSON.stringify([seed,chunkX,chunkY,forest,biome]),()=>{
  const count=Math.min(9,Math.max(1,Math.ceil(forest*8)));
  const cold=/taiga|boreal|tundra|conifer/i.test(biome);
  const unit=(salt:number)=>fieldVisualUnit(seed,chunkX,chunkY,salt);
  // The previous two fixed NW/SE anchors repeated on every Core chunk and
  // were visible as a checkerboard at mobile zoom. Candidate plants now form
  // patches selected by continuous world-coordinate canopy potential.
  // Coverage still determines the *maximum* display density; no real tree,
  // ecological inventory or obstacle is created.
  const candidates:Array<FieldCanopySprite&{score:number}>=[];
  for(let index=0;index<25;index++){
    const x=index%5,y=Math.floor(index/5);
    const u=.03+(x+.12+unit(600+index*2)*.76)*.188;
    const v=.03+(y+.12+unit(601+index*2)*.76)*.188;
    const coarse=fieldContinuousGroundNoise(seed,(chunkX+u)*.58,(chunkY+v)*.58);
    const fine=fieldContinuousGroundNoise(seed,(chunkX+u)*1.43,(chunkY+v)*1.43);
    const score=coarse*.64+fine*.30+(unit(850+index)-.5)*.18;
    const shape=unit(1000+index);
    const name:FieldCanopySprite['name']=cold
      ?(shape>.54?'cedar':'pine')
      :(shape<.29?'pine':shape<.54?'cedar':'oak');
    candidates.push({
      u:Math.min(.96,Math.max(.04,u)),v:Math.min(.96,Math.max(.04,v)),
      name,sizeFactor:.21+unit(1200+index)*.13,score,
    });
  }
  candidates.sort((a,b)=>b.score-a.score||a.u-b.u);
  return candidates.slice(0,count).map(({u,v,name,sizeFactor})=>({
    u,v,name,sizeFactor,
  }));
  });
}

/**
 * Pure slope evidence: an image of a sheer cliff is only justified where the
 * real Core elevation profile contains a significant local drop.
 */
export function fieldObservedRockFace(
  samples:readonly number[],rockCoverage:number,
):boolean {
  if(clamp(rockCoverage)<.48||samples.length<4||samples.some(s=>!Number.isFinite(s)))
    return false;
  return Math.max(...samples)-Math.min(...samples)>.095;
}


/**
 * A close camera changes how much ground is visible, not the physical scale
 * of a bonfire, bed or hare. Beyond the close-observer threshold, the sprite
 * magnification slows while its grounding/selection still follows full zoom.
 */
export function fieldVisualSpriteTile(tile:number,width:number,height:number):number {
  const unzoomed=Math.max(16,Math.min(width*.49,height*.35));
  return Math.min(tile,unzoomed*1.43);
}

/** Seamless seed-derived ground pigment. Absolute Core chunk coordinates,
 * not per-chunk random seeds, give identical values at chunk boundaries. */
export function fieldContinuousGroundNoise(
  seed:string,globalX:number,globalY:number,
):number {
  const layer=(frequency:number,salt:number):number=>{
    const x=globalX*frequency,y=globalY*frequency;
    const ix=Math.floor(x),iy=Math.floor(y);
    const smooth=(n:number)=>n*n*(3-2*n);
    const tx=smooth(x-ix),ty=smooth(y-iy);
    const n00=fieldVisualUnit(seed,ix,iy,salt);
    const n10=fieldVisualUnit(seed,ix+1,iy,salt);
    const n01=fieldVisualUnit(seed,ix,iy+1,salt);
    const n11=fieldVisualUnit(seed,ix+1,iy+1,salt);
    return (n00*(1-tx)+n10*tx)*(1-ty)
      +(n01*(1-tx)+n11*tx)*ty;
  };
  return (layer(1.85,702)*.72+layer(6.7,703)*.28)*2-1;
}

/** World-coordinate seed details do not shift when the user pans/zooms.
 * Decorative marks follow habitat coverage; none is harvestable. */
export interface FieldGroundMark {
  u:number;v:number;length:number;rotation:number;
  kind:'grass'|'lichen'|'soil';
}
export function fieldGroundDetailMarks(
  seed:string,chunkX:number,chunkY:number,
  grassCoverage:number|undefined,forestCoverage:number|undefined,
  rockCoverage:number|undefined,
):FieldGroundMark[] {
  const grass=clamp(grassCoverage),forest=clamp(forestCoverage),
    rock=clamp(rockCoverage);
  // An open field still needs fine habitat detail at close observer zoom.
  // The renderer applies its own camera-dependent budget, so this density
  // stays lightweight when observing many chunks at once.
  const count=Math.min(80,Math.floor(grass*48+forest*27+rock*9));
  if(count===0)return [];
  return cachedLayout(groundLayoutCache,
    JSON.stringify([seed,chunkX,chunkY,grass,forest,rock]),()=>{
  const out:FieldGroundMark[]=[];
  for(let i=0;i<count;i++){
    const u=.045+fieldVisualUnit(seed,chunkX,chunkY,1700+i*5)*.91;
    const v=.045+fieldVisualUnit(seed,chunkX,chunkY,1701+i*5)*.91;
    const choice=fieldVisualUnit(seed,chunkX,chunkY,1702+i*5);
    const kind:FieldGroundMark['kind']=choice<rock*.32?'lichen'
      :choice<.17?'soil':'grass';
    out.push({u,v,kind,length:.017+fieldVisualUnit(seed,chunkX,chunkY,1703+i*5)*.031,
      rotation:(fieldVisualUnit(seed,chunkX,chunkY,1704+i*5)-.5)*.95});
  }
  return out;
  });
}
