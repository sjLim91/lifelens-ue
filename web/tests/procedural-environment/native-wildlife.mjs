import assert from 'node:assert/strict';
import {readFileSync,existsSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve} from 'node:path';
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {source} from './compile.mjs';
const {NativeWildlifePool}=await source('render/native-wildlife.ts');
const {wildlifeRigClip,wildlifeVisualTravelMultiplier}=await source('render/wildlife-motion.ts');
assert.equal(wildlifeRigClip('Wolf','Flee',.15,1),'run');
assert.equal(wildlifeRigClip('Tiger','Flee',.15,1),'run');
assert.equal(wildlifeRigClip('Deer','Flee',.15,1),'run');
assert.equal(wildlifeRigClip('Hare','Flee',.15,1),'run');
assert.equal(wildlifeRigClip('Wolf','Hunt',.15,1),'hunt');
assert.equal(wildlifeRigClip('Wolf','Flee',0,1),'idle');
assert.equal(wildlifeRigClip('FreshwaterFish','Swim',0,1),'swim');
assert(wildlifeVisualTravelMultiplier('Flee')>wildlifeVisualTravelMultiplier('Hunt'));
assert(wildlifeVisualTravelMultiplier('Hunt')>wildlifeVisualTravelMultiplier('Roam'));
assert(wildlifeVisualTravelMultiplier('Forage')<wildlifeVisualTravelMultiplier('Roam'));


const {NATIVE_WILDLIFE_SOURCES:pins}=await source('render/native-wildlife-manifest.ts');
const root=resolve(import.meta.dirname,'../../..');
const output=resolve(import.meta.dirname,'review/native-wildlife');mkdirSync(output,{recursive:true});
const file=(species,tier)=>resolve(root,'docs/asset-reviews/native-wildlife/derived',pins[species].slug+(tier==='near'?'-rigged':'-'+tier)+'.glb');
async function load(species,tier='near'){
 const raw=readFileSync(file(species,tier));
 return new GLTFLoader().register(()=>({name:'LIFELENS_CONTRACT_TEST_TEXTURE_PLACEHOLDER',loadTexture:()=>Promise.resolve(new T.Texture())}))
 .parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');
}
const settle=()=>new Promise(resolve=>setTimeout(resolve,0));
const pose=(species,id=species)=>({id,species,sex:'Female',x:0,y:0,z:0,heading:0,scale:1,speed:0,phase:.31,behavior:'Roam'});
const records=[],contacts=[];
// Inspect actual original/deformed geometry, rather than trusting generator receipts.
for(const species of Object.keys(pins)){
 const gltf=await load(species),contract=gltf.parser.json.asset.extras.lifelensWildlife;
 const original=readFileSync(resolve(root,'docs/asset-reviews/native-wildlife',species,pins[species].uid,'source.glb'));
 const old=JSON.parse(original.subarray(20,20+original.readUInt32LE(12)));
 const nearRaw=readFileSync(file(species,'near'));
 const now=JSON.parse(nearRaw.subarray(20,20+nearRaw.readUInt32LE(12)));
 assert.deepEqual(now.meshes,old.meshes,'Original near topology/UV/skin accessor references must be retained');
 assert.deepEqual(now.skins,old.skins,'Original inverse binds and joint hierarchies must be retained');
 const sourceBinary=original.subarray(28+original.readUInt32LE(12)),nearBinary=nearRaw.subarray(28+nearRaw.readUInt32LE(12));
 const views=new Set(old.meshes.flatMap(m=>m.primitives.flatMap(p=>[p.indices,...Object.values(p.attributes)])).map(i=>old.accessors[i].bufferView));
 old.skins.forEach(s=>views.add(old.accessors[s.inverseBindMatrices].bufferView));
 for(const i of views){const a=old.bufferViews[i],b=now.bufferViews[i];assert.deepEqual(nearBinary.subarray(b.byteOffset??0,(b.byteOffset??0)+b.byteLength),sourceBinary.subarray(a.byteOffset??0,(a.byteOffset??0)+a.byteLength),'Texture resizing changed an original geometry/UV/weight/bind buffer');}
 assert.equal(contract.source_sha256,pins[species].sha256);
 const skins=[];gltf.scene.traverse(o=>{if(o.isSkinnedMesh)skins.push(o);});
 assert(skins.length&&skins.every(s=>s.geometry.attributes.skinWeight&&s.geometry.attributes.uv));
 const mixer=new T.AnimationMixer(gltf.scene);
 let worstLoop=0,maxHeight=0;
 for(const [name,c]of Object.entries(contract.clips)){
  const clip=gltf.animations.find(a=>a.name===c.name);assert(clip&&clip.tracks.length);
  mixer.stopAllAction();const action=mixer.clipAction(clip).reset().setLoop(T.LoopOnce,1);action.clampWhenFinished=true;action.play();
  const sample=time=>{
   mixer.setTime(time);gltf.scene.updateMatrixWorld(true);skins.forEach(s=>s.skeleton.update());
   return skins.flatMap(s=>{const result=[],count=s.geometry.attributes.position.count,step=Math.max(1,Math.floor(count/70));
    for(let i=0;i<count;i+=step){const p=new T.Vector3();s.getVertexPosition(i,p);p.applyMatrix4(s.matrixWorld);assert(p.toArray().every(Number.isFinite));maxHeight=Math.max(maxHeight,p.y);result.push(p);}return result;});
  };
  const start=sample(0),end=sample(c.duration);assert.equal(start.length,end.length);
  for(let i=0;i<start.length;i++)worstLoop=Math.max(worstLoop,start[i].distanceTo(end[i]));
 }
 assert(worstLoop<1e-4,species+' loop endpoint moved source vertices '+worstLoop);
 assert(maxHeight<(species==='Deer'?2.3:species==='Wolf'?1.6:species==='Hare'?1.1:.4),species+' did not normalize to metres');
 records.push({species,nativeBones:contract.bone_count,loopError:worstLoop,maxHeight});
 mixer.stopAllAction();mixer.uncacheRoot(gltf.scene);
}
console.log('PASS four original near surfaces/UVs/native rigs and actual deformed loop endpoints');

