import assert from 'node:assert/strict';
import fs from 'node:fs';
import { Buffer } from 'node:buffer';
import ts from 'typescript';

const source=fs.readFileSync(
  new URL('../../src/earth/core-earth-scenario.ts',import.meta.url),'utf8');
const module=ts.transpileModule(source,{
  compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022},
}).outputText;
const {readCoreEarthScenario,UNBOUND_EARTH_SCENARIO}=await import(
  `data:text/javascript;base64,${Buffer.from(module).toString('base64')}`);
const world={available:true,worldSeed:'4242001',generationVersion:3};
const valid={
  available:true,worldSeed:'4242001',generationVersion:3,
  regionId:'1276644333',latitudeDeg:35.1796,longitudeDeg:129.0756,
  originGridX:12,originGridY:-8,
  earthDataVersion:'ne110m-land-centers-1deg-v1',
  source:'CoreEarthScenario',singleOrigin:true,
  terrainGeographicallyConditioned:false,
  metricScaleProvisional:true,snapshotPersistence:false,
};
assert.deepEqual(readCoreEarthScenario(valid,world),valid);
const persisted={...valid,snapshotPersistence:true};
assert.deepEqual(readCoreEarthScenario(persisted,world),persisted);
for(const snapshotPersistence of [undefined,null,1,"false"]){
  assert.deepEqual(readCoreEarthScenario({...valid,snapshotPersistence},world),
    UNBOUND_EARTH_SCENARIO);
}
for(const change of [
  {worldSeed:'other'},{generationVersion:2},{regionId:'a12'},
  {latitudeDeg:91},{longitudeDeg:Infinity},{originGridX:1.2},
  {earthDataVersion:'../untrusted'},{source:'web-preview'},
  {terrainGeographicallyConditioned:true},{snapshotPersistence:'true'},
]){
  assert.deepEqual(readCoreEarthScenario({...valid,...change},world),
    UNBOUND_EARTH_SCENARIO);
}
assert.deepEqual(readCoreEarthScenario(null,world),UNBOUND_EARTH_SCENARIO);
assert.deepEqual(readCoreEarthScenario(valid,{...world,worldSeed:'999'}),
  UNBOUND_EARTH_SCENARIO);
console.log('Core-authorized Earth location contract passed');
