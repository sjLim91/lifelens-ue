import assert from 'node:assert/strict';
import {readFileSync,existsSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve} from 'node:path';
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {source} from './compile.mjs';
const {NativeWildlifePool}=await source('render/native-wildlife.ts');
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
const records=[];
// Inspect actual original/deformed geometry, rather than trusting generator receipts.
for(const species of Object.keys(pins)){
 const gltf=await load(species),contract=gltf.parser.json.asset.extras.lifelensWildlife;
 const original=readFileSync(resolve(root,'docs/asset-reviews/native-wildlife',species,pins[species].uid,'source.glb'));
 const old=JSON.parse(original.subarray(20,20+original.readUInt32LE(12)));
 const nearRaw=readFileSync(file(species,'near'));
 const now=JSON.parse(nearRaw.subarray(20,20+nearRaw.readUInt32LE(12)));
 assert.deepEqual(now.meshes,old.meshes,'Original near topology/UV/skin accessor references must be retained');
 assert.deepEqual(now.skins,old.skins,'Original inverse binds and joint hierarchies must be retained');
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
writeFileSync(resolve(output,'contracts.json'),JSON.stringify({status:'PASS_SOURCE_SKIN_LOD_RUNTIME_CONTRACTS',records,physicalAndroid:'NOT_MEASURED'},null,2)+'\n');