// All three real LOD files must pass the production loader and replace exactly one ID.
for(const tier of ['near','mid','far'])for(const species of Object.keys(pins)){
 assert(existsSync(file(species,tier)),'Missing actual '+species+' '+tier+' LOD');
 const gltf=await load(species,tier),parent=new T.Group();let calls=0;
 const pool=new NativeWildlifePool(async(s,t)=>{assert.equal(s,species);assert.equal(t,tier);calls++;return gltf;});
 const p=Object.freeze(pose(species));
 pool.begin(0);assert.equal(pool.draw(p,tier,0,()=>0),false);pool.end(parent);assert.equal(parent.children.length,0);
 await settle();pool.begin(0);assert.equal(pool.draw(p,tier,0,()=>0),true,species+' '+tier+' was rejected');pool.end(parent);
 assert.equal(calls,1);assert.equal(parent.children.length,1);
 if(tier!=='far'){
  const actor=pool.group.getObjectByName('LifeLensNativeAnimal:'+species);assert(actor);
  assert.equal(actor.userData.coreId,species);assert.deepEqual(actor.position.toArray(),[0,0,0]);
  const skinCount=[];actor.traverse(o=>{if(o.isSkinnedMesh)skinCount.push(o);});assert(skinCount.length);
  for(let i=0;i<15;i++){
   pool.begin(.05);const moved={...p,x:.007*i,z:-.003*i,speed:.15,behavior:'Roam'};
   assert(pool.draw(moved,tier,.05,(x,z)=>.03*x-.02*z));pool.end(parent);
   assert.deepEqual(actor.position.toArray(),[moved.x,0,moved.z],'Ground correction changed authoritative actor root');
  }
  if(species!=='FreshwaterFish'){
   const ground=(x,z)=>.03*x-.02*z;
   pool.begin(1);assert(pool.draw({...p,behavior:'Rest'},tier,1,ground));pool.end(parent);
   actor.updateMatrixWorld(true);
   const c=gltf.parser.json.asset.extras.lifelensWildlife,body=actor.getObjectByName(c.body),used=new Set(body.geometry.index.array);
   const errors=[];body.skeleton.update();
   for(const paw of c.paws){
    const family=new Set();body.skeleton.bones.find(b=>b.name===paw.name).traverse(b=>{if(b.isBone)family.add(body.skeleton.bones.indexOf(b));});
    const indices=paw.sole_vertices??[...used].filter(i=>{let weight=0;for(let k=0;k<4;k++)if(family.has(body.geometry.attributes.skinIndex.getComponent(i,k)))weight+=body.geometry.attributes.skinWeight.getComponent(i,k);return weight>.3;});
    assert(indices.length,'No rendered source foot vertices');
    let low=Infinity;for(const i of indices){if(!used.has(i))continue;const v=new T.Vector3();body.getVertexPosition(i,v);v.applyMatrix4(body.matrixWorld);low=Math.min(low,v.y-ground(v.x,v.z));}
    assert(Number.isFinite(low));errors.push(low);
   }
   const error=Math.max(...errors.map(Math.abs));contacts.push({species,tier,restSlopeSoleError:error,feet:errors});
   assert(error<.005,species+' '+tier+' actual deformed resting feet missed sloped ground by '+error);
  }
  if(species==='Deer'){
   const antlers=[];actor.traverse(o=>{if(o.isMesh&&(Array.isArray(o.material)?o.material:[o.material]).some(m=>m.name.includes('jiao_tex')))antlers.push(o);});
   assert(antlers.length&&antlers.every(o=>!o.visible),'Female deer retained source antlers');
   pool.begin(0);pool.draw({...p,sex:'Male'},tier,0,()=>0);pool.end(parent);assert(antlers.every(o=>o.visible));
  }
 }
 pool.begin(0);pool.end(parent);assert.equal(parent.children.length,0,'Removed Core ID retained an actor');pool.dispose();
}
console.log('PASS twelve actual assets, source validation, LODs, motion, female antlers and immutable roots on slopes');

