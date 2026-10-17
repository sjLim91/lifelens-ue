/** Source-preserving CC BY tiger authoring. No primitive body or reweighting. */
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve} from 'node:path';
import {createHash} from 'node:crypto';
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {MeshoptSimplifier} from 'meshoptimizer';
const project=resolve(import.meta.dirname,'../..');
const flags=process.argv.slice(2);
const inspection=flags.includes('--inspection');
const inputRoot=flags.includes('--input')?resolve(flags[flags.indexOf('--input')+1]):resolve(project,'docs/asset-reviews/distributed-tigers');
const output=flags.includes('--output')?resolve(flags[flags.indexOf('--output')+1]):resolve(project,'docs/asset-reviews/amil-tiger');
const ids=['fc2c7fd74dd04e39935803b884c77258','733afd855f5d4c378f041cc89e231770'];
const hashes=['6e152d1d98c6149d4241c292bf21cbc9e8ab16f81a456ea1515d174f7f7d1e40','4a6e97f3130d5f525e52e958cfcd687ee0ba1224eca18cff1a070b4765caec28'];
const digest=data=>createHash('sha256').update(data).digest('hex');
async function load(index){
 const raw=readFileSync(resolve(inputRoot,ids[index],inspection?'inspection.glb':'source.glb'));
 if(!inspection&&digest(raw)!==hashes[index])throw Error('Pinned original source mismatch');
 const loader=new GLTFLoader().register(()=>({name:'LIFELENS_AUTHORING_NO_TEXTURE_RENDER',loadTexture:()=>Promise.resolve(new T.Texture())}));
 const gltf=await loader.parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');
 let skin;gltf.scene.traverse(obj=>{if(obj.isSkinnedMesh){if(skin)throw Error('Unexpected second body');skin=obj;}});
 if(!skin)throw Error('Original weighted body missing');
 gltf.scene.updateMatrixWorld(true);
 return {raw,gltf,skin};
}
const target=await load(0),donor=await load(1);
const geometry=target.skin.geometry;
const canonical=name=>name.replace(/_\d+$/,'');
function uvTopology(model){
 const uv=model.skin.geometry.getAttribute('uv'),index=model.skin.geometry.index;
 const triangles=[];for(let i=0;i<index.count;i+=3){const tri=[];for(let j=0;j<3;j++){const v=index.getX(i+j);tri.push(uv.getX(v).toFixed(5)+','+uv.getY(v).toFixed(5));}tri.sort();triangles.push(tri.join('|'));}
 triangles.sort();return digest(triangles.join('\n'));
}
const topology=uvTopology(target);if(topology!==uvTopology(donor))throw Error('Donor UV topology differs from credited original');
const bindMap=new T.Matrix4().set(
 -643.2084350585938,765.346435546875,124.41268920898438,-564.3565063476562,
 771.2910766601562,648.1209106445312,.514186680316925,757.38916015625,
 -79.64775085449219,95.57726287841797,-999.7359008789062,-32.422462463378906,0,0,0,1);
