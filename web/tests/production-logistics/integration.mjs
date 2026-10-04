import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {source} from './compile.mjs';
const root=resolve(import.meta.dirname,'../..'),loader=new GLTFLoader(),assets=new Map();
for(const name of ['character','ual1','ual2']){const b=readFileSync(resolve(root,'public/vendor/characters/'+name+'.glb'));assets.set(name,await loader.parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),''));}
GLTFLoader.prototype.loadAsync=async url=>assets.get(url.includes('character.glb')?'character':url.includes('ual2')||url.includes('UAL2')?'ual2':'ual1');
const {ResidentWorldLayer}=await source('render/resident-world-layer.ts');
const {FacilityLayer}=await source('render/facility-layer.ts');
const l=new ResidentWorldLayer();for(let n=0;!l.ready&&n<300;n++)await new Promise(r=>setTimeout(r,5));assert(l.ready);
const terrain={available:true,worldSeed:'production-fixture',centerChunkX:0,centerChunkY:0,chunks:[{x:0,y:0,elevation01:0,waterKind:'None'}]};
const inventory=material=>[{item:'RawMaterial',material,quantity:3},{item:'SimpleContainer',material:'Clay',quantity:1}];
const r=(id,extra={})=>({id,name:id,alive:true,hasPosition:true,gridX:16,gridY:16,lifeStage:'Adult',ageYears:30,civilization:{inventory:inventory('Wood')},...extra});
const directive={active:true,kind:'Civilization',phase:'Moving',civilizationIntent:'Gather',facilityAction:'None',civilizationResourceNode:'wood',civilizationMaterial:'Wood',hasTargetGrid:true,targetGridX:16,targetGridY:16};
const world={available:true,facilities:[],storages:[],resources:[{id:'wood',gridX:17,gridY:16,hasAccessGrid:true,accessGridX:16,accessGridY:16,material:'Wood',quantity:10,maxQuantity:10}]};
l.setCivilization(world);
const identities=()=>[...l.actors].map(([id,a])=>[id,a.root,a.inventoryProps.root,a.inventoryProps.loadRoot]);
function resources(){const gs=new Set(),ms=new Set();let objects=0;for(const a of l.actors.values())a.root.traverse(o=>{objects++;if(o.geometry)gs.add(o.geometry);if(o.material)for(const m of Array.isArray(o.material)?o.material:[o.material])ms.add(m);});return{roots:l.actors.size,geometries:gs.size,materials:ms.size,objects};}
for(const count of [100,300]){
 const facts=Array.from({length:count},(_,i)=>r(String(i),{presentation:directive}));l.setResidents(facts,terrain,0,0);const roots=identities(),before=resources();
 for(let n=0;n<100;n++){l.setResidents(facts,terrain,0,0);l.update(.016);}assert.deepEqual(resources(),before);assert.deepEqual(identities(),roots);
 for(let n=0;n<20;n++){for(const fact of facts)fact.civilization.inventory=inventory(n%2?'CopperOre':'Wood');l.setResidents(facts,terrain,0,0);l.update(.016);}assert.deepEqual(identities(),roots);assert.equal(l.propResources.geometryCount,56);
 console.log('REAL MODEL BUDGET',JSON.stringify({residents:count,refreshes:100,...before,sharedPropGeometry:l.propResources.geometryCount}));
}
const working=r('worker',{presentation:{...directive,phase:'Interacting'}});l.setResidents([working],terrain,0,0);l.update(.02);const actor=l.actors.get('worker');assert.notEqual(l.restMotion(actor),'idle');
l.setCivilization({...world,resources:[]});assert.equal(l.restMotion(actor),'idle');assert.equal(l.interactionTargetYaw(actor),null);
l.setCivilization(world);l.setResidents([{...working,gridX:30}],terrain,0,0);for(let n=0;n<90;n++)l.update(.02);assert.equal(l.restMotion(actor),'idle');
l.setResidents([{...working,alive:false}],terrain,0,0);assert(!actor.root.visible);
l.setResidents([{...working,hasPosition:false}],terrain,0,0);assert(!actor.root.visible);
l.setResidents([working],terrain,1,0);l.setSimulationSpeed(0);const p=actor.current.clone(),time=actor.mixer.time;l.update(1);assert(actor.current.equals(p));assert.equal(actor.mixer.time,time);
l.setSimulationSpeed(1);l.dispose();l.dispose();assert.equal(l.actors.size,0);
const f=new FacilityLayer();
const facts=Array.from({length:150},(_,i)=>({id:'f'+i,kind:'Furnace',gridX:16,gridY:16,state:'Operational',active:true,workProgress:1,durability:1,fuelUnits:4,charcoalUnits:3,oreUnits:2,metalUnits:1,furnaceChargeMaterial:'CopperOre',furnaceOutputMaterial:'CopperMetal'}));
terrain.humanTraces={available:true,entries:facts.map(f=>({id:'facility:'+f.id,kind:'Facility',facilityKind:f.kind,gridX:f.gridX,gridY:f.gridY,state:f.state,progress01:1,active:true,lit:false,requiredMaterialUnits:8,deliveredMaterialUnits:8}))};
f.setCivilization({available:true,facilities:facts},terrain);assert(f.productionRocks.count+f.productionBars.count<=256);const structures=[...f.structures.values()].map(v=>v.group),rocks=f.productionRocks,bars=f.productionBars,material=f.productionMaterial;
for(let n=0;n<1000;n++)f.setCivilization({available:true,facilities:facts},terrain);
assert.deepEqual([...f.structures.values()].map(v=>v.group),structures);assert.equal(f.productionRocks,rocks);assert.equal(f.productionBars,bars);assert.equal(f.productionMaterial,material);
const owners=f.processingOwners.get(bars);assert(owners.length>0);assert.equal(f.pickTrace({intersectObjects:()=>[{object:bars,instanceId:0}]}),owners[0]);
const moved={...terrain,centerChunkX:1};f.setCivilization({available:true,facilities:facts},moved);const matrix=new THREE.Matrix4();rocks.getMatrixAt(0,matrix);assert(matrix.elements[12]<-7);
f.setCivilization({available:true,facilities:[]},{...terrain,humanTraces:{available:true,entries:[]}});assert.equal(rocks.count+bars.count,0);assert.equal(f.structures.size,0);assert.equal(f.processingOwners.size,0);
f.dispose();f.dispose();console.log('Facility processing pools, root continuity, selection, rebase, reset, double disposal PASS');
