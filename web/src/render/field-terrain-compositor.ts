import type {TerrainChunk} from '../runtime/core-types';
import {fieldNaturalFloorColor} from './field-natural-art';

export type FieldTerrainLookup=(x:number,y:number)=>TerrainChunk|undefined;
export interface GroundScreenPoint {x:number;y:number}
export type GroundProject=(x:number,y:number,h:number)=>GroundScreenPoint;
export type GroundElevation=(chunkX:number,chunkY:number,u:number,v:number)=>number;

const limit=(n:number,a=0,b=1)=>Math.max(a,Math.min(b,n));
const smooth=(n:number)=>{const x=limit(n);return x*x*(3-2*x);};
const parse=(str:string):[number,number,number]=>{
  const c=Number.parseInt(str.replace('#',''),16);
  if(!Number.isFinite(c))return [103,147,104];
  return [(c>>16)&255,(c>>8)&255,c&255];
};
const mix=(a:readonly number[],b:readonly number[],f:number):[number,number,number]=>{
  const t=limit(f);return [a[0]*(1-t)+b[0]*t,a[1]*(1-t)+b[1]*t,a[2]*(1-t)+b[2]*t];
};
const isWater=(c:TerrainChunk)=>c.waterKind==='Ocean'||c.waterKind==='Lake';
const code=(seed:string)=>{
  let h=2166136261>>>0;
  for(let i=0;i<seed.length;i++)h=Math.imul(h^seed.charCodeAt(i),16777619)>>>0;
  return (h>>>0)/4294967296;
};
/**
 * Every land pixel is interpolated between the same four authoritative Core
 * chunk centers on both sides of an edge. No rectangle of a single chunk
 * color is allowed. Never interpolate a land pixel into a Core water chunk.
 */
export function fieldWorldGroundColor(
  seed:string,chunkX:number,chunkY:number,u:number,v:number,
  lookup:FieldTerrainLookup,
  palette?:(chunk:TerrainChunk)=>[number,number,number],
):[number,number,number] {
  const chunk=lookup(chunkX,chunkY);
  if(!chunk)return [102,145,97];
  const readPalette=palette??((c:TerrainChunk)=>parse(fieldNaturalFloorColor(c)));
  const water=isWater(chunk),self=readPalette(chunk);
  const coord=(i:number,t:number):[number,number,number]=>{
    const global=i+t-.5,low=Math.floor(global);
    return [low,low+1,smooth(global-low)];
  };
  const [ax,bx,tx]=coord(chunkX,u),[ay,by,ty]=coord(chunkY,v);
  const tile=(x:number,y:number)=>{
    const c=lookup(x,y);
    if(c&&isWater(c)===water)return readPalette(c);
    // Missing diagonal samples must use the same world-coordinate fallback
    // from either side of a seam; using the *current chunk* created lines.
    for(const [dx,dy] of [[0,1],[-1,0],[1,0],[0,-1],
      [-1,1],[1,1],[-1,-1],[1,-1]]){
      const near=lookup(x+dx,y+dy);
      if(near&&isWater(near)===water)return readPalette(near);
    }
    return self;
  };
  const row0=mix(tile(ax,ay),tile(bx,ay),tx);
  const row1=mix(tile(ax,by),tile(bx,by),tx);
  const rgb=mix(row0,row1,ty);
  // Continuous, bounded painterly pigment in absolute world coordinates.
  // Low-frequency meadows and small grain are non-interactive decoration.
  const gx=chunkX+u-.5,gy=chunkY+v-.5,phase=code(seed)*17.3;
  const meadow=Math.sin(gx*5.8+phase)*Math.cos(gy*6.3-phase*.7)*.55
    +Math.cos(gx*11.4+gy*8.1+phase)*.22;
  const fiber=Math.sin(gx*84.1+gy*18.3+phase)*Math.cos(gy*79.9-gx*7.4)*.15;
  const amount=water?10:20;
  const variance=(meadow+fiber)*amount;
  const forest=limit(chunk.forestCoverage01??0),rock=limit(chunk.rockCoverage01??0);
  const warm=(Math.sin(gx*18+phase)*Math.sin(gy*13-phase)*.5+.5);
  // A shoreline is a color transition *on the already authoritative side*
  // of the water/land boundary. It never converts a Core land tile to water.
  const waterNeighbor=[
    {edge:u,found:lookup(chunkX-1,chunkY)},
    {edge:1-u,found:lookup(chunkX+1,chunkY)},
    {edge:v,found:lookup(chunkX,chunkY-1)},
    {edge:1-v,found:lookup(chunkX,chunkY+1)},
  ].filter(e=>e.found&&isWater(e.found)!==water);
  const edge=waterNeighbor.length?Math.min(...waterNeighbor.map(e=>e.edge)):1;
  const ripple=.012*Math.sin((gx+gy)*21+phase)+.009*Math.cos((gx-gy)*31+phase);
  const shallow=limit((.09-edge+ripple)/.09);
  const target=water?[131,181,166]:[157,166,117];
  return [
    limit(rgb[0]+variance+rock*warm*4,0,255)*(1-shallow*.40)+target[0]*shallow*.40,
    limit(rgb[1]+variance*.88+forest*warm*5,0,255)*(1-shallow*.40)+target[1]*shallow*.40,
    limit(rgb[2]+variance*.49-rock*warm*3,0,255)*(1-shallow*.40)+target[2]*shallow*.40,
  ];
}

