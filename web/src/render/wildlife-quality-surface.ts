import * as THREE from 'three';
import type { WildlifeSpecies } from '../runtime/core-types';

// Q1: an original, seamlessly fused animal surface, not a pile of ellipsoids.
// The bounded signed-distance surface is authored from species-specific
// anatomical masses. Its normals come from the smooth implicit field, not
// independent primitive normals. Stable topology supports distance LOD.
export type WildlifeLod = 'near' | 'mid' | 'far';
type Volume = { center: THREE.Vector3; radius: THREE.Vector3; part: number; blend: number };
type Anatomy = { volumes: Volume[]; eye: THREE.Vector3; bounds: THREE.Box3 };
const V=(x:number,y:number,z:number)=>new THREE.Vector3(x,y,z);
const clamp=THREE.MathUtils.clamp;
function anatomy(species: WildlifeSpecies, sex:'Male'|'Female'): Anatomy {
  const volumes:Volume[]=[];
  const add=(x:number,y:number,z:number,rx:number,ry:number,rz:number,part=0,blend=.055)=>{
    volumes.push({center:V(x,y,z),radius:V(rx,ry,rz),part,blend});
  };
  const chain=(a:[number,number,number],b:[number,number,number],
    radius:number,part:number,steps=4)=>{
    for(let i=0;i<=steps;i++){
      const t=i/steps,p=V(...a).lerp(V(...b),t);
      const r=radius*(1-.16*t);
      add(p.x,p.y,p.z,r,r*1.16,r,part,.045);
    }
  };
  let eye=V(.13,.4,.34);
  if(species==='Hare'){
    add(0,.23,-.07,.19,.20,.32);
    add(0,.26,-.25,.205,.205,.18);
    add(0,.39,.20,.145,.146,.17,4);
    add(0,.32,.345,.10,.079,.098,4);
    for(const s of [-1,1]){
      chain([s*.13,.22,-.21],[s*.155,.08,-.27],.11,2);
      chain([s*.125,.23,.16],[s*.12,.065,.25],.07,1);
      add(s*.087,.605,.19,.040,.227,.053,4,.018);
    }
    add(0,.25,-.38,.083,.073,.070,3);
    eye=V(.121,.42,.34);
  }else if(species==='Deer'){
    add(0,.80,-.12,.265,.315,.67);
    add(0,.93,.31,.215,.325,.27);
    chain([0,.95,.29],[0,1.33,.53],.145,4,5);
    add(0,1.38,.61,.17,.16,.24,4);
    add(0,1.325,.83,.095,.085,.155,4);
    for(const s of [-1,1]){
      chain([s*.17,.75,.31],[s*.175,.11,.28],.074,1,9);
      chain([s*.18,.72,-.54],[s*.16,.11,-.52],.086,2,9);
      add(s*.22,1.485,.48,.125,.052,.078,4,.016);
      if(sex==='Male'){
        chain([s*.10,1.51,.45],[s*.24,1.86,.42],.030,4,6);
        chain([s*.21,1.71,.42],[s*.33,1.88,.58],.020,4,3);
        chain([s*.22,1.81,.41],[s*.12,2.04,.51],.018,4,3);
      }
    }
    add(0,1.04,-.75,.082,.086,.17,3);
    eye=V(.15,1.43,.75);
  }else if(species==='Wolf'||species==='Tiger'){
    const tiger=species==='Tiger',k=tiger?1.22:1;
    const shift=tiger?.04:0;
    add(0,.66*k,-.12,.265*k,.27*k,.58*k);
    add(0,.80*k,.29,.245*k,.275*k,.34);
    add(0,.81*k,-.52,.23*k,.26*k,.25);
    add(0,.90*k,.47,.21*k,.23*k,.27,4);
    add(0,.92*k,.66,.15*k,.128*k,.235,4);
    add(0,.80*k,.82,.103*k,.07*k,.13,4);
    for(const s of [-1,1]){
      chain([s*.18*k,.57*k,.32],[s*.2*k,.10*k,.34],.108*k,1,8);
      chain([s*.19*k,.53*k,-.48],[s*.20*k,.10*k,-.51],.105*k,2,8);
      add(s*.14*k,1.11*k,.52,.075*k,.15*k,.075*k,4,.024);
    }
    chain([0,.70*k,-.65],[.06,.49*k,-.98],.076*k,3,6);
    eye=V(.175*k,1.02*k,.73);
  }else {
    add(0,.16,0,.105,.132,.34,0,.036);
    add(0,.16,.18,.084,.095,.175,4,.023);
    add(0,.15,-.28,.043,.085,.22,3,.025);
    add(0,.255,-.08,.021,.15,.17,5,.014);
    for(const s of [-1,1])add(s*.066,.12,-.09,.015,.083,.135,5,.018);
    eye=V(.08,.18,.245);
  }
  const bounds=new THREE.Box3();
  for(const v of volumes){
    bounds.expandByPoint(v.center.clone().sub(v.radius));
    bounds.expandByPoint(v.center.clone().add(v.radius));
  }
  return {volumes,eye,bounds};
}
function field(point:THREE.Vector3, shapes:Volume[]):number {
  let result=Infinity;
  for(const shape of shapes) {
    const dx=(point.x-shape.center.x)/shape.radius.x;
    const dy=(point.y-shape.center.y)/shape.radius.y;
    const dz=(point.z-shape.center.z)/shape.radius.z;
    const distance=(Math.hypot(dx,dy,dz)-1)*Math.min(
      shape.radius.x,shape.radius.y,shape.radius.z);
    if(result===Infinity){result=distance;continue;}
    const k=shape.blend,h=clamp(.5+.5*(distance-result)/k,0,1);
    result=THREE.MathUtils.lerp(distance,result,h)-k*h*(1-h);
  }
  return result;
}
const TETS=[[0,5,1,6],[0,1,2,6],[0,2,3,6],
  [0,3,7,6],[0,7,4,6],[0,4,5,6]] as const;