{
 const parent=new T.Group(),species='Hare',gltf=await load(species),pool=new NativeWildlifePool(async()=>gltf);
 pool.begin(0);pool.draw(pose(species),'near',0);await settle();pool.begin(0);
 let accepted=0;for(let i=0;i<20;i++)if(pool.draw(pose(species,'bounded:'+i),'near',.01))accepted++;
 pool.end(parent);assert.equal(accepted,4,'Native skin budget grew without bound');pool.dispose();
}
{
 const gltf=await load('Wolf');gltf.parser.json.asset.extras.lifelensWildlife.source_sha256='unapproved';
 const pool=new NativeWildlifePool(async()=>gltf),parent=new T.Group();pool.begin(0);pool.draw(pose('Wolf'),'near',0);await settle();
 pool.begin(0);assert.equal(pool.draw(pose('Wolf'),'near',0),false);pool.end(parent);assert.equal(parent.children.length,0);pool.dispose();
}
{
 let finish;const late=new Promise(resolve=>finish=resolve),pool=new NativeWildlifePool(()=>late);
 pool.begin(0);pool.draw(pose('Hare'),'near',0);pool.dispose();
 const gltf=await load('Hare');let disposed=0;gltf.scene.traverse(o=>{if(o.isMesh)o.geometry.addEventListener('dispose',()=>disposed++);});
 finish(gltf);await settle();assert(disposed>0,'Post-disposal asset leaked GPU geometry');
}
console.log('PASS bounded native skins, rejected source fallback and late-load disposal');
{
 const parent=new T.Group(),pool=new NativeWildlifePool((s,t)=>load(s,t));
 for(const species of ['Hare','Wolf']){
  const p=pose(species,'same-observed-id');pool.begin(0);pool.draw(p,'near',0,()=>0);await settle();
  // GLTF parsing is async; complete the actual source load before asserting.
  let ok=false;for(let i=0;i<20;i++){pool.begin(0);ok=pool.draw(p,'near',0,()=>0);pool.end(parent);if(ok)break;await settle();}
  assert(ok);const actor=pool.group.getObjectByName('LifeLensNativeAnimal:'+p.id);assert.equal(actor.userData.species,species);
  const contractName=species==='Hare'?'LifeLensHareBody':'LifeLensWolfBody';assert(actor.getObjectByName(contractName),'Reused an actor from a different observed species');
 }
 pool.dispose();
}
console.log('PASS same-ID species replacement does not reuse a stale native body');
// Reproduce the seeded shallow river with production visibility and native dimensions.
{
 const {WildlifeLayer}=await source('render/wildlife-layer.ts');
 const {ResidentGroundSurface}=await source('render/resident-grounding.ts');
 const {buildFlowWaterSurfaceGeometry}=await source('render/water-geometry.ts');
 const {WORLD_GRID_CONTRACT:grid}=await source('runtime/lifelens-contract.ts');
 const terrain=JSON.parse(readFileSync(resolve(import.meta.dirname,'review/core-fixture.json'),'utf8')).terrain;
 const before=JSON.stringify(terrain),ground=new ResidentGroundSurface();ground.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);
 const layer=new WildlifeLayer(undefined,(s,t)=>load(s,t));layer.setTerrain(terrain);
 const eligible=new Set(layer.candidateCoreIds),fish=terrain.wildlife.animals.find(a=>a.species==='FreshwaterFish'&&eligible.has(a.id));assert(fish);
 const x=(fish.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
 const z=(fish.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
 const camera=new T.PerspectiveCamera(38,1,.01,100);camera.position.set(x,ground.height(x,z)+.2,z+.2);camera.lookAt(x,ground.height(x,z),z);
 let actor;for(let i=0;i<80;i++){layer.update(.02,camera);actor=layer.group.getObjectByName('LifeLensNativeAnimal:'+fish.id);if(actor)break;await settle();}
 assert(actor,'Native fish incorrectly hidden by oversized procedural fallback bounds');assert(layer.renderedCoreIds.has(fish.id));
 const water=new T.Mesh(buildFlowWaterSurfaceGeometry(terrain),new T.MeshBasicMaterial({side:T.DoubleSide}));water.updateMatrixWorld();
 const ray=new T.Raycaster(new T.Vector3(x,1000,z),new T.Vector3(0,-1,0)),surface=ray.intersectObject(water)[0]?.point.y;assert(Number.isFinite(surface));
 let low=Infinity,high=-Infinity;
 actor.updateMatrixWorld(true);actor.traverse(o=>{if(!o.isSkinnedMesh)return;o.skeleton.update();for(let i=0;i<o.geometry.attributes.position.count;i++){const p=new T.Vector3();o.getVertexPosition(i,p);p.applyMatrix4(o.matrixWorld);low=Math.min(low,p.y);high=Math.max(high,p.y);}});
 assert(low>ground.height(x,z)+.015,'Fish intersects the real river bed');assert(high<surface-.015,'Fish intersects the real water surface');
 assert.equal(JSON.stringify(terrain),before,'Visibility changed authoritative observations');
 water.geometry.dispose();water.material.dispose();layer.dispose();
 console.log('PASS native fish fits the unchanged shallow Core river between bed and water surface');
}
// Crowds retain approved native surfaces while obeying both skin budgets.
{
 const parent=new T.Group(),cache=new Map(),pool=new NativeWildlifePool((s,t)=>{const key=s+':'+t;if(!cache.has(key))cache.set(key,load(s,t));return cache.get(key);});
 let accepted=0;
 for(let frame=0;frame<100;frame++){
  pool.begin(.016);accepted=0;for(let i=0;i<20;i++)if(pool.draw(pose('Hare','crowd:'+i),'near',.016,()=>0))accepted++;pool.end(parent);
  if(accepted===20)break;await settle();
 }
 assert.equal(accepted,20,'Budget overflow lost native animals instead of using reduced source surfaces');
 const actors=pool.group.children.filter(o=>o.name.startsWith('LifeLensNativeAnimal:'));
 assert.equal(actors.filter(o=>o.userData.tier==='near').length,4);assert.equal(actors.filter(o=>o.userData.tier==='mid').length,8);
 const far=pool.group.getObjectByName('LifeLensNativeFar:Hare');assert.equal(far.userData.coreIds.length,8);
 assert.equal(new Set([...actors.map(o=>o.userData.coreId),...far.userData.coreIds]).size,20,'Overflow duplicated an observed ID');
 pool.begin(0);pool.draw(pose('Hare','crowd:19'),'near',0,()=>0);pool.end(parent);
 assert(pool.group.getObjectByName('LifeLensNativeAnimal:crowd:19'));assert(!pool.group.getObjectByName('LifeLensNativeFar:Hare'),'Obsolete crowd far instances remained');pool.dispose();
 console.log('PASS crowded native surfaces degrade to 4 near / 8 mid / static instances without dropping IDs');
}
// A visible subject must not lose its native skin to nearby offscreen animals.
{
 const {WildlifeLayer}=await source('render/wildlife-layer.ts');
 const {ResidentGroundSurface}=await source('render/resident-grounding.ts');
 const {WORLD_GRID_CONTRACT:grid}=await source('runtime/lifelens-contract.ts');
 const terrain=JSON.parse(readFileSync(resolve(import.meta.dirname,'review/core-fixture.json'),'utf8')).terrain;
 const before=JSON.stringify(terrain),layer=new WildlifeLayer(undefined,(s,t)=>load(s,t));layer.setTerrain(terrain);
 const candidates=new Set(layer.candidateCoreIds),subject=terrain.wildlife.animals.find(a=>a.species==='Deer'&&a.sex==='Male'&&candidates.has(a.id));assert(subject);
 const ground=new ResidentGroundSurface();ground.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);
 const x=(subject.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk,z=(subject.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
 const y=ground.height(x,z),camera=new T.PerspectiveCamera(38,960/720,.02,300);
 for(const [name,dx,dz] of [['quarter',.65,1],['front',0,1],['side',1,0],['back',0,-1]]){
  camera.position.set(x+dx*5.25,y+1.68,z+dz*5.25);camera.lookAt(x,y+.798,z);
  let actor;for(let i=0;i<80;i++){layer.update(.02,camera);actor=layer.group.getObjectByName('LifeLensNativeAnimal:'+subject.id);if(actor)break;await settle();}
  assert(actor,'Visible deer starved by other skins in '+name+' view');assert.equal(actor.userData.tier,'near');
  // The production Observer can refresh observations between render updates.
  // setTerrain renders without receiving a camera; it must retain this view's
  // allocation rather than spending the skin budget in observation order.
  for(let refresh=0;refresh<3;refresh++){
   layer.setTerrain(terrain);
   const refreshed=layer.group.getObjectByName('LifeLensNativeAnimal:'+subject.id);
   assert(refreshed,'Observation refresh evicted the visible deer in '+name+' view');
   assert.equal(refreshed.userData.tier,'near','Observation refresh changed the visible deer tier');
   assert(layer.renderedCoreIds.has(subject.id));
  }
  const rebased={...terrain,centerChunkX:terrain.centerChunkX+1,centerChunkY:terrain.centerChunkY-1};
  layer.setTerrain(rebased);
  const afterRebase=layer.group.getObjectByName('LifeLensNativeAnimal:'+subject.id);
  assert(afterRebase,'Observation origin rebase evicted the visible deer in '+name+' view');
  assert.equal(afterRebase.userData.tier,'near');
  const rebasedXZ=[x-grid.worldUnitsPerChunk,z+grid.worldUnitsPerChunk];
  assert.deepEqual([afterRebase.position.x,afterRebase.position.z],rebasedXZ);
  layer.setTerrain(terrain);
  const near=[];layer.group.traverse(o=>{if(o.name.startsWith('LifeLensNativeAnimal:')&&o.userData.tier==='near')near.push(o);});assert(near.length<=4);
 }
 assert.equal(JSON.stringify(terrain),before);layer.dispose();
 console.log('PASS visible Core deer keeps a bounded native skin in all four camera views');
}
writeFileSync(resolve(output,'contracts.json'),JSON.stringify({status:'PASS_SOURCE_SKIN_LOD_RUNTIME_CONTRACTS',records,contacts,physicalAndroid:'NOT_MEASURED'},null,2)+'\n');
