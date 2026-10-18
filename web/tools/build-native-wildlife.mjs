/** Preserve reviewed original animal geometry, UVs and native joint hierarchies.
 * Author presentation clips on those joints; never create simulation outcomes.
 */
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve} from 'node:path';
import {createHash} from 'node:crypto';
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
const root=resolve(import.meta.dirname,'../..');
const out=resolve(root,'docs/asset-reviews/native-wildlife/derived');
const hash=b=>createHash('sha256').update(b).digest('hex');
function uvSignature(gltf){
 const triangles=[];gltf.scene.traverse(o=>{if(!o.isMesh)return;const uv=o.geometry.attributes.uv,index=o.geometry.index;if(!uv||!index)return;
  for(let i=0;i<index.count;i+=3){const t=[];for(let k=0;k<3;k++){const v=index.getX(i+k);t.push(uv.getX(v).toFixed(5)+','+uv.getY(v).toFixed(5));}triangles.push(t.sort().join('|'));}
 });return hash(triangles.sort().join('\n'));
}
const configs=[
 {species:'Hare',slug:'hare',uid:'073eef14ae3c4d06808498c3ba895e67',sha:'2bb6f05bd3e58ccf437ddb05c77a97ddd8ff67b9841b6b69bce90ba275165538',height:.80,head:'head_4',hip:'spine1_18',chest:'spine5_14',neck:'neck_5',
  legs:[['PawL_6','Upper_armL_9'],['PawR_10','Upper_armR_13'],['ToesL_20','upper_legL_23'],['ToesR_24','upper_legR_27']]},
 {species:'Wolf',slug:'wolf',uid:'9e7aceacef0f4486b605abdf1f62e4ac',sha:'8cf8c377a02ac417cda66737c82216c41a2ca329732c1ef7ef88024d7a9afe84',height:1.20,head:'Head_09',hip:'Root_01',chest:'Spine_01_07',neck:'Neck_08',
  legs:[['PawFront_L_025','UperArmFront_L_022'],['PawFront_R_034','UperArmFront_R_031'],['PawBack_L_042','UperArmBack_L_039'],['PawBack_R_051','UperArmBack_R_048']]},
 {species:'Deer',slug:'deer',uid:'347e5d4fbee14e53811c5df2221be8d0',sha:'',height:2.05,head:'head_024',hip:'pelvis_01',chest:'chest_021',neck:'neck_022',
  native:{idle:'IdleDeer',walk:'Walkdeer',run:'Rundeer'},
  legs:[['toe_front_l_041','leg_front_l_038'],['toe_front_r_047','leg_front_r_044'],['toe_back_l_010','leg_back__07'],['toe_back_r_017','leg_back__014']]},
 {species:'FreshwaterFish',slug:'freshwater-fish',uid:'4fa0cb7afc004d65bd49f406bffd5b2b',sha:'25a1a41d574f80f46d716d74bc2572dc573d00da82811fbfb7767bc54da23871',length:.65,yaw:-Math.PI/2,native:{swim:'Swim'},legs:[]},
];
// Deer bytes are pinned in the explicit source manifest beside this generator.
const pins=JSON.parse(readFileSync(resolve(root,'Tools/assets/native-wildlife-pins.json'),'utf8'));
const reports=[];
for(const cfg of configs){
 const dir=resolve(root,'docs/asset-reviews/native-wildlife',cfg.species,cfg.uid);
 const raw=readFileSync(resolve(dir,'source.glb')),receipt=JSON.parse(readFileSync(resolve(dir,'receipt.json'),'utf8'));
 cfg.sha=cfg.sha||pins[cfg.species].sha256;
 if(hash(raw)!==cfg.sha||receipt.sha256!==cfg.sha||receipt.license!=='CC-BY-4.0')throw Error('Source pin/license mismatch: '+cfg.species);
 const doc=JSON.parse(raw.subarray(20,20+raw.readUInt32LE(12)));
 const binary=raw.subarray(28+raw.readUInt32LE(12));
 const gltf=await new GLTFLoader().register(()=>({name:'LIFELENS_OFFLINE_GEOMETRY_AUTHORING',loadTexture:()=>Promise.resolve(new T.Texture())}))
  .parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');
 let upstreamProof=null;
 if(cfg.species==='Deer'){
  const pin=pins.Deer.upstream,folder=resolve(root,'docs/asset-reviews/native-wildlife/Deer',pin.uid),bytes=readFileSync(resolve(folder,'source.glb'));
  const r=JSON.parse(readFileSync(resolve(folder,'receipt.json'),'utf8'));
  if(hash(bytes)!==pin.sha256||r.sha256!==pin.sha256||r.license!=='CC-BY-4.0')throw Error('Original deer creator/license pin mismatch');
  const original=await new GLTFLoader().register(()=>({name:'LIFELENS_UPSTREAM_TOPOLOGY_VERIFICATION',loadTexture:()=>Promise.resolve(new T.Texture())})).parseAsync(bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength),'');
  const signature=uvSignature(gltf);if(signature!==uvSignature(original))throw Error('Deer animation derivative differs from the credited original UV topology');
  upstreamProof={status:'PASS_IDENTICAL_UV_TRIANGLE_MULTISET',...pin,uvTopologySha256:signature};
 }
 const skins=[];gltf.scene.traverse(o=>{if(o.isSkinnedMesh)skins.push(o);});
 const skin=skins.sort((a,b)=>b.geometry.attributes.position.count-a.geometry.attributes.position.count)[0];
 if(!skin)throw Error('Native source skin absent: '+cfg.species);
 const bones=skin.skeleton.bones;
 const canonical=name=>name.replace(/[ .:/]/g,'_').replace(/_+/g,'_').toLowerCase();
 const bone=stem=>bones.find(b=>canonical(b.name).endsWith(canonical(stem))||canonical(b.name).endsWith(canonical(stem.replace(/([a-z])([LR])_/,'$1_$2_'))));
 for(const key of ['head','hip','chest','neck'])if(cfg[key]&&!bone(cfg[key])){
  // glTF's property binding removes punctuation; match the actual native name.
  const flat=s=>s.toLowerCase().replace(/[^a-z0-9]/g,'');
  const found=bones.find(b=>flat(b.name).endsWith(flat(cfg[key])));if(found)cfg[key]=found.name;
 }
 const find=stem=>bone(stem)||bones.find(b=>b.name.toLowerCase().replace(/[^a-z0-9]/g,'').endsWith(stem.toLowerCase().replace(/[^a-z0-9]/g,'')));
 const wp=b=>b.getWorldPosition(new T.Vector3()),wq=b=>b.getWorldQuaternion(new T.Quaternion());
 gltf.scene.updateMatrixWorld(true);skins.forEach(s=>s.skeleton.update());
 const box=new T.Box3();gltf.scene.traverse(o=>{if(!o.isMesh)return;const p=new T.Vector3();for(let i=0;i<o.geometry.attributes.position.count;i++){o.getVertexPosition(i,p);box.expandByPoint(p.applyMatrix4(o.matrixWorld));}});
 const size=box.getSize(new T.Vector3());
 const yaw=cfg.yaw??-Math.atan2(wp(find(cfg.head)).x-wp(find(cfg.hip)).x,wp(find(cfg.head)).z-wp(find(cfg.hip)).z);
 const unit=cfg.length?cfg.length/Math.max(size.x,size.z):cfg.height/size.y;
 const nt=new T.Matrix4().makeRotationY(yaw).scale(new T.Vector3(unit,unit,unit));
 const center=box.getCenter(new T.Vector3()).applyMatrix4(nt);nt.setPosition(-center.x,-box.min.y*unit,-center.z);
 const frame=new T.Group();frame.matrixAutoUpdate=false;frame.matrix.copy(nt);frame.add(gltf.scene);
 const motionFrame=new T.Group();motionFrame.add(frame);motionFrame.updateMatrixWorld(true);
 const initial=new Map();gltf.scene.traverse(o=>initial.set(o,{p:o.position.clone(),q:o.quaternion.clone(),s:o.scale.clone()}));
 const restore=()=>{motionFrame.position.set(0,0,0);for(const[o,s]of initial){o.position.copy(s.p);o.quaternion.copy(s.q);o.scale.copy(s.s);}motionFrame.updateMatrixWorld(true);};
 const rotate=(b,axis,a)=>{if(!b)return;b.quaternion.copy(wq(b.parent).invert().multiply(new T.Quaternion().setFromAxisAngle(axis,a).multiply(wq(b))));b.updateWorldMatrix(false,true);};
 const move=(b,delta)=>{const p=wp(b).add(delta);b.position.copy(b.parent.worldToLocal(p));b.updateWorldMatrix(false,true);};
 const legs=cfg.legs.map(([pawName,upperName])=>{
  const paw=find(pawName),upper=find(upperName);if(!paw||!upper)throw Error('Native joint absent '+cfg.species+' '+pawName+' '+upperName);
  const chain=[];for(let b=paw.parent;b?.isBone;b=b.parent){chain.push(b);if(b===upper)break;}
  if(chain.at(-1)!==upper||chain.length>3)throw Error('Unexpected native limb chain');
  const family=new Set();paw.traverse(b=>{if(b.isBone)family.add(bones.indexOf(b));});
  const g=skin.geometry,j=g.attributes.skinIndex,w=g.attributes.skinWeight,candidates=[];
  for(let i=0;i<g.attributes.position.count;i++){
   let total=0;for(let k=0;k<4;k++)if(family.has(j.getComponent(i,k)))total+=w.getComponent(i,k);
   if(total>.3){const p=new T.Vector3();skin.getVertexPosition(i,p);p.applyMatrix4(skin.matrixWorld);candidates.push({i,y:p.y});}
  }
  if(candidates.length<8)throw Error('Original weighted foot surface absent '+cfg.species+' '+paw.name+' '+candidates.length);
  const low=Math.min(...candidates.map(v=>v.y));let soles=candidates.filter(v=>v.y<low+(cfg.species==='Hare'?.018:.03)).map(v=>v.i);
  if(soles.length<8)soles=candidates.sort((a,b)=>a.y-b.y).slice(0,8).map(v=>v.i);
  return {paw,upper,chain,soles,position:wp(paw),orientation:wq(paw),soleHeight:low};
 });
 function solve(leg,goal){
  for(let iteration=0;iteration<32;iteration++){
   if(wp(leg.paw).distanceTo(goal)<.0005)break;
   for(const joint of leg.chain){const pivot=wp(joint),a=wp(leg.paw).sub(pivot),b=goal.clone().sub(pivot);if(a.lengthSq()<1e-12||b.lengthSq()<1e-12)continue;a.normalize();b.normalize();const axis=a.clone().cross(b);if(axis.lengthSq()>1e-12)rotate(joint,axis.normalize(),Math.min(.3,Math.acos(T.MathUtils.clamp(a.dot(b),-1,1))));}
  }
  leg.paw.quaternion.copy(wq(leg.paw.parent).invert().multiply(leg.orientation));leg.paw.updateWorldMatrix(false,true);
 }
 const mixer=new T.AnimationMixer(gltf.scene);
 const referenceJoint=cfg.hip?find(cfg.hip):bones[0],referencePosition=wp(referenceJoint);
 const native=(name,phase)=>{
  const clip=gltf.animations.find(a=>a.name===cfg.native?.[name]);if(!clip)return false;
  const first=Math.min(...clip.tracks.map(t=>t.times[0])),last=Math.max(...clip.tracks.map(t=>t.times.at(-1)));
  const action=mixer.clipAction(clip).reset().setLoop(T.LoopOnce,1);action.clampWhenFinished=true;action.play();mixer.setTime(first+(last-first)*phase);frame.updateMatrixWorld(true);return true;
 };
 const objects=[];for(const[obj,a]of gltf.parser.associations)if(Number.isInteger(a.nodes)&&!objects.some(v=>v.index===a.nodes))objects.push({obj,index:a.nodes});
 const bodyAssociation=gltf.parser.associations.get(skin);let bodyNode=bodyAssociation?.nodes;
 if(bodyNode===undefined)bodyNode=doc.nodes.findIndex(n=>n.skin!==undefined&&n.mesh===bodyAssociation?.meshes);
 if(bodyNode<0)throw Error('Original body node association missing');
 const clipNames=cfg.species==='FreshwaterFish'?['idle','swim']:['idle','walk','run','hunt','forage','drink','rest'];
 const baked=[],metrics={};
 const axisX=new T.Vector3(1,0,0),axisY=new T.Vector3(0,1,0);
 for(const name of clipNames){
  const moving=['walk','run','hunt'].includes(name),run=name==='run',hare=cfg.species==='Hare';
  const duration=run?(hare?.5:.65):moving?(hare?.7:1.15):name==='swim'?1:2;
  const stance=run?.36:hare?.43:.64;
  const stride=moving?(run?(hare?.36:.70):(hare?.19:.32)):0;
  const offsets=hare?[0,.02,.49,.51]:run?[.32,.38,.84,.90]:[.25,.75,0,.5];
  const count=Math.round(duration*40)+1,samples=[];
  for(let f=0;f<count-1;f++){
   const phase=f/(count-1);mixer.stopAllAction();restore();
   const usedNative=native(name==='hunt'?'walk':name==='rest'?'idle':name==='idle'&&cfg.species==='FreshwaterFish'?'swim':name,phase);
   const currentRoot=wp(referenceJoint);motionFrame.position.set(referencePosition.x-currentRoot.x,0,referencePosition.z-currentRoot.z);motionFrame.updateMatrixWorld(true);
   if(cfg.species!=='FreshwaterFish'){
    const hip=find(cfg.hip),chest=find(cfg.chest),head=find(cfg.head),neck=find(cfg.neck);
    if(name==='rest'){
     // Core Rest is stationary rest, not an instruction to lie down. Retain
     // the source's supported seated/standing anatomy and relax the head.
     rotate(neck,axisX,.035);rotate(head,axisX,.025+.008*Math.sin(phase*Math.PI*2));
     if(!usedNative)move(chest,new T.Vector3(0,.0015*Math.sin(phase*Math.PI*2),0));
    }else if(name==='drink'||name==='forage'){
     motionFrame.position.y=-(hare?.055:.13);motionFrame.updateMatrixWorld(true);rotate(chest,axisX,hare?.10:.18);rotate(neck,axisX,hare?.36:.75);rotate(head,axisX,hare?.18:.4);
    }else if(!usedNative){
     const bob=moving?(run?.025:.008):.002;move(chest,new T.Vector3(0,bob*Math.sin(phase*Math.PI*2),0));
    }
    for(let i=0;i<legs.length;i++){
     const leg=legs[i],p=(phase+offsets[i])%1,goal=leg.position.clone();goal.y-=leg.soleHeight;
     if(moving){const swing=(p-stance)/(1-stance);goal.z+=p<stance?stride*(.5-p/stance):stride*(-.5+swing);if(p>=stance)goal.y+=(run?.13:hare?.075:.07)*Math.sin(Math.PI*swing);}
     solve(leg,goal);
    }
   }
   frame.updateMatrixWorld(true);skins.forEach(s=>s.skeleton.update());
   const feet=legs.map(l=>{const points=l.soles.map(i=>{const p=new T.Vector3();skin.getVertexPosition(i,p);return p.applyMatrix4(skin.matrixWorld);});return {y:Math.min(...points.map(p=>p.y)),x:points.reduce((s,p)=>s+p.x,0)/points.length,z:points.reduce((s,p)=>s+p.z,0)/points.length};});
   let floor=feet.length?-Math.min(0,...feet.map(v=>v.y)):0;
   if(name==='rest'){
    let low=Infinity;for(const s of skins){const p=new T.Vector3();for(let i=0;i<s.geometry.attributes.position.count;i++){s.getVertexPosition(i,p);low=Math.min(low,p.applyMatrix4(s.matrixWorld).y);}}floor=Math.max(floor,-low);
   }
   samples.push({time:duration*phase,floor,offset:[motionFrame.position.x,motionFrame.position.y+floor,motionFrame.position.z],feet,transforms:objects.map(({obj,index})=>({index,p:obj.position.toArray(),q:obj.quaternion.toArray(),s:obj.scale.toArray()}))});
  }
  samples.push({...structuredClone(samples[0]),time:duration});
  baked.push({name,duration,distance:moving?stride/stance:name==='swim'?.4:0,samples,stance,offsets});
  metrics[name]={frames:count,maxPenetration:feetMin(samples),floorRange:[Math.min(...samples.map(s=>s.floor)),Math.max(...samples.map(s=>s.floor))]};
 }
 function feetMin(samples){return samples.length?Math.min(0,...samples.flatMap(s=>s.feet.map(f=>f.y+s.floor))):0;}
 const chunks=[binary];let length=binary.length;
 function accessor(values,type){
  const data=new Float32Array(values),pad=(4-length%4)%4;if(pad){chunks.push(Buffer.alloc(pad));length+=pad;}
  const view=doc.bufferViews.length;doc.bufferViews.push({buffer:0,byteOffset:length,byteLength:data.byteLength});chunks.push(Buffer.from(data.buffer));length+=data.byteLength;
  const index=doc.accessors.length;doc.accessors.push({bufferView:view,componentType:5126,type,count:values.length/({SCALAR:1,VEC3:3,VEC4:4}[type]),...(type==='SCALAR'?{min:[Math.min(...values)],max:[Math.max(...values)]}:{})});return index;
 }
 const scene=doc.scenes[doc.scene??0];const normalizeNode=doc.nodes.length;doc.nodes.push({name:`LifeLens${cfg.species}SourceFrame`,matrix:nt.toArray(),children:scene.nodes});
 const floorNode=doc.nodes.length;doc.nodes.push({name:`LifeLens${cfg.species}ContactOffset`,children:[normalizeNode]});scene.nodes=[floorNode];
 doc.animations=[];
 for(const clip of baked){
  const a={name:`${cfg.slug}_${clip.name}`,samplers:[],channels:[]};
  for(const{index}of objects)for(const[path,type,key]of[['translation','VEC3','p'],['rotation','VEC4','q'],['scale','VEC3','s']]){
   const values=clip.samples.map(s=>s.transforms.find(t=>t.index===index)[key]);
   const constant=values.every(v=>v.every((x,k)=>Math.abs(x-values[0][k])<1e-7));
   const sampler=a.samplers.length;a.samplers.push({input:accessor(constant?[0,clip.duration]:clip.samples.map(s=>s.time),'SCALAR'),output:accessor(constant?[...values[0],...values[0]]:values.flat(),type),interpolation:'LINEAR'});a.channels.push({sampler,target:{node:index,path}});
  }
  const sampler=a.samplers.length;a.samplers.push({input:accessor(clip.samples.map(s=>s.time),'SCALAR'),output:accessor(clip.samples.flatMap(s=>s.offset),'VEC3'),interpolation:'LINEAR'});a.channels.push({sampler,target:{node:floorNode,path:'translation'}});doc.animations.push(a);
 }
 for(const t of baked[0].samples[0].transforms){delete doc.nodes[t.index].matrix;Object.assign(doc.nodes[t.index],{translation:t.p,rotation:t.q,scale:t.s});}
 doc.nodes[bodyNode].name=`LifeLens${cfg.species}Body`;
 for(const m of doc.materials??[]){const p=m.pbrMetallicRoughness??={};p.metallicFactor=0;p.roughnessFactor=cfg.species==='FreshwaterFish'?.5:.88;
  if(cfg.species==='Deer'){const n=m.name?.toLowerCase()??'';p.baseColorFactor=n.includes('eyewai')?[.7,.62,.49,1]:n.includes('eye')||n.includes('yandi')?[.025,.018,.012,1]:n.includes('jiao')?[.25,.18,.11,1]:n.includes('kouqiang')?[.08,.035,.025,1]:[.24,.115,.047,1];}
 }
 const creator=upstreamProof?`${upstreamProof.author} (mesh/rig); ${receipt.author.displayName||receipt.author.username} (animation derivative)`:receipt.author.displayName||receipt.author.username;
 const contract={version:1,species:cfg.species,tier:'near',units:'metres',forward:'+Z',up:'+Y',license:receipt.license,creator,source_sha256:cfg.sha,
  body:`LifeLens${cfg.species}Body`,bone_count:bones.length,status:'DRAFT_REQUIRES_RUNTIME_REVIEW',
  ...(upstreamProof?{upstream_source_sha256:upstreamProof.sha256}:{}),
  clips:Object.fromEntries(baked.map(c=>[c.name,{name:`${cfg.slug}_${c.name}`,duration:c.duration,cycle_distance:c.distance,stance:c.stance,offsets:c.offsets}])),
  paws:legs.map(l=>({name:l.paw.name,upper:l.upper.name,sole_vertices:l.soles})),boundsHeight:cfg.height??size.y*unit};
 doc.asset.extras={...(doc.asset.extras??{}),lifelensWildlife:contract};doc.asset.copyright=`${receipt.name} by ${creator}, CC BY 4.0. LifeLens metric normalization, native-joint presentation clips and contact trajectories.`;
 doc.buffers[0].byteLength=length;let packed=Buffer.concat(chunks);packed=Buffer.concat([packed,Buffer.alloc((4-packed.length%4)%4)]);
 let encoded=Buffer.from(JSON.stringify(doc));encoded=Buffer.concat([encoded,Buffer.alloc((4-encoded.length%4)%4,32)]);
 const header=Buffer.alloc(20);header.write('glTF');header.writeUInt32LE(2,4);header.writeUInt32LE(28+encoded.length+packed.length,8);header.writeUInt32LE(encoded.length,12);header.writeUInt32LE(0x4e4f534a,16);
 const bh=Buffer.alloc(8);bh.writeUInt32LE(packed.length);bh.writeUInt32LE(0x004e4942,4);
 const result=Buffer.concat([header,encoded,bh,packed]);mkdirSync(out,{recursive:true});writeFileSync(resolve(out,`${cfg.slug}-rigged.glb`),result);
 const report={species:cfg.species,sourceUid:cfg.uid,sourceSha256:cfg.sha,upstreamProof,derivedSha256:hash(result),bytes:result.length,contract,metrics};reports.push(report);
 console.log('NATIVE_SOURCE_DERIVATIVE',cfg.species,result.length,JSON.stringify(metrics));
}
writeFileSync(resolve(out,'authoring.json'),JSON.stringify({status:'DRAFT_SOURCE_DERIVATIVES',runtimeAdmission:false,generatorSha256:hash(readFileSync(import.meta.filename)),reports},null,2)+'\n');