const CORNERS=[[0,0,0],[1,0,0],[1,1,0],[0,1,0],
  [0,0,1],[1,0,1],[1,1,1],[0,1,1]] as const;
function speciesPigment(species:WildlifeSpecies,p:THREE.Vector3,part:number):THREE.Color {
  const h=p.y;
  const palettes:Record<WildlifeSpecies,[number,number,number]> = {
    Hare:[0x77604b,0xae9272,0xd9caba],
    Deer:[0x84563a,0xbc9065,0xd7c6a4],
    Wolf:[0x424a49,0x777d75,0xc0bbae],
    FreshwaterFish:[0x355b58,0x6d9b91,0xc4d6bd],
    Tiger:[0x9b5426,0xdf8b38,0xf1e2c5],
  };
  const [dark,main,light]=palettes[species];
  const t=species==='Tiger' ? clamp((h-.45)/.8,0,1)
    : species==='Deer' ? clamp((h-.6)/1.0,0,1)
    : clamp((h-.13)/.8,0,1);
  const underside=(p.y<.5 && p.z>-.65)?clamp((.5-p.y)*1.1,0,.72):0;
  const noise=Math.sin(p.x*71+p.z*19)*Math.cos(p.y*38-p.z*56);
  return new THREE.Color(main).lerp(new THREE.Color(dark),.16+t*.23)
    .lerp(new THREE.Color(light),underside)
    .multiplyScalar(1+noise*.045);
}
/** Deterministic fused-anatomy geometry, built once per species/LOD and
 * shared by instanced actors. Camera LOD only changes the surface density.
 */
