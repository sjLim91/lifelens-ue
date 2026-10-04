import assert from 'node:assert/strict';
import * as THREE from 'three';
import { source } from './compile.mjs';
const { observedSettlements, observedTradeRoutes, selectedFrontier } = await source('render/settlement-presentation.ts');
const { SettlementFocusLayer } = await source('render/settlement-focus-layer.ts');
const { WORLD_PRESENTATION } = await source('render/world-presentation-config.ts');
const c = WORLD_PRESENTATION.settlement;
const settlement = (id='2', extra={}) => ({id,gridX:16,gridY:16,residentCount:4,facilityCount:3,
  operationalFacilityCount:2,plannedFacilityCount:1,storageSiteCount:1,active:true,established:true,...extra});
const route = (extra={}) => ({id:'2:4',firstSettlement:'2',secondSettlement:'4',firstGridX:16,firstGridY:16,
  secondGridX:45,secondGridY:16,partnerCount:1,exchangeCount:2,distanceGrid:29,active:true,...extra});
const payload = {available:true,settlements:[settlement(),settlement('4',{gridX:45})],tradeRoutes:[route()]};
const resident = {id:'r',alive:true,hasPosition:true,gridX:16,gridY:16,
  migration:{candidate:true,hasFrontierTarget:true,frontierGridX:200,frontierGridY:16}};
const terrain = {available:true,worldSeed:'1',centerChunkX:0,centerChunkY:0,chunks:[0,1].map(x=>({x,y:0,elevation01:.2}))};
let passed=0;
function test(name,run){run();passed++; console.log(`PASS ${name}`);}
test('missing/empty/unavailable settlement truth',()=>{
  for(const p of [{},{available:true},{available:true,settlements:[]},{...payload,available:false}])assert.deepEqual(observedSettlements(p),[]);
});
test('invalid/duplicate ID, coordinate, facts fail closed',()=>{
  for(const extra of [{id:undefined},{id:0},{id:''},{id:'0'},{gridX:NaN},{gridY:Infinity},{residentCount:-1},{facilityCount:'4'},{active:undefined}])
    assert.equal(observedSettlements({available:true,settlements:[settlement('2',extra)]}).length,0);
  assert.equal(observedSettlements({...payload,settlements:[settlement(),settlement()]}).length,0);
});
test('inactive and unestablished remain factual anchors',()=>assert.equal(observedSettlements({available:true,
  settlements:[settlement('2',{active:false,established:false})]}).length,1));
test('routes require actual endpoint IDs and finite coordinates',()=>{
  for(const extra of [{id:undefined},{firstSettlement:'missing'},{secondSettlement:'2'},{firstGridX:NaN},{secondGridY:Infinity},{partnerCount:-1},{exchangeCount:NaN}])
    assert.equal(observedTradeRoutes({...payload,tradeRoutes:[route(extra)]}).length,0);
  assert.equal(observedTradeRoutes({...payload,tradeRoutes:[route(),route()]}).length,0);
  assert.equal(observedTradeRoutes({...payload,tradeRoutes:undefined}).length,0);
});
test('zero evidence and inactive exact routes are retained faintly',()=>assert.equal(observedTradeRoutes({...payload,
  tradeRoutes:[route({active:false,partnerCount:0,exchangeCount:0})]}).length,1));
test('migration is not a move or inferred settlement',()=>{
  for(const r of [undefined,{...resident,migration:undefined},{...resident,alive:false},{...resident,hasPosition:false},
    {...resident,migration:{...resident.migration,candidate:false}},{...resident,migration:{...resident.migration,hasFrontierTarget:false}},
    {...resident,migration:{...resident.migration,frontierGridX:NaN}}]) assert.equal(selectedFrontier(r),null);
  assert.deepEqual(selectedFrontier(resident),{x:200,y:16});
});
test('1000 refreshes at 1/10/50/100 settlements and 1/20/100 routes keep fixed identities',()=>{
  for(const n of [1,10,50,100])for(const r of [1,20,100]) {
    const layer=new SettlementFocusLayer(),root=layer.group,children=[...root.children];
    const p={available:true,settlements:Array.from({length:n},(_,i)=>settlement(String(i+1),{gridX:6+i%12*2,gridY:6+Math.floor(i/12)*2})),
      tradeRoutes:n>1?Array.from({length:r},(_,i)=>route({id:String(i+1),firstSettlement:'1',secondSettlement:'2'})):[]};
    layer.setTargets(p,terrain);
    const attributes=children.map(x=>x.geometry.attributes.position.array);
    for(let i=0;i<1000;i++)layer.setTargets(structuredClone(p),terrain);
    assert.equal(layer.group,root);assert.deepEqual(layer.group.children,children);
    children.forEach((x,i)=>assert.equal(x.geometry.attributes.position.array,attributes[i]));
    assert.ok(layer.diagnostics.anchors<=c.maxVisibleAnchors);
    assert.ok(layer.diagnostics.routeSegments<=c.maxVisibleRoutes*c.routeSegments);
    const counts=[0,0,0];children.forEach((x,i)=>x.geometry.addEventListener('dispose',()=>counts[i]++));
    layer.dispose();layer.dispose();assert.deepEqual(counts,[1,1,1]);assert.equal(root.children.length,0);
  }
});
test('reset/new game clears old pick targets, selected halo, routes and direction',()=>{
  const layer=new SettlementFocusLayer();layer.setTargets(payload,terrain);layer.select('2');layer.setSelectedResident(resident);
  assert.ok(layer.diagnostics.guideSegments>0);layer.reset();
  assert.equal(layer.diagnostics.anchors,0);assert.equal(layer.diagnostics.routeSegments,0);assert.equal(layer.diagnostics.guideSegments,0);
  layer.setTargets({available:true,settlements:[]},terrain);assert.equal(layer.diagnostics.anchors,0);layer.dispose();
});
test('camera, static pause/speed and terrain rebase preserve Core coordinates',()=>{
  const before=JSON.stringify({payload,resident});const layer=new SettlementFocusLayer();layer.setTargets(payload,terrain);
  const root=layer.group,first=root.children[0].geometry.attributes.position.array[0];
  layer.setSelectedResident(resident);const camera=new THREE.PerspectiveCamera();camera.position.set(0,12,12);
  layer.update(camera,{x:0,y:1,z:0});const points=Array.from(root.children[2].geometry.attributes.position.array);
  for(let i=0;i<100;i++)layer.update(camera,{x:0,y:1,z:0});assert.deepEqual(Array.from(root.children[2].geometry.attributes.position.array),points);
  layer.setTargets(payload,{...terrain,centerChunkX:1});assert.equal(root.children[0].geometry.attributes.position.array[0],first-8);
  assert.equal(JSON.stringify({payload,resident}),before);layer.dispose();
});
test('bounded frontier, no target icon, missing terrain hides all overlays',()=>{
  const layer=new SettlementFocusLayer();layer.setTargets(payload,terrain);layer.setSelectedResident(resident);
  assert.ok(layer.diagnostics.guideSegments<=c.migrationSegments);assert.equal(layer.group.children.length,3);
  layer.setTargets(payload,{...terrain,available:false});assert.equal(layer.diagnostics.anchors,0);assert.equal(layer.diagnostics.routeSegments,0);
  assert.equal(layer.diagnostics.guideSegments,0);layer.dispose();
});
console.log(`Settlement/trade presentation: ${passed} tests PASS`);
