import assert from 'node:assert/strict';
import fs from 'node:fs';
import { Buffer } from 'node:buffer';
import ts from 'typescript';

const source=fs.readFileSync(
  new URL('../../src/earth/core-earth-solar.ts',import.meta.url),'utf8');
const module=ts.transpileModule(source,{
  compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022},
}).outputText;
const {readCoreEarthSolar,UNBOUND_EARTH_SOLAR,formatSolarHour}=await import(
  `data:text/javascript;base64,${Buffer.from(module).toString('base64')}`);
const world={available:true,worldSeed:'4242001',generationVersion:3,minute:442};
const scenario={
  available:true,worldSeed:'4242001',generationVersion:3,
  regionId:'1276644333',latitudeDeg:35.1796,longitudeDeg:129.0756,
};
const valid={
  available:true,source:'CoreEarthSolar',
  worldSeed:'4242001',generationVersion:3,
  regionId:'1276644333',minute:442,
  latitudeDeg:35.1796,longitudeDeg:129.0756,
  localSolarHour:16.15,solarElevationDeg:32.1,
  daylightHours:12.1,sunlightAboveHorizon:true,
  sunshinePotential01:0.532,regionalWeatherApplied:false,
};
assert.deepEqual(readCoreEarthSolar(valid,world,scenario),valid);
assert.equal(formatSolarHour(16.15),'16:09');
assert.equal(formatSolarHour(-1),'—');
assert.equal(formatSolarHour(24),'—');
for (const change of [
  {worldSeed:'spoofed'},{generationVersion:4},{regionId:'111'},
  {minute:443},{latitudeDeg:34},{longitudeDeg:0},
  {source:'WebSolar'},{regionalWeatherApplied:true},
  {localSolarHour:-0.01},{localSolarHour:24},
  {daylightHours:25},{solarElevationDeg:91},
  {sunshinePotential01:1.01},{sunlightAboveHorizon:'true'},
]) {
  assert.deepEqual(readCoreEarthSolar({...valid,...change},world,scenario),
    UNBOUND_EARTH_SOLAR);
}
assert.deepEqual(readCoreEarthSolar(valid,world,{available:false}),
  UNBOUND_EARTH_SOLAR);
assert.deepEqual(readCoreEarthSolar(null,world,scenario),
  UNBOUND_EARTH_SOLAR);
assert.deepEqual(readCoreEarthSolar(valid,{...world,minute:443},scenario),
  UNBOUND_EARTH_SOLAR);
console.log('Earth solar read-only Core observation contract passed');