export function createQualityWildlifeSurface(
  species:WildlifeSpecies,sex:'Male'|'Female',lod:WildlifeLod,
):THREE.BufferGeometry {
  const form=anatomy(species,sex);
  const bounds=form.bounds.clone().expandByScalar(.10);
  const size=bounds.getSize(new THREE.Vector3());
  const maxDimension=Math.max(size.x,size.y,size.z);
  const longest=lod==='near'?67:lod==='mid'?42:24;
  const step=maxDimension/longest;
  const nx=Math.ceil(size.x/step)+1,ny=Math.ceil(size.y/step)+1,nz=Math.ceil(size.z/step)+1;
  const count=nx*ny*nz;
  const values=new Float32Array(count);
  const sample=(x:number,y:number,z:number)=>x+nx*(y+ny*z);
  const point=V(0,0,0);
  for(let z=0;z<nz;z++)for(let y=0;y<ny;y++)for(let x=0;x<nx;x++){
    point.set(bounds.min.x+x*step,bounds.min.y+y*step,bounds.min.z+z*step);
    values[sample(x,y,z)]=field(point,form.volumes);
  }
  const vertices:number[]=[],normals:number[]=[],colors:number[]=[],parts:number[]=[];
  const speciesIds:number[]=[];
  const indexList:number[]=[];
  const currentPosition=V(0,0,0);
  const grade=(p:THREE.Vector3)=>{
    const delta=step*.5;
    const d=V(
      field(V(p.x+delta,p.y,p.z),form.volumes)-field(V(p.x-delta,p.y,p.z),form.volumes),
      field(V(p.x,p.y+delta,p.z),form.volumes)-field(V(p.x,p.y-delta,p.z),form.volumes),
      field(V(p.x,p.y,p.z+delta),form.volumes)-field(V(p.x,p.y,p.z-delta),form.volumes),
    );
    return d.lengthSq()>.0000001?d.normalize():V(0,1,0);
  };
  const partAt=(p:THREE.Vector3)=>{
    let best=Infinity,result=0;
    for(const vol of form.volumes){
      const distance=p.distanceToSquared(vol.center)
        /(vol.radius.lengthSq()+.00001);
      if(distance<best){best=distance;result=vol.part;}
    }
    return result;
  };
  const speciesIndex:{[k in WildlifeSpecies]:number}={
    Hare:0,Deer:1,Wolf:2,FreshwaterFish:3,Tiger:4,
  };
  const triangle=(a:THREE.Vector3,b:THREE.Vector3,c:THREE.Vector3)=>{
    const normalA=grade(a),normalB=grade(b),normalC=grade(c);
    const cross=new THREE.Vector3().subVectors(b,a)
      .cross(new THREE.Vector3().subVectors(c,a));
    const orient=cross.dot(normalA.clone().add(normalB).add(normalC));
    const face=orient<0?[a,c,b]:[a,b,c];
    for(const pos of face){
      vertices.push(pos.x,pos.y,pos.z);
      const n=grade(pos); normals.push(n.x,n.y,n.z);
      const col=speciesPigment(species,pos,partAt(pos));
      colors.push(col.r,col.g,col.b);
      parts.push(partAt(pos));speciesIds.push(speciesIndex[species]);
    }
    const offset=vertices.length/3;
    indexList.push(offset-3,offset-2,offset-1);
  };
  const crossing=(a:number,b:number,positions:THREE.Vector3[],v:number[])=>
    positions[a].clone().lerp(positions[b],clamp(v[a]/(v[a]-v[b]),0,1));
  const corners:THREE.Vector3[]=Array.from({length:8},()=>V(0,0,0));
  for(let z=0;z<nz-1;z++)for(let y=0;y<ny-1;y++)for(let x=0;x<nx-1;x++){
    const values8:number[]=[];
    for(let c=0;c<8;c++){
      const [dx,dy,dz]=CORNERS[c];
      corners[c].set(bounds.min.x+(x+dx)*step,
        bounds.min.y+(y+dy)*step,bounds.min.z+(z+dz)*step);
      values8[c]=values[sample(x+dx,y+dy,z+dz)];
    }
    if(values8.every(v=>v>=0)||values8.every(v=>v<0))continue;
    for(const tet of TETS){
      const inside=tet.filter(i=>values8[i]<0),outside=tet.filter(i=>values8[i]>=0);
      if(!inside.length||!outside.length)continue;
      if(inside.length===1){
        const a=inside[0];
        triangle(...outside.map(i=>crossing(a,i,corners,values8)) as
          [THREE.Vector3,THREE.Vector3,THREE.Vector3]);
      } else if(inside.length===3){
        const a=outside[0];
        triangle(...inside.map(i=>crossing(a,i,corners,values8)) as
          [THREE.Vector3,THREE.Vector3,THREE.Vector3]);
      } else {
        const a=crossing(inside[0],outside[0],corners,values8);
        const b=crossing(inside[0],outside[1],corners,values8);
        const c=crossing(inside[1],outside[0],corners,values8);
        const d=crossing(inside[1],outside[1],corners,values8);
        triangle(a,c,b);triangle(b,c,d);
      }
    }
  }
  const geo=new THREE.BufferGeometry();
  geo.setAttribute('position',new THREE.Float32BufferAttribute(vertices,3));
  geo.setAttribute('normal',new THREE.Float32BufferAttribute(normals,3));
  geo.setAttribute('color',new THREE.Float32BufferAttribute(colors,3));
  geo.setAttribute('wildlifePart',new THREE.Float32BufferAttribute(parts,1));
  geo.setAttribute('wildlifeSpeciesId',new THREE.Float32BufferAttribute(speciesIds,1));
  geo.setIndex(indexList);
  geo.computeBoundingBox();
  // The height is Core-owned, and the author's local minimum is the sole.
  geo.translate(0,-geo.boundingBox!.min.y,0);
  geo.computeBoundingBox();geo.computeBoundingSphere();
  geo.userData.wildlifeAuthoredVersion='Q1';
  geo.userData.wildlifeLod=lod;
  geo.userData.wildlifeSurfaceKind='fused-implicit';
  if(vertices.length<300)throw new Error('Empty quality wildlife surface: '+species);
  return geo;
}
