import assert from 'node:assert/strict';
import {readFileSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
const directory=process.argv[2]??'/workspace/scratch/amil-native-build';
const loader=new GLTFLoader().register(()=>({name:'REAL_SKIN_NO_NODE_TEXTURE_RENDER',loadTexture:()=>Promise.resolve(new T.Texture())}));
const raw=readFileSync(resolve(directory,'tiger-rigged.glb'));
const gltf=await loader.parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');
const contract=gltf.parser.json.asset.extras.lifelensTiger;
assert.equal(contract.version,2);assert.equal(contract.bone_count,47);
let skin;gltf.scene.traverse(o=>{if(o.isSkinnedMesh)skin=o;});assert(skin);assert.equal(skin.skeleton.bones.length,47);
const mixer=new T.AnimationMixer(gltf.scene),report={},failures=[];
function sample(clip,time){mixer.stopAllAction();const a=mixer.clipAction(clip);a.reset().setLoop(T.LoopOnce,1);a.clampWhenFinished=true;a.play();mixer.setTime(time);gltf.scene.updateMatrixWorld(true);skin.skeleton.update();const body=new T.Box3();for(let i=0;i<skin.geometry.attributes.position.count;i++){const p=new T.Vector3();skin.getVertexPosition(i,p);p.applyMatrix4(skin.matrixWorld);body.expandByPoint(p);}
 const paws=contract.paws.map(paw=>{const points=paw.sole_vertices.map(i=>{const p=new T.Vector3();skin.getVertexPosition(i,p);return p.applyMatrix4(skin.matrixWorld);});return {name:paw.name,y:Math.min(...points.map(p=>p.y)),x:points.reduce((a,p)=>a+p.x,0)/points.length,z:points.reduce((a,p)=>a+p.z,0)/points.length};});
 const pelvis=skin.skeleton.bones.find(b=>/Pelvis/.test(b.name)).getWorldPosition(new T.Vector3());const head=skin.skeleton.bones.find(b=>/Head_/.test(b.name)).getWorldPosition(new T.Vector3());return {body:[body.min.toArray(),body.max.toArray()],paws,pelvis:pelvis.toArray(),head:head.toArray()};}
for(const [key,value] of Object.entries(contract.clips)){
 const clip=gltf.animations.find(c=>c.name===value.name);assert(clip);
 const frames=Array.from({length:61},(_,i)=>({t:i/60,...sample(clip,clip.duration*i/60)}));
 const minBody=Math.min(...frames.map(f=>f.body[0][1]));const paws=[];
 for(let p=0;p<4;p++){let start=null,maxSlide=0,contacts=0;for(const f of frames){const paw=f.paws[p];if(paw.y<.008){contacts++;const point={x:paw.x,z:paw.z+value.cycle_distance*f.t};if(start)maxSlide=Math.max(maxSlide,Math.hypot(point.x-start.x,point.z-start.z));else start=point;}else start=null;}paws.push({name:contract.paws[p].name,contacts,maxStanceSlide:maxSlide,minY:Math.min(...frames.map(f=>f.paws[p].y)),maxY:Math.max(...frames.map(f=>f.paws[p].y))});}
 const first=frames[0],last=frames.at(-1),loopError=Math.max(...first.paws.map((p,i)=>Math.hypot(p.x-last.paws[i].x,p.y-last.paws[i].y,p.z-last.paws[i].z)));
 report[key]={minBody,paws,loopError,head:first.head,pelvis:first.pelvis,bounds:first.body};
 if(minBody<-.035)failures.push(`${key}: body penetrates ground ${minBody}m`);
 if(loopError>.0001)failures.push(`${key}: loop seam ${loopError}m`);
 if(['walk','hunt','run'].includes(key)&&paws.some(p=>p.contacts<3||p.maxStanceSlide>.06))failures.push(`${key}: stance contact or slide exceeds 6cm`);
 console.log(key,JSON.stringify(report[key]));
}
if(report.idle.pelvis[1]-report.rest.pelvis[1]<.3)failures.push('Rest pose does not lower the body');
if(report.idle.head[1]-report.drink.head[1]<.25)failures.push('Drink does not lower the head');
const result={status:failures.length?'FAIL_NATIVE_CONTACT':'PASS_NATIVE_CONTACT_ONLY',input:contract.status,sourceHash:contract.source_sha256,report,failures,observer:'NOT_RUN',android:'NOT_MEASURED',mergeAllowed:false};
writeFileSync(resolve(directory,'native-contact-validation.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({status:result.status,failures}));if(failures.length)process.exitCode=1;
