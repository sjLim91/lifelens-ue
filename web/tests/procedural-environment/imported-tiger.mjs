import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync,existsSync} from 'node:fs';
import {resolve} from 'node:path';
import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {source} from './compile.mjs';

// Node checks skin/animation data, not texture appearance or browser quality.
const {ImportedTigerPool}=await source('render/imported-tiger.ts');
const directory=process.env.LIFELENS_TIGER_TEST_DIR
  ??resolve(import.meta.dirname,'../../public/vendor/wildlife');
const inspection=Boolean(process.env.LIFELENS_TIGER_TEST_DIR);
const assets=new Map();
for(const [tier,name] of [['near','tiger-rigged.glb'],['mid','tiger-mid.glb']]){
  const buffer=readFileSync(resolve(directory,name));
  const loader=new GLTFLoader().register(()=>({name:'LIFELENS_NODE_TEXTURE_TEST',
    loadTexture:()=>Promise.resolve(new THREE.Texture())}));
  const gltf=await loader.parseAsync(buffer.buffer.slice(buffer.byteOffset,buffer.byteOffset+buffer.byteLength),'');
  assets.set(tier,gltf);
  assert.equal(gltf.animations.length,7);
  let skins=0;
  gltf.scene.traverse(object=>{if(object instanceof THREE.SkinnedMesh){
    skins++;assert.equal(object.skeleton.bones.length,24);
    const weights=object.geometry.getAttribute('skinWeight');
    for(let i=0;i<weights.count;i++)assert(Math.abs(weights.getX(i)+weights.getY(i)+weights.getZ(i)+weights.getW(i)-1)<1e-5);
  }});
  assert.equal(skins,1);
}
const pool=new ImportedTigerPool(async tier=>assets.get(tier));
const pose={id:'core-tiger-17',x:0,y:0,z:0,heading:0,scale:1,speed:0,phase:0,behavior:'Rest'};
const parent=new THREE.Group();
pool.begin();assert.equal(pool.draw(pose,'near',0),false,'Loading must keep fallback');pool.end(parent);
await new Promise(resolve=>setImmediate(resolve));
pool.begin();assert.equal(pool.draw(pose,'near',0),true);pool.end(parent);
const actor=pool.group.children[0];assert.equal(actor.userData.coreId,pose.id);
const initial=new THREE.Vector3();let skin;
actor.traverse(object=>{if(object instanceof THREE.SkinnedMesh)skin=object;});
actor.updateMatrixWorld(true);skin.skeleton.update();skin.getVertexPosition(0,initial);
let maximumDeformation=0;
for(let i=0;i<36;i++){
  pool.begin();assert(pool.draw({...pose,behavior:'Roam',speed:.7096774193548387/1.2,z:i*.02},'near',1.2/36));pool.end(parent);
  parent.updateMatrixWorld(true);skin.skeleton.update();
  const current=new THREE.Vector3();skin.getVertexPosition(0,current);
  maximumDeformation=Math.max(maximumDeformation,current.distanceTo(initial));
  assert.equal(actor.userData.clip,'walk');
  assert.equal(actor.position.z,i*.02,'Actor root must follow supplied Core presentation position');
  assert(actor.position.toArray().every(Number.isFinite));
  skin.skeleton.bones.forEach(bone=>assert(bone.matrixWorld.elements.every(Number.isFinite)));
}
assert(maximumDeformation>.01,'Mixer must deform the actual imported source skin');
pool.begin();assert(pool.draw(pose,'near',0));pool.end(parent);
assert.equal(pool.group.children[0],actor,'A stable Core ID must retain its skeleton');
assert.equal(actor.userData.clip,'rest','Stopping must stop the gait');
pool.begin();
for(let i=0;i<3;i++)assert.equal(pool.draw({...pose,id:'near-'+i},'near',0),i<2);
pool.end(parent);assert.equal(pool.group.children.length,2,'Near skin budget must be bounded');
pool.begin();assert.equal(pool.draw(pose,'mid',0),false);pool.end(parent);
await new Promise(resolve=>setImmediate(resolve));
pool.begin();for(let i=0;i<5;i++)assert.equal(pool.draw({...pose,id:'mid-'+i},'mid',0),i<4);
pool.end(parent);assert.equal(pool.group.children.length,4,'Mid skin budget must be bounded');
pool.begin();pool.end(parent);assert.equal(pool.group.children.length,0);assert.equal(parent.children.length,0);
let coreBinding='NOT_RUN_NO_FIXTURE';
const fixturePath=resolve(import.meta.dirname,'review/core-fixture.json');
if(existsSync(fixturePath)){
  const {WildlifeLayer}=await source('render/wildlife-layer.ts');
  const {ResidentGroundSurface}=await source('render/resident-grounding.ts');
  const {WORLD_GRID_CONTRACT:grid}=await source('runtime/lifelens-contract.ts');
  const terrain=JSON.parse(readFileSync(fixturePath,'utf8')).terrain;
  const snapshot=JSON.stringify(terrain);
  const tiger=terrain.wildlife.animals.find(animal=>animal.species==='Tiger');
  assert(tiger,'The authoritative fixture must actually contain a tiger');
  const x=(tiger.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
  const z=(tiger.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
  const surface=new ResidentGroundSurface();surface.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);
  const y=surface.height(x,z);assert.notEqual(y,null);
  const camera=new THREE.PerspectiveCamera();camera.position.set(x+3,y+2,z+4);
  const layer=new WildlifeLayer(async tier=>assets.get(tier));
  layer.setTerrain(terrain);await new Promise(resolve=>setImmediate(resolve));layer.update(.016,camera);
  const actor=layer.group.getObjectByName(`LifeLensTiger:${tiger.id}`);
  if(!actor)console.error('TIGER_BINDING_DIAGNOSTIC',{
    observed:terrain.wildlife.animals.filter(animal=>animal.species==='Tiger'),
    accepted:[...layer.animals.values()].filter(animal=>animal.species==='Tiger'),
    loaded:[...layer.tigers.assets.keys()],requested:[...layer.tigers.requested],
    actors:[...layer.tigers.actors.keys()],camera:camera.position.toArray()});
  assert(actor,'Production WildlifeLayer must draw the authoritative tiger using its imported skin');
  assert.equal(actor.userData.coreId,tiger.id);assert.equal(actor.userData.behavior,tiger.behavior);
  assert.equal(actor.position.x,x);assert.equal(actor.position.z,z);
  assert.equal(JSON.stringify(terrain),snapshot,'Rendering must not change the Core observation');
  layer.group.updateMatrixWorld(true);
  let groundedSkin;actor.traverse(object=>{if(object instanceof THREE.SkinnedMesh)groundedSkin=object;});
  groundedSkin.skeleton.update();
  const jointIds=groundedSkin.geometry.getAttribute('skinIndex');
  const jointWeights=groundedSkin.geometry.getAttribute('skinWeight');
  const soleContacts=[];
  for(const name of ['Fore.L.Paw','Fore.R.Paw','Hind.L.Paw','Hind.R.Paw']){
    const bone=groundedSkin.skeleton.bones.findIndex(b=>b.name===THREE.PropertyBinding.sanitizeNodeName(name));
    assert(bone>=0);const points=[];
    for(let i=0;i<jointIds.count;i++){
      if([0,1,2,3].some(j=>jointIds.getComponent(i,j)===bone&&jointWeights.getComponent(i,j)>.5)){
        const point=new THREE.Vector3();groundedSkin.getVertexPosition(i,point);
        point.applyMatrix4(groundedSkin.matrixWorld);points.push(point);
      }
    }
    const minimum=Math.min(...points.map(point=>point.y));
    const errors=points.filter(point=>point.y<=minimum+.012&&surface.height(point.x,point.z)!==null)
      .map(point=>point.y-surface.height(point.x,point.z));
    assert(errors.length>0);const contact=Math.min(...errors);
    assert(Math.abs(contact)<.035,`${name} floats or penetrates actual Core terrain by ${contact}m`);
    soleContacts.push({paw:name,contactError:contact,samples:errors.length});
  }
  const imported=layer.group.getObjectByName('LifeLensImportedTigers');
  const ids=imported.children.map(object=>object.userData.coreId);
  assert.equal(new Set(ids).size,ids.length);
  // Observer origin rebasing must move the presentation while retaining identity.
  layer.setTerrain({...terrain,centerChunkX:terrain.centerChunkX+1});
  camera.position.x-=grid.worldUnitsPerChunk;layer.update(.016,camera);
  assert.equal(layer.group.getObjectByName(`LifeLensTiger:${tiger.id}`),actor);
  assert(Math.abs(actor.position.x-(x-grid.worldUnitsPerChunk))<1e-8);
  camera.position.set(actor.position.x+15,y+2,z);
  layer.update(.016,camera);await new Promise(resolve=>setImmediate(resolve));layer.update(.016,camera);
  const midActor=layer.group.getObjectByName(`LifeLensTiger:${tiger.id}`);
  assert.equal(midActor.userData.tier,'mid');
  layer.setTerrain({...terrain,centerChunkX:terrain.centerChunkX+1});
  assert.equal(layer.group.getObjectByName(`LifeLensTiger:${tiger.id}`),midActor,
    'An unchanged Core refresh must retain the mid-LOD skeleton instead of rebuilding near then mid');
  layer.setTerrain({...terrain,wildlife:{...terrain.wildlife,animals:[]}});
  assert.equal(layer.instanceCount,0);assert.equal(layer.group.getObjectByName('LifeLensImportedTigers'),undefined);
  layer.dispose();coreBinding={status:'PASS_NATIVE_CORE_FIXTURE_BINDING',coreId:tiger.id,coreBehavior:tiger.behavior,
    soleContacts,checked:['skin-path','identity','position','behavior','observation-immutability','sloped-terrain-soles','origin-rebase','lod-refresh-stability','removal']};
}
pool.dispose();assert.equal(pool.draw(pose,'near',0),false);

// Late network completion after disposal must release the source resources.
let complete;let disposed=0;
const lateAsset=assets.get('near');
lateAsset.scene.traverse(object=>{if(object instanceof THREE.Mesh)object.geometry.addEventListener('dispose',()=>disposed++);});
const late=new ImportedTigerPool(()=>new Promise(resolve=>{complete=resolve;}));
late.begin();assert.equal(late.draw(pose,'near',0),false);late.dispose();complete(lateAsset);
await new Promise(resolve=>setImmediate(resolve));assert(disposed>0);assert.equal(late.group.children.length,0);

const result={status:'PASS_SKIN_ANIMATION_AND_POOL_CONTRACT',
  input:inspection?'LOCAL_INSPECTION_DERIVATIVE':'STAGED_PINNED_SOURCE_DERIVATIVE',
  threeRevision:THREE.REVISION,maximumDeformation,nearBudget:2,midBudget:4,
  coreBinding,textures:'NOT_TESTED_IN_NODE',observer:'NOT_RUN',android:'NOT_MEASURED',mergeAllowed:false};
mkdirSync(resolve(import.meta.dirname,'review'),{recursive:true});
writeFileSync(resolve(import.meta.dirname,'review/tiger-contract.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result));