/** Offscreen textures are created once per changed Core snapshot and reused
 * during zoom, pan and animation. Capped for mobile RAM and Canvas2D budget. */
export class FieldTerrainCompositor {
  private readonly textures=new Map<string,HTMLCanvasElement>();
  private readonly limit=112;
  private readonly size=80;
  private create(seed:string,chunk:TerrainChunk,lookup:FieldTerrainLookup){
    if(typeof document==='undefined')return null;
    const tex=document.createElement('canvas');
    tex.width=this.size;tex.height=this.size;
    const ctx=tex.getContext('2d');
    if(!ctx||typeof ctx.createImageData!=='function')return null;
    const img=ctx.createImageData(this.size,this.size),d=img.data;
    // Each chunk's Core palette is stable for this texture snapshot. Reusing
    // its nine neighbors avoids millions of repeated color/biome conversions.
    const colors=new WeakMap<TerrainChunk,[number,number,number]>();
    const readPalette=(c:TerrainChunk):[number,number,number]=>{
      let rgb=colors.get(c);
      if(!rgb){rgb=parse(fieldNaturalFloorColor(c));colors.set(c,rgb);}
      return rgb;
    };
    for(let j=0;j<this.size;j++)for(let i=0;i<this.size;i++){
      const u=(i+.5)/this.size,v=(j+.5)/this.size;
      const [r,g,b]=fieldWorldGroundColor(seed,chunk.x,chunk.y,u,v,lookup,readPalette);
      const k=(j*this.size+i)*4;
      d[k]=r;d[k+1]=g;d[k+2]=b;d[k+3]=255;
    }
    ctx.putImageData(img,0,0);
    // Authored brushwork stays inside the cached texture, never 80 repeated
    // V-shaped marks floating above the scene. These are purely pigment.
    let randomSeed=Math.floor(code(seed+':ground:'+chunk.x+':'+chunk.y)*4294967295)||1;
    const rand=()=>{randomSeed^=randomSeed<<13;randomSeed^=randomSeed>>>17;
      randomSeed^=randomSeed<<5;return (randomSeed>>>0)/4294967296;};
    const grass=limit(chunk.grassCoverage01??0);
    const forest=limit(chunk.forestCoverage01??0);
    const rock=limit(chunk.rockCoverage01??0);
    const water=isWater(chunk);
    if(water){
      const count=10+Math.floor(rand()*7);
      for(let i=0;i<count;i++){
        const x=rand()*this.size,y=rand()*this.size,w=2+rand()*9;
        ctx.strokeStyle=`rgba(216, 234, 208, ${(.12+rand()*.15).toFixed(3)})`;
        ctx.lineWidth=.65+rand()*.45;
        ctx.beginPath();ctx.moveTo(x-w*.5,y);
        ctx.quadraticCurveTo(x,y-1.2,x+w*.5,y-.3);ctx.stroke();
      }
    }else{
      // Diffuse meadow patches, with smaller muted plant and earth gestures.
      const patchCount=Math.round(grass*6+forest*6+rock*2);
      for(let i=0;i<patchCount;i++){
        const x=rand()*this.size,y=rand()*this.size,r=4+rand()*11;
        const g=ctx.createRadialGradient(x,y,0,x,y,r);
        const soil=rand()<(rock*.34+.12);
        g.addColorStop(0,soil?'rgba(109,109,80,.12)':'rgba(172,186,109,.18)');
        g.addColorStop(1,'rgba(100,125,80,0)');
        ctx.fillStyle=g;ctx.beginPath();
        ctx.ellipse(x,y,r,r*(.52+rand()*.43),rand()-.5,0,Math.PI*2);
        ctx.fill();
      }
      const clusters=Math.floor(grass*26+forest*19+rock*7);
      for(let i=0;i<clusters;i++){
        const x=2+rand()*(this.size-4),y=2+rand()*(this.size-4);
        const localGrass=rand()<(grass+forest*.20);
        if(localGrass){
          const blades=2+Math.floor(rand()*3);
          const len=1.2+rand()*3.1;
          ctx.fillStyle=rand()<.5?'rgba(190,203,129,.37)':'rgba(48,101,60,.24)';
          for(let n=0;n<blades;n++){
            const dx=(n-(blades-1)/2)*1.7;
            ctx.beginPath();ctx.moveTo(x+dx,y+.8);
            ctx.quadraticCurveTo(x+dx-1,y-len*.65,x+dx+.2,y-len);
            ctx.lineTo(x+dx+.8,y+.7);ctx.closePath();ctx.fill();
          }
        }else if(rock>.17){
          ctx.fillStyle=rand()<.5?'rgba(183,178,143,.34)':'rgba(79,91,76,.28)';
          ctx.beginPath();ctx.moveTo(x-2,y+1);
          ctx.lineTo(x-1,y-1.8);ctx.lineTo(x+2.1,y-1.1);
          ctx.lineTo(x+2.7,y+1);ctx.closePath();ctx.fill();
        }else {
          ctx.strokeStyle='rgba(105,93,64,.22)';ctx.lineWidth=.7;
          ctx.beginPath();ctx.moveTo(x-2,y);
          ctx.quadraticCurveTo(x,y-1.2,x+3,y+.3);ctx.stroke();
        }
      }
      // Leaf litter only where Core records actual forest habitat.
      const litter=Math.floor(forest*27);
      for(let i=0;i<litter;i++){
        const x=rand()*this.size,y=rand()*this.size;
        ctx.fillStyle=rand()<.55?'rgba(169,139,80,.31)':'rgba(114,145,74,.39)';
        ctx.beginPath();ctx.ellipse(x,y,.8+rand()*1.6,.35+rand()*.8,
          rand()*Math.PI,0,Math.PI*2);ctx.fill();
      }
    }
    return tex;
  }
  paint(
    ctx:CanvasRenderingContext2D,seed:string,chunk:TerrainChunk,
    lookup:FieldTerrainLookup,project:GroundProject,elev:GroundElevation,
  ):boolean {
    if(typeof ctx.drawImage!=='function'||typeof document==='undefined')return false;
    const neighbors=[-1,0,1].flatMap(dy=>[-1,0,1].map(dx=>{
      const c=lookup(chunk.x+dx,chunk.y+dy);
      return c?[
        c.waterKind,c.elevation01,c.grassCoverage01,c.forestCoverage01,
        c.rockCoverage01,c.wetlandCoverage01,c.shrubCoverage01,
      ].join(','):'-';
    }));
    const key=JSON.stringify([seed,chunk.x,chunk.y,...neighbors]);
    let image=this.textures.get(key);
    if(!image) {
      const created=this.create(seed,chunk,lookup);
      if(!created)return false;
      if(this.textures.size>=this.limit){
        const first=this.textures.keys().next().value;
        if(first)this.textures.delete(first);
      }
      this.textures.set(key,created);image=created;
    }
    // Preserve actual observed height variation with 4x4 source slices;
    // 1px overlap avoids tile seams from half-pixel canvas rounding.
    for(let y=0;y<4;y++)for(let x=0;x<4;x++){
      const u=x/4,v=y/4,du=.25;
      const a=elev(chunk.x,chunk.y,u,v);
      const b=elev(chunk.x,chunk.y,u+du,v);
      const c=elev(chunk.x,chunk.y,u+du,v+du);
      const d=elev(chunk.x,chunk.y,u,v+du);
      const p=project(chunk.x+u-.5,chunk.y+v-.5,(a+b+d)*.333333);
      const q=project(chunk.x+u+du-.5,chunk.y+v+du-.5,(b+c+d)*.333333);
      const x0=Math.min(p.x,q.x),y0=Math.min(p.y,q.y),w=Math.abs(q.x-p.x),h=Math.abs(q.y-p.y);
      if(w<.1||h<.1)continue;
      ctx.drawImage(image,x*this.size/4,y*this.size/4,
        this.size/4,this.size/4,x0-.35,y0-.35,w+.7,h+.7);
      // Readable actual relief, rather than invented decorative cliffs.
      // Descending Core elevations cast a restrained terrain ledge shadow.
      const slope=Math.max(Math.abs(a-b),Math.abs(a-d),Math.abs(c-b),Math.abs(c-d));
      if(!isWater(chunk)&&slope>.043){
        const weight=Math.min(1,(slope-.043)*6.5);
        const high=a>d?a:d,low=b<c?b:c;
        const edge0=project(chunk.x+u-.5,chunk.y+v+du-.5,high);
        const edge1=project(chunk.x+u+du-.5,chunk.y+v+du-.5,low);
        ctx.beginPath();ctx.moveTo(edge0.x,edge0.y);
        ctx.lineTo(edge1.x,edge1.y);
        ctx.lineWidth=1.4+weight*3.6;
        ctx.strokeStyle=`rgba(41, 64, 52, ${(.12+weight*.26).toFixed(3)})`;
        ctx.stroke();
      }
    }
    return true;
  }
  clear(){this.textures.clear();}
}

