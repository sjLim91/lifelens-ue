import assert from 'node:assert/strict';
import {readFileSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {clone} from 'three/addons/utils/SkeletonUtils.js';
import {source} from './compile.mjs';
const {NativeTigerGrounding}=await source('render/native-tiger-grounding.ts');
const directory=process.argv[2]??'/workspace/scratch/amil-native-build';
const raw=readFileSync(resolve(directory,'tiger-rigged.glb'));
const loader=new GLTFLoader().register(()=>({name:'GROUNDING_GEOMETRY_ONLY',loadTexture:()=>Promise.resolve(new T.Texture())}));
const gltf=await loader.parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');
const contract=gltf.parser.json.asset.extras.lifelensTiger;
const terrain=(x,z)=>23.2+.17*x+.22*z+.035*Math.sin(4*x)*Math.sin(3*z);
const scenarios=[],failures=[];
for(const key of ['idle','walk','hunt','run','rest','drink']){
 const root=new T.Group(),frame=new T.Group(),model=clone(gltf.scene);root.add(frame);frame.add(model);
 let skin;model.traverse(o=>{if(o.isSkinnedMesh)skin=o;});
 const feet=new NativeTigerGrounding(root,frame,model,contract.paws);
 const mixer=new T.AnimationMixer(model),clip=gltf.animations.find(c=>c.name===contract.clips[key].name);
 mixer.clipAction(clip).play();const duration=clip.duration,phases=key==='run'?[.32,.38,.84,.90]:[.25,.75,0,.5],stance=key==='run'?.36:key==='hunt'?.70:.62;
 let maxContactError=0,contacts=0,worst=null;const observations=[];
 for(let i=0;i<91;i++){
  const time=duration*i/60,phase=time/duration%1,heading=Math.sin(i/90)*.7;
  feet.restore();mixer.setTime(time);
  const speed=['walk','hunt','run'].includes(key)?contract.clips[key].cycle_distance/duration*.8:0;
  const z=speed*time,x=.03*Math.sin(i/30);
  root.position.set(x,terrain(x,z),z);root.rotation.y=heading;root.scale.setScalar(.8);
  const span=.3,sx=(terrain(x+span,z)-terrain(x-span,z))/(span*2),sz=(terrain(x,z+span)-terrain(x,z-span))/(span*2);
  const normal=new T.Vector3(-sx,1,-sz).normalize().applyAxisAngle(new T.Vector3(0,1,0),-heading);
  frame.quaternion.setFromUnitVectors(new T.Vector3(0,1,0),normal);
  feet.apply({id:'native-terrain',x,y:terrain(x,z),z,heading,scale:.8,speed,phase:0,behavior:key==='rest'?'Rest':'Hunt'},key,phase,false,terrain);
  root.updateMatrixWorld(true);skin.skeleton.update();
  const values=[];
  for(let l=0;l<4;l++){
   const errors=contract.paws[l].sole_vertices.map(index=>{const p=new T.Vector3();skin.getVertexPosition(index,p);p.applyMatrix4(skin.matrixWorld);return p.y-terrain(p.x,p.z);});
   const minimum=Math.min(...errors);values.push(minimum);
   if(!['walk','hunt','run'].includes(key)||(phase+phases[l])%1<stance){if(Math.abs(minimum)>maxContactError){maxContactError=Math.abs(minimum);const foot=feet.feet[l];worst={frame:i,paw:l,phase,error:minimum,planted:foot.planted,anchor:foot.anchor.toArray(),actual:foot.paw.getWorldPosition(new T.Vector3()).toArray()};}contacts++;}
   assert(Number.isFinite(minimum));
  }
  observations.push({frame:i,phase,errors:values});
 }
 scenarios.push({clip:key,maxContactError,contacts,worst});console.log(key,maxContactError,contacts,JSON.stringify(worst));
 if(maxContactError>=.035)failures.push(`${key} foot leaves actual sloped terrain by ${maxContactError}m`);
}
const report={status:failures.length?'FAIL_NATIVE_TERRAIN':'PASS_NATIVE_DEFORMED_TERRAIN_CONTACT_ONLY',failures,input:contract.status,sourceHash:contract.source_sha256,scenarios,observer:'NOT_RUN',android:'NOT_MEASURED',mergeAllowed:false};
writeFileSync(resolve(directory,'native-terrain-validation.json'),JSON.stringify(report,null,2)+'\n');

if(failures.length)process.exitCode=1;
