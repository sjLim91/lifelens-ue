import assert from 'node:assert/strict';
import fs from 'node:fs';
import { Buffer } from 'node:buffer';
import ts from 'typescript';

const source = fs.readFileSync(
  new URL('../../src/earth/natural-earth-land.ts', import.meta.url), 'utf8');
const js = ts.transpileModule(source, {
  compilerOptions: { module: ts.ModuleKind.ES2022, target: ts.ScriptTarget.ES2022 },
}).outputText;
const {unwrapCoastlineRing,landTexturePixel,paintNaturalEarthTexture} =
  await import(`data:text/javascript;base64,${Buffer.from(js).toString('base64')}`);

const land = JSON.parse(fs.readFileSync(
  new URL('../../public/earth/ne_110m_land.geojson',import.meta.url), 'utf8'));
assert.equal(land.type,'FeatureCollection');
assert(land.features.length>=100);
assert(land.features.every(x=>x.geometry.type==='Polygon'));

const seam=unwrapCoastlineRing([[179,1],[-179,2],[-178,3],[179,1]]);
assert.equal(seam.length,4);
assert.equal(seam[1].longitudeDeg,181);
assert.equal(seam[2].longitudeDeg,182);
assert.equal(seam[3].longitudeDeg,179);
const polar=landTexturePixel({longitudeDeg:180,latitudeDeg:90},720,360);
assert.deepEqual(polar,{x:720,y:0});
const calls=[];
const ctx={
  fillStyle:'',strokeStyle:'',lineWidth:0,
  fillRect(...args){calls.push(['rect',...args]);},
  beginPath(){calls.push(['begin']);},
  moveTo(...args){calls.push(['move',...args]);},
  lineTo(...args){calls.push(['line',...args]);},
  closePath(){calls.push(['close']);},
  fill(rule){calls.push(['fill',rule]);},
  stroke(){calls.push(['stroke']);},
};
paintNaturalEarthTexture(ctx,land,1024,512);
assert(calls.some(x=>x[0]==='fill' && x[1]==='evenodd'));
assert(calls.filter(x=>x[0]==='fill').length>=127);
assert(calls.filter(x=>x[0]==='line').length>5000);
assert.throws(()=>paintNaturalEarthTexture(ctx,{type:'Invalid',features:[]},1024,512));
console.log('Natural Earth globe coastline projection tests passed');
