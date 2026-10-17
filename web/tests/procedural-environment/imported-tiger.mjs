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
for(const [tier,name] of [['near','tiger-rigged.glb'],['mid','tiger-mid.glb'],['far','tiger-far.glb']]){
  const buffer=readFileSync(resolve(directory,name));
  const loader=new GLTFLoader().register(()=>({name:'LIFELENS_NODE_TEXTURE_TEST',
    loadTexture:()=>Promise.resolve(new THREE.Texture())}));
  const gltf=await loader.parseAsync(buffer.buffer.slice(buffer.byteOffset,buffer.byteOffset+buffer.byteLength),'');
  assets.set(tier,gltf);
  if(tier==='far'){assert.equal(gltf.animations.length,0);assert.equal(gltf.scenes.length,3);continue;}
  assert.equal(gltf.animations.length,7);
  let skins=0;
  gltf.scene.traverse(object=>{if(object instanceof THREE.SkinnedMesh){
    skins++;assert.equal(object.skeleton.bones.length,47);
    const weights=object.geometry.getAttribute('skinWeight');
    for(let i=0;i<weights.count;i++)assert(Math.abs(weights.getX(i)+weights.getY(i)+weights.getZ(i)+weights.getW(i)-1)<1e-5);
  }});
  assert.equal(skins,1);
}
// A behavior change must blend real skin poses, rather than teleport the joints.
const blendPool=new ImportedTigerPool(async tier=>assets.get(tier));
const blendPose={id:'core-blend',x:0,y:0,z:0,heading:0,scale:1,speed:0,phase:.7,behavior:'Rest'};
const blendParent=new THREE.Group();
function blendFrame(pose,dt){blendPool.begin();const ready=blendPool.draw(pose,'near',dt);blendPool.end(blendParent);return ready;}
blendFrame(blendPose,0);await new Promise(resolve=>setImmediate(resolve));blendFrame(blendPose,0);
let blendSkin;blendPool.group.traverse(obj=>{if(obj instanceof THREE.SkinnedMesh)blendSkin=obj;});
function blendVertex(){blendParent.updateMatrixWorld(true);blendSkin.skeleton.update();const p=new THREE.Vector3();blendSkin.getVertexPosition(0,p);return p;}
const restVertex=blendVertex();
const walkPose={...blendPose,behavior:'Roam',speed:.6};
blendFrame(walkPose,0);
assert(restVertex.distanceTo(blendVertex())<1e-5,'Starting a blend must preserve the current real skin pose');
for(let i=0;i<36;i++)blendFrame(walkPose,1/60);
assert(restVertex.distanceTo(blendVertex())>.01,'The stand transition must eventually reach moving skin poses');
const walkVertex=blendVertex();
const huntPose={...walkPose,behavior:'Hunt'};
blendFrame(huntPose,0);
assert(walkVertex.distanceTo(blendVertex())<1e-5,'Changing locomotion must not snap the deformed mesh');
for(let i=0;i<24;i++)blendFrame(huntPose,1/60);
const blendActor=blendPool.actors.get(blendPose.id);
assert.equal([...blendActor.actions.values()].filter(a=>a.isRunning()).length,1,'Finished blends must release outgoing animation work');
assert.equal(blendActor.current,'hunt');
blendPool.dispose();
console.log('PASS actual source skin continuity for rest/walk/hunt transitions');

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
  const soleContacts=[];
  for(const paw of assets.get('near').parser.json.asset.extras.lifelensTiger.paws){
    const errors=paw.sole_vertices.map(index=>{
      const point=new THREE.Vector3();groundedSkin.getVertexPosition(index,point);
      point.applyMatrix4(groundedSkin.matrixWorld);const height=surface.height(point.x,point.z);
      return height===null?null:point.y-height;
    }).filter(error=>error!==null);
    assert(errors.length>0);const contact=Math.min(...errors);
    assert(Math.abs(contact)<.035,`${paw.name} floats or penetrates actual Core terrain by ${contact}m`);
    soleContacts.push({paw:paw.name,contactError:contact,samples:errors.length});
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
// Far actors retain exact IDs and root transforms with three shared source poses.
pool.begin();assert.equal(pool.draw(pose,'far',0),false);pool.end(parent);
await new Promise(resolve=>setImmediate(resolve));
pool.begin();
for(let i=0;i<129;i++)assert.equal(pool.draw({...pose,id:'far-'+i,x:i,behavior:i%3===0?'Rest':i%3===1?'Forage':'Roam'},'far',.016),i<128);
pool.end(parent);
const farGroup=pool.group.getObjectByName('LifeLensTigerFarInstances');
assert.equal(farGroup.userData.coreIds.length,128);
assert.equal(new Set(farGroup.userData.coreIds).size,128);
assert.equal(pool.actors.size,0,'Far actors must not retain detailed skeletons');
let total=0;
for(const mesh of farGroup.children){
  assert(mesh instanceof THREE.InstancedMesh);total+=mesh.count;
  assert(mesh.geometry.index.count/3<=1100);
  for(let i=0;i<mesh.count;i++){
    const m=new THREE.Matrix4();mesh.getMatrixAt(i,m);
    assert.equal(m.elements[12],Number(mesh.userData.coreIds[i].slice(4)));
  }
}
assert.equal(total,128);
assert(farGroup.getObjectByName('LifeLensTigerFarRest').count>0);
assert(farGroup.getObjectByName('LifeLensTigerFarForage').count>0);
pool.begin();pool.end(parent);assert.equal(parent.children.length,0);
// Inspection derivatives are deliberately barred from the production loader.
const rejected={...assets.get('near'),parser:{json:structuredClone(assets.get('near').parser.json)}};
rejected.parser.json.asset.extras.lifelensInspectionOnly={runtimeAdmission:false};
const rejectPool=new ImportedTigerPool(async()=>rejected);
rejectPool.begin();assert.equal(rejectPool.draw(pose,'near',0),false);
await new Promise(resolve=>setImmediate(resolve));
assert.equal(rejectPool.draw(pose,'near',0),false);rejectPool.dispose();
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
  threeRevision:THREE.REVISION,maximumDeformation,nearBudget:2,midBudget:4,farBudget:128,farTrianglesPerActor:998,
  coreBinding,textures:'NOT_TESTED_IN_NODE',observer:'NOT_RUN',android:'NOT_MEASURED',mergeAllowed:false};
mkdirSync(resolve(import.meta.dirname,'review'),{recursive:true});
writeFileSync(resolve(import.meta.dirname,'review/tiger-contract.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result));
