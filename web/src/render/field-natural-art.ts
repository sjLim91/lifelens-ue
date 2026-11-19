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

export interface FieldCanopySprite {
  u:number;v:number;name:Extract<FieldSpriteName,'pine'|'cedar'|'oak'>;
  sizeFactor:number;
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
  const count=forest<.18?0:Math.min(7,Math.max(1,Math.ceil(forest*6.1)));
  const cold=/taiga|boreal|tundra|conifer/i.test(biome);
  const unit=(salt:number)=>fieldVisualUnit(seed,chunkX,chunkY,salt);
  const groups=count<4?1:2;
  const centers=[
    {u:.26+unit(301)*.12,v:.27+unit(302)*.12},
    {u:.69+unit(303)*.09,v:.67+unit(304)*.09},
  ];
  const out:FieldCanopySprite[]=[];
  for(let i=0;i<count;i++){
    const g=i%groups,rank=Math.floor(i/groups),a=unit(310+i)*Math.PI*2;
    const spread=rank===0?0:.09+Math.sqrt(rank)*.046;
    const u=Math.min(.91,Math.max(.09,centers[g].u+Math.cos(a)*spread));
    const v=Math.min(.91,Math.max(.09,centers[g].v+Math.sin(a)*spread));
    const shape=unit(350+i);
    const name:FieldCanopySprite['name']=cold
      ?(shape>.54?'cedar':'pine')
      :(shape<.30?'pine':shape<.55?'cedar':'oak');
    out.push({u,v,name,sizeFactor:.24+unit(370+i)*.115});
  }
  return out;
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
