import * as THREE from 'three';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import type { WildlifeSpecies } from '../runtime/core-types';

// E1: authored quadruped anatomy and distinctive pelage, generated locally.
// One merged geometry per species/sex: no downloads or paid models.
// Visual articulation tags: 0 trunk, 1 front legs, 2 hind legs, 3 tail,
// 4 head/neck/ears, 5 fins. These drive cheap per-instance WebGL movement.
export function createWildlifeGeometry(species: WildlifeSpecies, sex: 'Male' | 'Female' = 'Female'): THREE.BufferGeometry {
  const pieces: THREE.BufferGeometry[] = [];
  const add = (geo: THREE.BufferGeometry, tint: number, region = 0): void => {
    const n = geo.getAttribute('position').count;
    const color = new Float32Array(n * 3);
    const part = new Float32Array(n);
    const rgb = new THREE.Color(tint);
    for (let i = 0; i < n; i++) {
      rgb.toArray(color, 3*i);
      part[i] = region;
    }
    geo.setAttribute('color', new THREE.BufferAttribute(color, 3));
    geo.setAttribute('wildlifePart', new THREE.BufferAttribute(part, 1));
    pieces.push(geo);
  };
  const oval = (c: number, x: number, y: number, z: number,
    sx: number, sy: number, sz: number, region = 0, tilt = 0): void => {
    const geo = new THREE.SphereGeometry(1, 16, 12);
    geo.scale(sx, sy, sz);
    geo.rotateZ(tilt);
    geo.translate(x,y,z);
    add(geo,c,region);
  };
  const segment = (color: number, points: [number,number,number][],
    radius: number, region = 0, segments = 10): void => {
    const path = new THREE.CatmullRomCurve3(points.map(p=>new THREE.Vector3(...p)));
    add(new THREE.TubeGeometry(path, segments, radius, 6, false),color,region);
  };
  const eyes = (x: number, y: number, z: number, c=0x201c15, region=4): void => {
    for(const s of [-1,1]) {
      oval(0xf4dcb2,s*x,y,z,.034,.027,.014,region);
      oval(c,s*x,y,z+.012,.016,.022,.010,region);
      oval(0xffffff,s*x-.004,y+.007,z+.021,.006,.006,.004,region);
    }
  };
  const paws = (c:number, x:number, y:number, z:number, sy:number, depth:number, region:number) => {
    oval(c,x,y,z,.072,sy,depth,region);
    oval(0x38322c,x,y-sy*.92,z+depth*.30,.067,.031,.090,region);
  };
  if(species==='Hare') {
    oval(0x786553,0,.22,-.08,.18,.18,.29);
    oval(0xa88c72,0,.16,.02,.145,.10,.25);
    oval(0x9d8166,0,.36,.20,.14,.15,.16,4);
    oval(0xd9c8ab,0,.28,.32,.115,.085,.09,4);
    for(const s of [-1,1]) {
      oval(0x75543f,s*.085,.60,.22,.043,.23,.054,4,s*-.15);
      oval(0xc68a88,s*.087,.615,.26,.023,.186,.020,4,s*-.15);
      paws(0x95765d,s*.125,.105,-.20,.10,.13,2);
      paws(0x917b63,s*.12,.095,.18,.09,.080,1);
      oval(0x45382e,s*.135,.37,.305,.034,.033,.025,4);
    }
    oval(0xe6dfc9,0,.24,-.36,.088,.09,.075,3);
    oval(0x533b36,0,.29,.425,.025,.018,.016,4);
    eyes(.107,.399,.340);
    for(const s of [-1,1]) for(const k of [-1,0,1])
      segment(0xc2b3a0,[[s*.045,.288,.398],[s*.14,.29+k*.025,.42],
        [s*.215,.27+k*.046,.41]],.002,4,5);
  } else if(species==='Deer') {
    oval(0x8e6546,0,.79,-.16,.26,.34,.62);
    oval(0xbf956c,0,.64,.02,.215,.20,.47);
    oval(0xa27753,0,1.13,.39,.145,.30,.19,4,-.15);
    oval(0xa27753,0,1.36,.55,.18,.17,.23,4);
    oval(0xddcab0,0,1.28,.70,.105,.078,.19,4);
    oval(0x3c3030,0,1.34,.86,.063,.044,.046,4);
    for(const s of [-1,1]) {
      oval(0xa47d58,s*.215,1.47,.47,.115,.047,.075,4,s*.35);
      paws(0x8a684d,s*.17,.34,.25,.31,.073,1);
      paws(0x836346,s*.175,.34,-.53,.32,.079,2);
      if(sex==='Male') {
        segment(0x917255,[[s*.105,1.48,.40],[s*.16,1.67,.34],
          [s*.25,1.93,.38],[s*.28,2.00,.44]],.028,4,14);
        segment(0x937656,[[s*.17,1.72,.35],[s*.31,1.78,.48],
          [s*.34,1.9,.50]],.018,4,9);
        segment(0x937656,[[s*.20,1.83,.37],[s*.11,2.02,.45]],.016,4,7);
      }
    }
    oval(0xd9c6a2,0,1.03,-.75,.075,.10,.12,3);
    eyes(.142,1.415,.720);
  } else if(species==='Wolf') {
    oval(0x666f70,0,.63,-.12,.255,.27,.57);
    oval(0x909391,0,.58,.05,.20,.15,.38);
    oval(0x7d8580,0,.81,.30,.21,.24,.23,4);
    oval(0x68736e,0,.91,.52,.18,.18,.20,4);
    oval(0xc5c5ad,0,.80,.69,.13,.080,.20,4);
    oval(0x272b28,0,.79,.88,.058,.039,.043,4);
    for(const s of [-1,1]) {
      oval(0x68706a,s*.14,1.096,.48,.078,.17,.077,4,s*-.12);
      oval(0x404844,s*.14,1.10,.52,.040,.115,.028,4);
      paws(0x6c726c,s*.18,.275,.28,.25,.086,1);
      paws(0x69716c,s*.18,.24,-.48,.21,.094,2);
      oval(0x4c544e,s*.18,.92,.65,.037,.028,.028,4);
    }
    segment(0x565e59,[[0,.68,-.57],[0,.60,-.73],
      [0,.48,-.94],[0,.40,-1.00]],.069,3,12);
    eyes(.16,.97,.656,0xc5aa69);
  } else if(species==='Tiger') {
    // Bengal-type orange/ivory pelage; striped curved ribbons adhere to body.
    oval(0xc97830,0,.72,-.12,.32,.31,.73);
    oval(0xf5dfb4,0,.56,-.03,.255,.22,.57);
    oval(0xd38235,0,.89,.42,.31,.33,.26,4);
    oval(0xcf7b31,0,1.08,.60,.28,.24,.23,4);
    oval(0xf6e6cc,0,.98,.79,.195,.135,.22,4);
    oval(0x362923,0,1.025,.97,.086,.057,.056,4);
    for(const side of [-1,1]) {
      oval(0xc97c35,side*.197,1.31,.50,.098,.115,.072,4,side*.1);
      oval(0x302826,side*.197,1.33,.543,.054,.053,.018,4);
      paws(0xb97332,side*.234,.315,.35,.31,.112,1);
      paws(0xbf7935,side*.23,.30,-.56,.30,.127,2);
      oval(0xf5e7cc,side*.095,.95,.82,.112,.073,.114,4);
      oval(0x1b201c,side*.205,1.15,.78,.032,.023,.015,4);
      for(let k=-1;k<=1;k++)
        segment(0x30241e,[[side*.26,1.13+k*.05,.67],
          [side*.31,1.10+k*.055,.48],[side*.265,1.03+k*.06,.34]],.014,4,9);
      for(let i=0;i<12;i++) {
        const z=-.70+i*.104;
        const radial=Math.sqrt(Math.max(.05,1-Math.pow((z+.12)/.73,2)));
        const points: [number,number,number][]=[];
        for(let j=0;j<8;j++){
          const theta=j/7*1.40;
          points.push([side*.32*radial*Math.sin(theta),
            .72+.31*radial*Math.cos(theta)-.015,z+.025*Math.sin(theta+i)]);
        }
        segment(0x2d2520,points,.0115,0,10);
      }
      for(let k=-1;k<=1;k++) segment(0xe7d9bd,
        [[side*.12,1.02,.87],[side*.30,1.01+k*.025,.93],
          [side*.42,.98+k*.05,.90]],.0025,4,8);
    }
    eyes(.19,1.15,.804,0xc5a145);
    segment(0xc27a35,[[0,.79,-.71],[.05,.77,-.89],
      [.04,.69,-1.11],[0,.58,-1.36]],.071,3,18);
    for(let k=0;k<5;k++)
      oval(0x282421,0,.75-k*.034,-.82-k*.12,
        .074,.028,.020,3);
    oval(0x292724,0,.56,-1.37,.072,.052,.046,3);
  } else {
    // Fish have a tapered trunk, visible gills, caudal fork and dorsal fin.
    oval(0x729a93,0,.16,0,.077,.125,.32);
    oval(0xc6d9ba,0,.082,.03,.066,.056,.23);
    oval(0x537c76,0,.30,-.055,.020,.13,.145,5);
    for(const s of [-1,1]) {
      oval(0x507c72,s*.064,.16,-.29,.035,.11,.12,5,s*.2);
      oval(0x6b918a,s*.058,.11,.04,.021,.086,.10,5);
      segment(0x4e736c,[[s*.073,.09,.18],[s*.083,.20,.19]],.009,4,6);
    }
    oval(0x4e7972,0,.13,-.36,.11,.088,.095,5);
    oval(0x719b8e,0,.15,.31,.055,.062,.09,4);
    eyes(.046,.185,.24,0x171c19);
  }
  const merged=mergeGeometries(pieces);
  for(const part of pieces) part.dispose();
  if(!merged) throw new Error('Wildlife procedural body merge failed: '+species);
  merged.computeBoundingBox();
  merged.translate(0,-merged.boundingBox!.min.y,0);
  merged.computeVertexNormals();
  merged.computeBoundingBox();
  merged.computeBoundingSphere();
  merged.userData.wildlifeAuthoredVersion='E1';
  return merged;
}
