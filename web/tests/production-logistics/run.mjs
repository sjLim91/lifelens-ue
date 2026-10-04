import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import * as THREE from 'three';
import {source} from './compile.mjs';
const {residentCarriedMaterial:load,ResidentInventoryProps,ResidentPropResources}=await source('render/resident-props.ts');
const {ResidentProductionTargets}=await source('render/resident-production-context.ts');
const {PRODUCTION_MATERIALS,ownedMaterialUnits}=await source('render/production-material-presentation.ts');
const {facilityProductionStocks}=await source('render/facility-production-presentation.ts');
const {WORLD_GRID_CONTRACT:G}=await source('runtime/lifelens-contract.ts');
const {NaturalResourceProjection}=await source('render/natural-resource-projection.ts');
const p=(extra={})=>({active:true,kind:'Civilization',phase:'Moving',civilizationIntent:'Gather',civilizationMaterial:'Wood',...extra});
const inv=(material='Wood',quantity=2)=>[{item:'RawMaterial',material,quantity}];
let passed=0;function test(name,fn){fn();passed++;console.log('PASS',name);}
test('all actual Core MaterialKind families registered, unknown/zero/missing inventory fail closed',()=>{
 const header=readFileSync(new URL('../../../Source/LifeLensCore/include/lifelens/Civilization.h',import.meta.url),'utf8');
 const enums=header.match(/enum class MaterialKind\s*\{([\s\S]*?)\};/)[1].replace(/\/\/[^\n]*/g,'').split(',').map(v=>v.trim()).filter(v=>v&&v!=='Unknown');
 assert.deepEqual(Object.keys(PRODUCTION_MATERIALS).sort(),enums.sort());
 for(const material of enums) {const stacks=inv(material);if(material==='Water')stacks.push({item:'SimpleContainer',quantity:1});assert.equal(load(stacks,p({civilizationMaterial:material})),material);}
 for(const inventory of [undefined,[],inv('Wood',0),inv('Wood',NaN),inv('FutureMaterial',1)])assert.equal(load(inventory,p()),null);
 assert.equal(load(inv(),null),null);assert.equal(load(inv(),p({civilizationIntent:'FutureIntent'})),null);
 assert.equal(load(inv(),p({facilityAction:'FutureAction'})),null);assert.equal(load(inv(),p({kind:'FutureKind'})),null);
 assert.equal(load(inv('Water'),p({civilizationMaterial:'Water'})),null);
});
test('possession travels across actions; directive never creates future gather/retrieve/craft output',()=>{
 assert.equal(load(inv(),p()),'Wood');assert.equal(load([],p()),null);
 assert.equal(load(inv(),p({civilizationIntent:'Retrieve'})),'Wood');
 assert.equal(load([],p({civilizationIntent:'Craft',civilizationItem:'BronzeAxe'})),null);
 assert.equal(load(inv(),p({phase:'Interacting'})),null);
 assert.equal(load(inv(),p({phase:'Interacting',civilizationIntent:'Store'})),'Wood');
 assert.equal(load(inv(),p({phase:'Interacting',civilizationIntent:'Retrieve'})),null);
 assert.equal(ownedMaterialUnits([...inv('Wood',1),...inv('Wood',2)],'Wood'),3);
});
const index=new ResidentProductionTargets();
const resource={id:'tree',gridX:18,gridY:16,hasAccessGrid:true,accessGridX:16,accessGridY:16,material:'Wood',quantity:10,maxQuantity:20};
const facility={id:'project',kind:'Shelter',gridX:16,gridY:16,state:'UnderConstruction',active:false};
const storage={id:'store',gridX:16,gridY:16,totalUnits:2,inventory:inv()};
const interacting=p({phase:'Interacting',hasTargetGrid:true,targetGridX:16,targetGridY:16,civilizationResourceNode:'tree'});
const coord=(grid,center=0)=>(grid/G.gridCellsPerChunk-center-.5)*G.worldUnitsPerChunk;
test('exact node id/material/access, no remote gathering or proximity inference',()=>{
 index.setSnapshot({available:true,resources:[resource]});assert(index.interacting(interacting,coord(16),coord(16),0,0));
 assert(!index.interacting(interacting,coord(30),coord(16),0,0));
 for(const extra of [{civilizationResourceNode:'missing'},{civilizationMaterial:'Stone'},{active:false},{hasTargetGrid:false},{targetGridX:0}])assert.equal(index.resolve({...interacting,...extra}),null);
 index.setSnapshot({available:true,resources:[{...resource,quantity:0}]});assert.equal(index.resolve(interacting),null);
 index.setSnapshot({available:true,resources:[{...resource,gridX:500}]});assert(!index.interacting(interacting,coord(16),coord(16),0,0));
});
test('storage and project are exact factual targets; unknown/missing/mismatched facilities fail closed',()=>{
 index.setSnapshot({available:true,storages:[storage],facilities:[facility]});
 const store={...interacting,civilizationIntent:'Store',civilizationStorage:'store'};assert(index.interacting(store,coord(16),coord(16),0,0));
 assert.equal(index.resolve({...store,civilizationStorage:'missing'}),null);
 const work={...interacting,civilizationIntent:'Craft',facilityAction:'Work',facilityKind:'Shelter',facilityId:'project'};
 assert(index.resolve(work));assert.equal(index.resolve({...work,facilityId:'other'}),null);assert.equal(index.resolve({...work,facilityKind:'Furnace'}),null);
 assert(index.interacting(work,coord(16,1),coord(16,-1),1,-1));
 assert(!index.interacting(work,coord(16),coord(16),1,-1));
 index.clear();assert.equal(index.resolve(work),null);
});
test('cultivation and processing only exact active operational facility; no inferred use',()=>{
 index.setSnapshot({available:true,facilities:[{...facility,id:'plot',kind:'CultivatedPlot',state:'Operational',active:true},{...facility,id:'furnace',kind:'Furnace',state:'Operational',active:true}]});
 for(const action of ['Plant','Water','Tend','Harvest'])assert(index.resolve({...interacting,civilizationIntent:'Craft',facilityKind:'CultivatedPlot',facilityId:'plot',facilityAction:action}));
 assert.equal(index.resolve({...interacting,civilizationIntent:'Craft',facilityKind:'CultivatedPlot',facilityId:'missing',facilityAction:'Water'}),null);
 assert(index.resolve({...interacting,civilizationIntent:'Craft',facilityKind:'Furnace',facilityId:'furnace',facilityAction:'LoadSmeltCharge'}));
 assert.equal(index.resolve({...interacting,civilizationIntent:'Craft',facilityKind:'CultivatedPlot',facilityId:'plot',facilityAction:'LoadSmeltCharge'}),null);
});
test('processing buffers consume only actual amounts/types and bounded visual bands',()=>{
 const f={...facility,kind:'Furnace',state:'Operational',fuelUnits:4,charcoalUnits:2,oreUnits:3,metalUnits:1,furnaceChargeMaterial:'CopperOre',furnaceOutputMaterial:'CopperMetal'};
 assert.equal(facilityProductionStocks(f).length,4);
 assert.deepEqual(facilityProductionStocks({...f,fuelUnits:0,charcoalUnits:0,oreUnits:0,metalUnits:0}),[]);
 assert.equal(facilityProductionStocks({...f,furnaceOutputMaterial:'Unknown'}).length,3);
 assert.deepEqual(facilityProductionStocks({...f,state:'Ruined'}),[]);
 assert(facilityProductionStocks({...f,metalUnits:1000000}).every(s=>s.fill<=1));
});
test('existing resource projection reflects Core depletion only, without a Web regrowth clock',()=>{
 const terrain={available:true,chunks:[{x:0,y:0}]};const a=new NaturalResourceProjection({available:true,resources:[resource]},terrain);
 const b=new NaturalResourceProjection({available:true,resources:[{...resource,quantity:0}]},terrain);
 assert.notEqual(a.treeSignature,b.treeSignature);assert.equal(resource.quantity,10);
 assert.equal(new NaturalResourceProjection({available:true,resources:[resource]},terrain).signature,a.signature);
});
test('actual IronOre uses the existing rock projection and disappears only at factual depletion',()=>{
 const terrain={available:true,centerChunkX:0,centerChunkY:0,chunks:[{x:0,y:0}]};
 const node={id:'iron',material:'IronOre',gridX:16,gridY:16,quantity:8,maxQuantity:8};
 const before=new NaturalResourceProjection({available:true,resources:[node]},terrain);
 const after=new NaturalResourceProjection({available:true,resources:[{...node,quantity:0}]},terrain);
 assert(before.candidates('rocks',0,0,'fixture',[],8).some(v=>v.material==='IronOre'));
 assert.equal(after.candidates('rocks',0,0,'fixture',[],8).length,0);
});
test('fixed prop slots + shared catalog keep 100/300 residents identities stable across 100 refreshes',()=>{
 const resources=new ResidentPropResources();const actors=[];
 for(let n=0;n<300;n++) {
  const visual=new THREE.Group(),model=new THREE.Group();for(const name of ['pelvis','hand_l','hand_r']){const b=new THREE.Bone();b.name=name;model.add(b);}visual.add(model);
  actors.push(new ResidentInventoryProps(visual,model,resources));
 }
 const meshes=actors.map(a=>[...a.root.children,a.load]);const geometryCount=resources.geometryCount;
 for(const count of [100,300])for(let n=0;n<100;n++)for(const a of actors.slice(0,count)) {
  const material=Object.keys(PRODUCTION_MATERIALS)[n%16];const inventory=[...inv(material,n%3+1),{item:'DiggingStick',quantity:1},{item:'SimpleContainer',quantity:1}];
  a.setInventory(inventory,p({civilizationMaterial:material}));a.setVisuallyMoving(true);a.update();
 }
 assert.equal(resources.geometryCount,geometryCount);assert.equal(geometryCount,56);
 for(let i=0;i<actors.length;i++){assert.deepEqual([...actors[i].root.children,actors[i].load],meshes[i]);actors[i].reset();assert(!actors[i].hasCarriedLoad);assert(!actors[i].root.visible);assert(!actors[i].loadRoot.visible);actors[i].dispose();actors[i].dispose();}
 let gd=0,md=0;resources.get('Wood').addEventListener('dispose',()=>gd++);resources.material.addEventListener('dispose',()=>md++);resources.dispose();resources.dispose();assert.equal(gd,1);assert.equal(md,1);
 console.log('PROP BUDGET',JSON.stringify({residents:300,meshesPerResident:4,sharedGeometries:geometryCount,sharedMaterials:1,refreshes:100}));
});
test('unmatched interaction removes hand props; consumption hides slots without destroying pool',()=>{
 const visual=new THREE.Group(),model=new THREE.Group();for(const name of ['pelvis','hand_l','hand_r']){const b=new THREE.Bone();b.name=name;model.add(b);}visual.add(model);
 const a=new ResidentInventoryProps(visual,model);a.setInventory(inv(),p({phase:'Interacting',facilityAction:'DeliverMaterial'}));assert(a.hasCarriedLoad);
 a.setInteractionAllowed(false);assert(!a.hasCarriedLoad);a.setInventory([]);assert(!a.root.visible);assert(!a.loadRoot.visible);a.dispose();a.dispose();
});
console.log(`Production logistics: ${passed} tests passed`);
