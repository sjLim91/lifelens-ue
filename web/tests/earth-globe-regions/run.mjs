import assert from 'node:assert/strict';
import fs from 'node:fs';
import { Buffer } from 'node:buffer';
import ts from 'typescript';

const source = fs.readFileSync(
  new URL('../../src/earth/globe-region-picking.ts', import.meta.url),
  'utf8',
);
const output = ts.transpileModule(source, {
  compilerOptions: {
    module: ts.ModuleKind.ES2022,
    target: ts.ScriptTarget.ES2022,
  },
}).outputText;
const {geoFromGlobeUV,globeSurfaceUnitVector,formatPreviewGeoPoint} = await import(
  `data:text/javascript;base64,${Buffer.from(output).toString('base64')}`,
);
assert.deepEqual(geoFromGlobeUV(0.5,0.5),{
  latitudeDeg:0,longitudeDeg:0,
});
assert.deepEqual(geoFromGlobeUV(0,0),{
  latitudeDeg:-90,longitudeDeg:-180,
});
assert.deepEqual(geoFromGlobeUV(1,1),{
  latitudeDeg:90,longitudeDeg:-180,
});
const position=geoFromGlobeUV((129.0756+180)/360,(35.1796+90)/180);
assert(Math.abs(position.latitudeDeg-35.1796)<1e-9);
assert(Math.abs(position.longitudeDeg-129.0756)<1e-9);
assert.equal(formatPreviewGeoPoint(position),'35.180°N, 129.076°E');
assert.equal(formatPreviewGeoPoint({latitudeDeg:-33.9,longitudeDeg:18.4}),
  '33.900°S, 18.400°E');
for(const [u,v] of [
  [-0.01,0],[1.01,0.5],[0.5,-0.01],[0.5,1.01],
  [NaN,0.5],[0,Infinity],
]) assert.equal(geoFromGlobeUV(u,v),null);
for (const [lat,lon,want] of [
  [0,-180,[-1,0,0]],
  [0,0,[1,0,0]],
  [0,90,[0,0,-1]],
  [90,0,[0,1,0]],
  [-90,0,[0,-1,0]],
]) {
  const got=globeSurfaceUnitVector({latitudeDeg:lat,longitudeDeg:lon});
  assert(got);
  for(const [idx,value] of [got.x,got.y,got.z].entries()){
    assert(Math.abs(value-want[idx])<1e-10);
  }
}
assert.equal(globeSurfaceUnitVector({latitudeDeg:99,longitudeDeg:0}),null);
assert.equal(globeSurfaceUnitVector({latitudeDeg:0,longitudeDeg:NaN}),null);

console.log('Globe A/B coordinate selection contract passed');