const inverseBindMap=bindMap.clone().invert();
function normalization(model,scale,heading){
 const bones=model.skin.skeleton.bones;
 const hip=bones.find(b=>/Pelvis/.test(b.name)).getWorldPosition(new T.Vector3());
 const chest=bones.find(b=>/Spine2_/.test(b.name)).getWorldPosition(new T.Vector3());
 const center=hip.add(chest).multiplyScalar(.5);
 const matrix=new T.Matrix4().makeRotationY(heading).scale(new T.Vector3(scale,scale,scale));
 center.applyMatrix4(matrix);matrix.setPosition(-center.x,0,-center.z);return matrix;
}
const unit=.0117,nt=normalization(target,unit,0),nd=normalization(donor,.117,Math.PI);
const worldMap=nt.clone().invert().multiply(nd);
const bones=target.skin.skeleton.bones,donorBones=new Map(donor.skin.skeleton.bones.map((b,index)=>[canonical(b.name),{bone:b,index}]));
const mappings=bones.map((bone,index)=>{
 const match=index===0?null:donorBones.get(canonical(bone.name));
 return !match?{bone,index}:{bone,index,donor:match.bone,correction:donor.skin.skeleton.boneInverses[match.index].clone().multiply(inverseBindMap).multiply(target.skin.skeleton.boneInverses[index].clone().invert())};
});
if(mappings.filter(m=>m.donor).length!==38||bones.length!==47)throw Error('Reviewed native skeleton identity changed');
const initial=new Map();target.gltf.scene.traverse(obj=>initial.set(obj,{position:obj.position.clone(),quaternion:obj.quaternion.clone(),scale:obj.scale.clone()}));
function restore(){for(const [obj,state] of initial){obj.position.copy(state.position);obj.quaternion.copy(state.quaternion);obj.scale.copy(state.scale);}target.gltf.scene.updateMatrixWorld(true);}
const sourceClip=target.gltf.animations[0],donorClip=donor.gltf.animations[0];
const sourceMixer=new T.AnimationMixer(target.gltf.scene),sourceAction=sourceMixer.clipAction(sourceClip);
const donorMixer=new T.AnimationMixer(donor.gltf.scene),donorAction=donorMixer.clipAction(donorClip);
sourceAction.setLoop(T.LoopOnce,1);sourceAction.clampWhenFinished=true;
donorAction.setLoop(T.LoopOnce,1);donorAction.clampWhenFinished=true;donorAction.play();
const bone=semantic=>bones.find(b=>canonical(b.name)===semantic);
const pWorld=b=>b.getWorldPosition(new T.Vector3());
const qWorld=b=>b.getWorldQuaternion(new T.Quaternion());
function rotateWorld(b,axis,angle){
 const q=new T.Quaternion().setFromAxisAngle(axis,angle).multiply(qWorld(b));
 b.quaternion.copy(qWorld(b.parent).invert().multiply(q));b.updateMatrixWorld(true);
}
function moveWorld(b,delta){const p=pWorld(b).add(delta);b.position.copy(b.parent.worldToLocal(p));b.updateMatrixWorld(true);}
const axisX=new T.Vector3(1,0,0),axisY=new T.Vector3(0,1,0);
const legs=[
 {name:'fore-left',paw:bone('Bip01_L_Hand'),chain:[bone('Bip01_L_Forearm'),bone('Bip01_L_UpperArm')]},
 {name:'fore-right',paw:bone('Bip01_R_Hand'),chain:[bone('Bip01_R_Forearm'),bone('Bip01_R_UpperArm')]},
 {name:'hind-left',paw:bone('Bip01_L_Foot'),chain:[bone('Bip01_L_HorseLink'),bone('Bip01_L_Calf'),bone('Bip01_L_Thigh')]},
 {name:'hind-right',paw:bone('Bip01_R_Foot'),chain:[bone('Bip01_R_HorseLink'),bone('Bip01_R_Calf'),bone('Bip01_R_Thigh')]},
];
restore();target.skin.skeleton.update();
for(const leg of legs){
 if(!leg.paw||leg.chain.some(b=>!b))throw Error('Native limb joint absent');
 leg.position=pWorld(leg.paw);leg.quaternion=qWorld(leg.paw);
 const family=new Set();leg.paw.traverse(b=>{if(b.isBone)family.add(bones.indexOf(b));});
 const joints=geometry.getAttribute('skinIndex'),weights=geometry.getAttribute('skinWeight');
 const candidates=[];
 for(let v=0;v<geometry.attributes.position.count;v++){
  let weight=0;for(let j=0;j<4;j++)if(family.has(joints.getComponent(v,j)))weight+=weights.getComponent(v,j);
  if(weight>.5){const p=new T.Vector3();target.skin.getVertexPosition(v,p);p.applyMatrix4(target.skin.matrixWorld).applyMatrix4(nt);candidates.push({index:v,height:p.y});}
 }
 if(candidates.length<8)throw Error('Native paw skin region absent: '+leg.name);
 const low=Math.min(...candidates.map(p=>p.height));leg.soleHeight=low;leg.soles=candidates.filter(p=>p.height<low+.025).map(p=>p.index);
}
// Paw pads are rigid contact surfaces. Keep source weights elsewhere; isolate
// only the reviewed sole vertices from forearm/hock influences that caused slip.
const correctedPads=new Set();
for(const leg of legs){const joint=bones.indexOf(leg.paw);for(const vertex of leg.soles){
 for(let k=0;k<4;k++){geometry.attributes.skinIndex.setComponent(vertex,k,k===0?joint:0);geometry.attributes.skinWeight.setComponent(vertex,k,k===0?1:0);}correctedPads.add(vertex);
}}
function ccd(leg,goal){
 for(let attempt=0;attempt<30;attempt++){
  if(pWorld(leg.paw).distanceTo(goal)*unit<.0005)break;
  for(const joint of leg.chain){
   const pivot=pWorld(joint),a=pWorld(leg.paw).sub(pivot),b=goal.clone().sub(pivot);
   if(a.lengthSq()<1e-12||b.lengthSq()<1e-12)continue;a.normalize();b.normalize();
   const cross=a.clone().cross(b);if(cross.lengthSq()<1e-12)continue;
   rotateWorld(joint,cross.normalize(),Math.min(.35,Math.acos(T.MathUtils.clamp(a.dot(b),-1,1))));
  }
 }
 const q=qWorld(leg.paw.parent).invert().multiply(leg.quaternion);leg.paw.quaternion.copy(q);
 leg.paw.traverse(child=>{if(child!==leg.paw&&child.isBone){const rest=initial.get(child);child.position.copy(rest.position);child.quaternion.copy(rest.quaternion);child.scale.copy(rest.scale);}});
 leg.paw.updateMatrixWorld(true);
}
function retarget(phase,amount){
 donorMixer.setTime(donorClip.duration*phase);donor.gltf.scene.updateMatrixWorld(true);
 for(const {bone,donor,correction} of mappings){
  if(!donor){bone.updateMatrixWorld(true);continue;}
  const desired=worldMap.clone().multiply(donor.matrixWorld).multiply(correction);
  bone.parent.matrixWorld.clone().invert().multiply(desired).decompose(bone.position,bone.quaternion,bone.scale);
  if(amount<1){const rest=initial.get(bone);bone.position.lerpVectors(rest.position,bone.position,amount);bone.quaternion.slerpQuaternions(rest.quaternion,bone.quaternion,amount);bone.scale.lerpVectors(rest.scale,bone.scale,amount);}
  bone.updateMatrixWorld(true);
 }
}
function sample(name,phase){
 sourceMixer.stopAllAction();restore();
 if(name==='run'){sourceAction.reset().play();sourceMixer.setTime(sourceClip.duration*phase);target.gltf.scene.updateMatrixWorld(true);}
 else if(name==='walk'||name==='hunt')retarget(phase,1);
 else{
  if(name==='rest'){
   moveWorld(bones[0],new T.Vector3(0,-.43/unit,0));
   rotateWorld(bone('Bip01_Neck'),axisX,.42);rotateWorld(bone('Bip01_Head'),axisX,.18);
  }else if(name==='forage'||name==='drink'){
   moveWorld(bones[0],new T.Vector3(0,(name==='drink'?-.17:-.12)/unit,0));
   rotateWorld(bone('Bip01_Spine2'),axisX,.25);
   rotateWorld(bone('Bip01_Neck'),axisX,name==='drink'?.85:.70);
   rotateWorld(bone('Bip01_Head'),axisX,(name==='drink'?.25:.15)+.015*Math.sin(phase*Math.PI*4));
   rotateWorld(bone('Bone03'),axisX,(name==='forage'?.04:.015)*(.5+.5*Math.sin(phase*Math.PI*8)));
  }else moveWorld(bone('Bip01_Spine2'),new T.Vector3(0,.002*Math.sin(phase*Math.PI*2)/unit,0));
  for(const leg of legs){const goal=leg.position.clone();goal.y-=leg.soleHeight/unit;if(name==='rest'){goal.z+=(leg.name.startsWith('fore')?.35:.08)/unit;goal.x+=Math.sign(goal.x)*.02/unit;}ccd(leg,goal);}
  if(name==='rest')rotateWorld(bone('Bip01_Tail'),axisX,.55);
  rotateWorld(bone('Bip01_Tail1'),axisY,.04*Math.sin(phase*Math.PI*2));
 }
 if(['walk','hunt','run'].includes(name)){
  if(name==='hunt'){moveWorld(bone('Bip01_Pelvis'),new T.Vector3(0,-.12/unit,0));rotateWorld(bone('Bip01_Neck'),axisX,.10);}
  const stance=name==='run'?.36:name==='hunt'?.70:.62;
  const stride=name==='run'?.72:name==='hunt'?.16:.32;
  const offsets=name==='run'?[.32,.38,.84,.90]:[.25,.75,0,.5];
  for(let index=0;index<legs.length;index++){
   const leg=legs[index],p=(phase+offsets[index])%1;
   const swing=(p-stance)/(1-stance),advance=p<stance?stride*(.5-p/stance):stride*(-.5+swing);
   const lift=p<stance?0:(name==='run'?.20:name==='hunt'?.065:.08)*Math.sin(Math.PI*swing);
   const goal=leg.position.clone();goal.y+=(-leg.soleHeight+lift)/unit;goal.z+=advance/unit;ccd(leg,goal);
  }
 }
 target.gltf.scene.updateMatrixWorld(true);target.skin.skeleton.update();
 const soles=legs.map(leg=>{const points=leg.soles.map(index=>{const p=new T.Vector3();target.skin.getVertexPosition(index,p);return p.applyMatrix4(target.skin.matrixWorld).applyMatrix4(nt);});return {name:leg.name,minY:Math.min(...points.map(p=>p.y)),x:points.reduce((s,p)=>s+p.x,0)/points.length,z:points.reduce((s,p)=>s+p.z,0)/points.length};});
 return soles;
}
const size=target.raw.readUInt32LE(12),base=JSON.parse(target.raw.subarray(20,20+size)),binary=target.raw.subarray(28+size);
const objects=base.nodes.map((node,index)=>({index,obj:target.gltf.scene.getObjectByName(T.PropertyBinding.sanitizeNodeName(node.name??''))})).filter(item=>item.obj&&(item.obj.isBone||bones.some(b=>{for(let p=b.parent;p;p=p.parent)if(p===item.obj)return true;return false;})));
const configurations=[['idle',2,0],['walk',1.2,.32/.62],['run',sourceClip.duration,.72/.36],['hunt',1.4,.16/.70],['forage',2,0],['drink',2,0],['rest',3,0]];
const baked=[],metrics={};
for(const [name,duration,distance] of configurations){
 const frames=Math.max(17,Math.round(duration*60)+1),samples=[];
 for(let i=0;i<frames-1;i++){
  const phase=i/(frames-1),soles=sample(name,phase);
  samples.push({time:duration*phase,soles,transforms:objects.map(({index,obj})=>({index,translation:obj.position.toArray(),rotation:obj.quaternion.toArray(),scale:obj.scale.toArray()}))});
 }
 samples.push({...structuredClone(samples[0]),time:duration});
 const floor=name==='run'?-Math.min(...samples.flatMap(s=>s.soles.map(p=>p.minY))):0;
 for(const s of samples)s.floor=name==='run'?floor:-Math.min(...s.soles.map(p=>p.minY));
 let cycleDistance=distance;

 baked.push({name,duration,cycleDistance,samples});
 metrics[name]={duration,cycleDistance,frames,floorRange:[Math.min(...samples.map(s=>s.floor)),Math.max(...samples.map(s=>s.floor))],pawRegions:legs.map(l=>({name:l.name,vertices:l.soles.length}))};
}
await MeshoptSimplifier.ready;
mkdirSync(output,{recursive:true});const receipts=[];
for(const tier of ['near','mid','far']){
 const doc=structuredClone(base),chunks=[binary];let length=binary.length;
 function accessor(values,type,componentType=5126){
  const a=componentType===5125?new Uint32Array(values):componentType===5123?new Uint16Array(values):new Float32Array(values);const pad=(4-length%4)%4;if(pad){chunks.push(Buffer.alloc(pad));length+=pad;}
  const view=doc.bufferViews.length;doc.bufferViews.push({buffer:0,byteOffset:length,byteLength:a.byteLength});chunks.push(Buffer.from(a.buffer));length+=a.byteLength;
  const i=doc.accessors.length,dim={SCALAR:1,VEC3:3,VEC4:4}[type];doc.accessors.push({bufferView:view,componentType,count:values.length/dim,type,...(type==='SCALAR'?{min:[Math.min(...values)],max:[Math.max(...values)]}:{})});return i;
 }
 const weights=geometry.attributes.skinWeight,joints=geometry.attributes.skinIndex;
 doc.meshes[0].primitives[0].attributes.WEIGHTS_0=accessor(Array.from({length:weights.count*4},(_,i)=>weights.getComponent(Math.floor(i/4),i%4)),'VEC4');
 doc.meshes[0].primitives[0].attributes.JOINTS_0=accessor(Array.from({length:joints.count*4},(_,i)=>joints.getComponent(Math.floor(i/4),i%4)),'VEC4',5123);
 let triangles=geometry.index.count/3,simplificationError=0;
 if(tier!=='near'){
  const positions=Float32Array.from({length:geometry.attributes.position.count*3},(_,i)=>geometry.attributes.position.getComponent(Math.floor(i/3),i%3));
  const [indices,error]=MeshoptSimplifier.simplify(new Uint32Array(geometry.index.array),positions,3,(tier==='mid'?2600:1000)*3,tier==='mid'?.015:.1,[]);
  doc.meshes[0].primitives[0].indices=accessor(Array.from(indices),'SCALAR',5125);triangles=indices.length/3;simplificationError=error;
 }
 const scene=doc.scenes[doc.scene??0],normalNode=doc.nodes.length;doc.nodes.push({name:'LifeLensTigerSourceFrame',matrix:nt.toArray(),children:scene.nodes});
 const floorNode=doc.nodes.length;doc.nodes.push({name:'LifeLensTigerGroundOffset',children:[normalNode]});scene.nodes=[floorNode];
 doc.animations=[];
 for(const clip of baked){
  const animation={name:'tiger_'+clip.name,samplers:[],channels:[]};
  for(const {index} of objects){delete doc.nodes[index].matrix;const rest=clip.samples[0].transforms.find(t=>t.index===index);Object.assign(doc.nodes[index],{translation:rest.translation,rotation:rest.rotation,scale:rest.scale});
   for(const [path,type] of [['translation','VEC3'],['rotation','VEC4'],['scale','VEC3']]){
    const values=clip.samples.map(s=>s.transforms.find(t=>t.index===index)[path]);const constant=values.every(v=>v.every((x,j)=>Math.abs(x-values[0][j])<1e-6));
    const times=constant?[0,clip.duration]:clip.samples.map(s=>s.time),data=constant?[...values[0],...values[0]]:values.flat();
    const sampler=animation.samplers.length;animation.samplers.push({input:accessor(times,'SCALAR'),output:accessor(data,type),interpolation:'LINEAR'});animation.channels.push({sampler,target:{node:index,path}});
   }
  }
  const sampler=animation.samplers.length;animation.samplers.push({input:accessor(clip.samples.map(s=>s.time),'SCALAR'),output:accessor(clip.samples.flatMap(s=>[0,s.floor,0]),'VEC3'),interpolation:'LINEAR'});animation.channels.push({sampler,target:{node:floorNode,path:'translation'}});doc.animations.push(animation);
 }
 // Use the standing clip as the ordinary bind presentation, not the last baked rest.
 for(const t of baked[0].samples[0].transforms)Object.assign(doc.nodes[t.index],{translation:t.translation,rotation:t.rotation,scale:t.scale});
 for(const material of doc.materials){material.alphaMode='MASK';material.alphaCutoff=.35;material.pbrMetallicRoughness.metallicFactor=0;material.pbrMetallicRoughness.roughnessFactor=.9;}
 const contract={version:2,tier,units:'metres',forward:'+Z',up:'+Y',creator:'Amil (francescolima74)',license:'CC-BY-4.0',source_sha256:hashes[0],donor_sha256:hashes[1],status:inspection?'INSPECTION_ONLY':'DRAFT_REQUIRES_OBSERVER_AND_DEVICE_VALIDATION',bone_count:47,head_joint:bone('Bip01_Head').name,triangles,clips:Object.fromEntries(baked.map(c=>[c.name,{name:'tiger_'+c.name,duration:c.duration,cycle_distance:c.cycleDistance}])),paws:legs.map(l=>({name:l.paw.name,upper:l.chain.at(-1).name,lower:l.chain[0].name,sole_vertices:l.soles}))};
 doc.asset.copyright='Running Tiger by Amil (francescolima74), walk animation donor by Unity (shehab house), CC BY 4.0. LifeLens normalization, retargeting, behavioral poses and LOD adaptations.';
 doc.asset.extras={...(doc.asset.extras??{}),lifelensTiger:contract};doc.buffers[0].byteLength=length;
 let packed=Buffer.concat(chunks);packed=Buffer.concat([packed,Buffer.alloc((4-packed.length%4)%4)]);let json=Buffer.from(JSON.stringify(doc));json=Buffer.concat([json,Buffer.alloc((4-json.length%4)%4,32)]);
 const header=Buffer.alloc(20);header.write('glTF');header.writeUInt32LE(2,4);header.writeUInt32LE(28+json.length+packed.length,8);header.writeUInt32LE(json.length,12);header.writeUInt32LE(0x4e4f534a,16);const bh=Buffer.alloc(8);bh.writeUInt32LE(packed.length);bh.writeUInt32LE(0x004e4942,4);
 const bytes=Buffer.concat([header,json,bh,packed]),name=tier==='near'?'tiger-rigged.glb':`tiger-${tier}.glb`;writeFileSync(resolve(output,name),bytes);
 receipts.push({file:name,sha256:digest(bytes),bytes:bytes.length,triangles,simplificationError,contract});
}
writeFileSync(resolve(output,'native-authoring.json'),JSON.stringify({status:inspection?'INSPECTION_ONLY':'DRAFT_SOURCE_DERIVATIVE',runtimeAdmission:false,sourceHashes:hashes,uvTopologyHash:topology,mappedBones:38,preservedBones:47,correctedPawPadVertices:correctedPads.size,metrics,receipts},null,2)+'\n');
console.log(JSON.stringify({status:inspection?'INSPECTION_ONLY':'DRAFT_SOURCE_DERIVATIVE',output,metrics,files:receipts.map(r=>({file:r.file,bytes:r.bytes,triangles:r.triangles}))}));
const sources=ids.map((uid,index)=>{
 let receipt={};try{receipt=JSON.parse(readFileSync(resolve(inputRoot,uid,'receipt.json'),'utf8'));}catch{if(!inspection)throw Error('Canonical license/download receipt missing');}
 return {uid,sha256:hashes[index],sourceUrl:`https://sketchfab.com/3d-models/${uid}`,author:index===0?'Amil (francescolima74)':'Unity (shehab house)',license:'CC-BY-4.0',retrievedAt:receipt.retrievedAt??'LOCAL_INSPECTION_ONLY',downloadUrl:receipt.downloadUrl??null};
});
const credit=`# LifeLens native tiger sources\n\n${sources.map(s=>`- “${s.uid===ids[0]?'Running Tiger':'tiger walk v02'}” by ${s.author}. [Original](${s.sourceUrl}), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Retrieved ${s.retrievedAt}; SHA-256: ${s.sha256}. Public redistribution URL: ${s.downloadUrl??'inspection only'}.`).join('\n')}\n\nOriginal bytes and original per-object metadata remain in ../distributed-tigers/<uid>/.\nPublic redistribution: [AI2 Objaverse](https://huggingface.co/datasets/allenai/objaverse), [ODC-By 1.0](https://opendatacommons.org/licenses/by/1-0/). Dataset credit: Deitke et al., Objaverse: A Universe of Annotated 3D Objects, 2022, https://objaverse.allenai.org/.\n\nLifeLens adaptations: metric/forward normalization; original 47-joint skeleton and near topology/UV preservation; 38 mapped donor joints; native walking retarget; stance trajectories and sole-pad rig corrections (${correctedPads.size} vertices), seven behavioral clips, alpha-mask/roughness material correction, and bounded source-derived LODs. No primitive body, voxel remesh, paid provider, viewer scraping or authenticated download is used.\n\nStatus: ${inspection?'INSPECTION ONLY — reduced textures':'DRAFT — observer, visual and actual-device review required'}. This is not merge acceptance.\n`;
writeFileSync(resolve(output,'CREDITS.md'),credit);
const reportPath=resolve(output,'native-authoring.json');const report=JSON.parse(readFileSync(reportPath,'utf8'));report.sourcePins=sources;report.generatorSha256=digest(readFileSync(import.meta.filename));writeFileSync(reportPath,JSON.stringify(report,null,2)+'\n');