/**
 * Only a Core-observed land/water boundary gets a soft shore highlight.
 * Slight bends stay on the exact boundary; no river, lake or coast is created.
 */
export function paintFieldShore(
 ctx:CanvasRenderingContext2D,chunk:TerrainChunk,lookup:FieldTerrainLookup,
 project:GroundProject,elev:GroundElevation,seed:string,
):void {
  if(chunk.waterKind!=='Lake'&&chunk.waterKind!=='Ocean')return;
  const same=(x:number,y:number)=>{
    const other=lookup(x,y);return other&&isWater(other);
  };
  const sides=[
    {dx:0,dy:-1,from:[0,0],to:[1,0]},
    {dx:1,dy:0,from:[1,0],to:[1,1]},
    {dx:0,dy:1,from:[0,1],to:[1,1]},
    {dx:-1,dy:0,from:[0,0],to:[0,1]},
  ];
  const phase=code(seed+chunk.x+':'+chunk.y)*6.28;
  for(const side of sides){
    const other=lookup(chunk.x+side.dx,chunk.y+side.dy);
    if(!other||same(chunk.x+side.dx,chunk.y+side.dy))continue;
    ctx.beginPath();
    for(let k=0;k<=12;k++){
      const t=k/12;
      const u=side.from[0]*(1-t)+side.to[0]*t;
      const v=side.from[1]*(1-t)+side.to[1]*t;
      const h=elev(chunk.x,chunk.y,u,v);
      const p=project(chunk.x+u-.5,chunk.y+v-.5,h+.002);
      const d=Math.sin(t*11+phase)*.0055;
      if(k===0)ctx.moveTo(p.x,p.y);
      else ctx.lineTo(p.x+(side.dx!==0?d:0),p.y+(side.dy!==0?d:0));
    }
    ctx.strokeStyle='rgba(225,225,181,.46)';ctx.lineWidth=2;
    ctx.stroke();
  }
}
