import assert from 'node:assert/strict';
import * as THREE from 'three';
import {source} from './compile.mjs';
const H=await source('render/facility-construction-presentation.ts');
const {FacilityLayer}=await source('render/facility-layer.ts');
const {WORLD_PRESENTATION:C}=await source('render/world-presentation-config.ts');
export const kinds=['PrimitiveStorage','FirePit','WorkSurface','SleepingPlace','Shelter','Furnace','CultivatedPlot','FutureFacility'];
export const trace=(extra={})=>({id:'facility:1',kind:'Facility',facilityKind:'Shelter',gridX:16,gridY:16,state:'UnderConstruction',progress01:0,deliveredMaterialUnits:0,requiredMaterialUnits:10,active:false,lit:false,...extra});
export const detail=(extra={})=>({id:'1',kind:'Shelter',gridX:16,gridY:16,state:'UnderConstruction',constructionWork:0,workProgress:0,requiredMaterialUnits:10,deliveredMaterialUnits:0,durability:1,requirements:[],...extra});
export const terrain=(entries=[])=>({available:true,worldSeed:'fixture',centerChunkX:0,centerChunkY:0,chunks:[{x:0,y:0,elevation01:0,waterKind:'None'}],humanTraces:{available:true,entries}});
const resident=(extra={})=>({id:'r',alive:true,hasPosition:true,presentation:{active:true,kind:'Civilization',phase:'Interacting',facilityAction:'Repair',facilityId:'1',...extra}});
let passed=0;function test(name,fn){fn();console.log('PASS',name);passed++;}
test('all facility kinds and unknown fallback derive only actual phases',()=>{for(const facilityKind of kinds){
 assert.equal(H.deriveFacilityVisualState(trace({facilityKind,state:'Planned'})).phase,'Planned');
 assert.equal(H.deriveFacilityVisualState(trace({facilityKind,deliveredMaterialUnits:3})).phase,'MaterialsArriving');
 assert.equal(H.deriveFacilityVisualState(trace({facilityKind,deliveredMaterialUnits:10})).phase,'ReadyForWork');
 for(const [p,phase] of [[.15,'EarlyConstruction'],[.45,'Framing'],[.75,'Finishing'],[.95,'Finishing']]) assert.equal(H.deriveFacilityVisualState(trace({facilityKind,progress01:p})).phase,phase);
 assert.equal(H.deriveFacilityVisualState(trace({facilityKind,state:'Operational'})).phase,'Operational');
 assert.equal(H.deriveFacilityVisualState(trace({facilityKind,state:'Ruined'})).phase,'Ruined');
}});
test('actual delivered volume, exact requirements and stale detail safe generic fallback',()=>{
 assert.deepEqual(H.constructionMaterialPiles(trace()),[]);
 for(const n of [0.01,3,10]) assert(Math.abs(H.constructionMaterialPiles(trace({deliveredMaterialUnits:n})).reduce((s,p)=>s+p.fill/4,0)-n/10)<1e-8);
 const t=trace({deliveredMaterialUnits:3}),d=detail({deliveredMaterialUnits:3,requirements:[{material:'Stone',required:10,delivered:3}]});
 assert(H.constructionMaterialPiles(t,d).every(p=>p.material==='Stone'));
 assert(H.constructionMaterialPiles(t,detail()).every(p=>p.material==='Unknown'));
 assert.equal(H.deriveFacilityVisualState(trace({progress01:.75}),detail({workProgress:.1})).workProgress,.75);
});
test('durability bands require exact matching current detail; inactivity no evidence',()=>{
 for(const [durability,band] of [[1,0],[.55,1],[.25,2],[.1,3]]) assert.equal(H.deriveFacilityVisualState(trace({state:'Operational'}),detail({state:'Operational',durability})).durabilityBand,band);
 assert.equal(H.deriveFacilityVisualState(trace({state:'Operational'}),detail({state:'Ruined',durability:0})).durabilityBand,0);
 assert.equal(H.deriveFacilityVisualState(trace({state:'Operational'}),detail({id:'wrong',durability:0})).durabilityBand,0);
});
test('reveal monotonic bounded and bucket stable',()=>{
 let old=0;for(let p=0;p<=1;p+=.001){const n=H.partReveal(p,.2,.4);assert(n>=old&&n<=1);old=n;}
 assert.equal(H.deriveFacilityVisualState(trace({progress01:.531})).visualProgress,H.deriveFacilityVisualState(trace({progress01:.532})).visualProgress);
});
test('Repair exact site and current directive, wrong id, dead, moving, proximity no authority',()=>{
 assert.equal(H.facilityActivitySites([resident()]).get('facility:1'),'Repair');
 assert.equal(H.facilityActivitySites([resident({facilityAction:'Work'}),resident({facilityAction:'DeliverMaterial'})]).get('facility:1'),'Work');
 for(const p of [{facilityId:'2'},{active:false},{phase:'Moving'},{kind:'Social'},{facilityAction:'None'}]) assert.equal(H.facilityActivitySites([resident(p)]).get('facility:1'),undefined);
 assert.equal(H.facilityActivitySites([{...resident(),alive:false}]).size,0);
 assert.equal(H.facilityActivitySites([{...resident(),hasPosition:false}]).size,0);
 assert.equal(H.facilityActivitySites([{id:'near',hasPosition:true}]).size,0);
});
test('unstarted real planned DTO site only, trace priority, outside window skipped',()=>{
 const w=terrain();const d=detail({state:'Planned',kind:'SleepingPlace'});
 assert.equal(H.facilityPresentationTraces(w,{available:true,facilities:[d]}).length,1);
 assert.equal(H.facilityPresentationTraces(w,{available:true,facilities:Array.from({length:150},(_,i)=>({...d,id:String(i+1)}))}).length,64);
 assert.equal(H.facilityPresentationTraces(w,{available:false,facilities:[d]}).length,0);
 assert.equal(H.facilityPresentationTraces(w,{available:true,facilities:[{...d,gridX:999}]}).length,0);
 assert.equal(H.facilityPresentationTraces(terrain([trace({progress01:.5})]),{available:true,facilities:[d]}).length,1);
});
test('kind geometry progress material and ruin continuity preserve site identity',()=>{for(const facilityKind of kinds){
 const l=new FacilityLayer();let root;
 for(const [state,progress01,deliveredMaterialUnits] of [['Planned',0,0],['Planned',0,3],['UnderConstruction',0,10],['UnderConstruction',.15,10],['UnderConstruction',.45,10],['UnderConstruction',.95,10],['Operational',1,10],['Ruined',1,10],['Operational',1,10]]){
  const t=trace({facilityKind,state,progress01,deliveredMaterialUnits});l.setCivilization({available:true,facilities:[detail({kind:facilityKind,state,deliveredMaterialUnits})]},terrain([t]));
  const g=l.structures.get(t.id).group;if(root)assert.equal(g,root);root=g;assert(g.children.length>0);
 }
 l.dispose();
}});
test('150 facilities / 1000 refreshes no unchanged rebuild; bounded shared resources and activity',()=>{
 const entries=Array.from({length:150},(_,i)=>trace({id:`facility:${i+1}`,facilityKind:kinds[i%7],gridX:i%16,gridY:Math.floor(i/16),state:'Operational',progress01:1,deliveredMaterialUnits:10}));
 const w=terrain(entries),ds=entries.map(t=>detail({id:t.id.slice(9),kind:t.facilityKind,gridX:t.gridX,gridY:t.gridY,state:'Operational',durability:.4}));
 const l=new FacilityLayer();l.setCivilization({available:true,facilities:ds},w);
 let builds=0;const build=l.buildFacility.bind(l);l.buildFacility=(...args)=>{builds++;return build(...args)};
 const roots=[...l.structures.values()].map(e=>e.group);const count=l.group.children.length;const materials=l.wornMaterials.size;
 for(let i=0;i<1000;i++) {l.setCivilization({available:true,facilities:ds.map(d=>({...d,durability:.4+i%2*.001}))},w);l.setResidents(Array.from({length:20},(_,n)=>resident({facilityId:String(n+1)})));l.update(.016);assert(l.activityGeometry.drawRange.count<=C.construction.maxActiveSites*4);}
 assert.equal(builds,0);assert.equal(l.group.children.length,count+1);assert.equal(l.wornMaterials.size,materials);assert.deepEqual([...l.structures.values()].map(e=>e.group),roots);
 let gd=0,md=0;l.activityGeometry.addEventListener('dispose',()=>gd++);l.activityMaterial.addEventListener('dispose',()=>md++);
 l.setResidents([]);l.update(.016);assert.equal(l.activityGeometry.drawRange.count,0);
 l.setCivilization({available:true,facilities:[]},terrain());assert.equal(l.structures.size,0);l.dispose();assert.equal(gd,1);assert.equal(md,1);
 console.log('RESOURCE',JSON.stringify({inputFacilities:150,visibleFacilities:roots.length,unchangedRebuilds:builds,wornMaterials:materials,maxActivityDrawCalls:1}));
});
test('partial upright construction is grounded and Operational bed preserves support',()=>{
 const l=new FacilityLayer(),t=trace({facilityKind:'PrimitiveStorage'}),g=new THREE.Group();
 l.addCylinderAtProgress(g,t,.15,.06,l.woodMaterial,[0,.62,0],[.11,1.24,.11],[0,0,0],20);
 assert(Math.abs(g.children[0].position.y-g.children[0].scale.y/2)<1e-8);
 const bed=new THREE.Group();l.buildSleepingPlace(bed,trace({facilityKind:'SleepingPlace',state:'Operational'}),1);
 assert.equal(bed.children.length,3);assert.equal(bed.children.reduce((n,b)=>n+b.count,0),7);
 const matrix=new THREE.Matrix4();const mat=bed.children.find(b=>b.material===l.beddingMaterial);mat.getMatrixAt(0,matrix);
 const position=new THREE.Vector3(),scale=new THREE.Vector3(),rotation=new THREE.Quaternion();matrix.decompose(position,rotation,scale);
 assert(Math.abs(position.y+scale.y/2+.025-.14)<1e-6);l.disposeStructureInstances(bed);l.dispose();
});
test('repair does not restore geometry until Core condition changes, site and disposal stable',()=>{
 const l=new FacilityLayer(),t=trace({state:'Operational',progress01:1,deliveredMaterialUnits:10}),w=terrain([t]);
 l.setCivilization({available:true,facilities:[detail({state:'Operational',durability:.25})]},w);
 const root=l.structures.get(t.id).group,child=root.children[0],condition=child.material;
 l.setResidents([resident()]);l.update(.1);assert.equal(root.children[0],child);assert.equal(child.material,condition);
 l.setCivilization({available:true,facilities:[detail({state:'Operational',durability:1})]},w);
 assert.equal(l.structures.get(t.id).group,root);assert.notEqual(root.children[0].material,condition);
 l.setSimulationSpeed(0);const time=l.activityTime;l.update(1);assert.equal(l.activityTime,time);
 const mats=new Set(),geos=new Set();root.traverse(o=>{if(o.geometry)geos.add(o.geometry);if(o.material)mats.add(o.material)});
 for(let n=0;n<100;n++)l.setTerrain(w);
 const afterMats=new Set(),afterGeos=new Set();root.traverse(o=>{if(o.geometry)afterGeos.add(o.geometry);if(o.material)afterMats.add(o.material)});
 assert.deepEqual(afterMats,mats);assert.deepEqual(afterGeos,geos);l.dispose();
});
console.log(`Facility construction: ${passed} tests passed`);
