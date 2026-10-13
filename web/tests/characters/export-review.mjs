// Offline inspection of the real pinned rig and clips; no simulation fixtures are shipped.
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import ts from 'typescript';
import { dirname, relative, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { clone } from 'three/addons/utils/SkeletonUtils.js';
const out=new URL('./review/',import.meta.url);mkdirSync(out,{recursive:true});
// Emit the actual transitive source dependency tree. Props now share the world
// presentation config, which also imports generated Core constants.
const sourceRoot=fileURLToPath(new URL('../../src/',import.meta.url));
const emitRoot=fileURLToPath(new URL('compiled/',out)), emitted=new Map();
function emit(path){
 if(emitted.has(path))return emitted.get(path);
 const target=resolve(emitRoot,relative(sourceRoot,path).replace(/\.ts$/,'.mjs'));
 emitted.set(path,target);
 let code=ts.transpileModule(readFileSync(path,'utf8'),{compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022}}).outputText;
 code=code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g,(_,prefix,id,suffix)=>{
  emit(resolve(dirname(path),id+'.ts'));return prefix+id+'.mjs'+suffix;
 });
 mkdirSync(dirname(target),{recursive:true});writeFileSync(target,code);return target;
}
const {createResidentAppearanceProfile,applyResidentMaterialVariant}=await import(pathToFileURL(emit(resolve(sourceRoot,'render/resident-appearance.ts'))));
const {ResidentInventoryProps}=await import(pathToFileURL(emit(resolve(sourceRoot,'render/resident-props.ts'))));
const {syncResidentAuthoredHair,syncResidentAuthoredFace}=await import(pathToFileURL(emit(resolve(sourceRoot,'render/resident-authored-appearance.ts'))));
const {syncResidentAuthoredGarment}=await import(pathToFileURL(emit(resolve(sourceRoot,'render/resident-authored-garment.ts'))));
const {syncResidentAnatomicalBody,ANATOMICAL_BODY_NAME}=await import(pathToFileURL(emit(resolve(sourceRoot,'render/resident-anatomical-body.ts'))));
const loader=new GLTFLoader();
async function load(file){const b=readFileSync(new URL('../../public/vendor/characters/'+file,import.meta.url));return loader.parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),'');}
const base=await load('character.glb'),ual1=await load('ual1.glb'),ual2=await load('ual2.glb');
const clips=[...ual1.animations,...ual2.animations];
base.scene.updateMatrixWorld(true);const box=new THREE.Box3().setFromObject(base.scene),size=box.getSize(new THREE.Vector3()),center=box.getCenter(new THREE.Vector3());
base.scene.position.sub(new THREE.Vector3(center.x,box.min.y,center.z));
const template=new THREE.Group();template.add(base.scene);template.scale.setScalar(1/size.y);
const result=[];
let maleBareVertices=null, maleAuthoredVertices=null;
const cases=[
 ['Male / low','Male',0,'Idle_Loop',.7, ['StoneHammer','SharpFlake']],
 ['Female / low','Female',0,'Idle_Loop',.7,['SimpleContainer','DiggingStick']],
 ['Male / high','Male',1,'Idle_Loop',.7,['BronzeAxe','RecordTablet']],
 ['Female / high','Female',1,'Idle_Loop',.7,['Cordage','SharpFlake']],
 ...['Farm_PlantSeed','Farm_Watering','Farm_Harvest','Walk_Carry_Loop',
      'Walk_Loop','Crouch_Idle_Loop','Sitting_Idle_Loop','Swim_Fwd_Loop']
      .flatMap(c=>[.2,.5,.8].map(t=>[c+' '+t,'Female',.5,c,t,['SimpleContainer','DiggingStick']])),
];
for(let ci=0;ci<cases.length;ci++){
 const [label,sex,axis,clipName,phase,items]=cases[ci];
 const profile=createResidentAppearanceProfile({id:'same-person',sex,ageYears:30,genetics:{heightPotential:axis,buildPotential:axis,faceShape:axis,skinTone:axis,hairPigment:axis,eyePigment:axis}});
 const model=clone(template);applyResidentMaterialVariant(model,profile);
 if(!syncResidentAnatomicalBody(model,profile))
   throw Error('Anatomical body generation on real pinned GLB failed: '+label);
 const authored=model.getObjectByName(ANATOMICAL_BODY_NAME);
 if(!(authored instanceof THREE.SkinnedMesh))
   throw Error('Authored anatomy is not a real skinned mesh: '+label);
 const sumVertices=authored.geometry.getAttribute('position').count;
 if(sumVertices<9000)throw Error('A6 anatomical shape too sparse: '+label);
 if(authored.geometry.userData.anatomyVersion!=='A6-A7')
   throw Error('Anatomy contract not updated: '+label);
 if(ci===0)maleAuthoredVertices=[...authored.geometry.getAttribute('position').array];
 if(ci===1){
   const female=[...authored.geometry.getAttribute('position').array];
   if(!maleAuthoredVertices || female.length!==maleAuthoredVertices.length)
     throw Error('Male/female anatomy vertex counts must match');
   let difference=0;for(let i=0;i<female.length;i++)
     difference+=Math.abs(female[i]-maleAuthoredVertices[i]);
   if(difference<.1)throw Error('Actual new anatomy does not distinguish sex morphology');
 }
 const digits=authored.geometry.getAttribute('skinIndex'),
       weights=authored.geometry.getAttribute('skinWeight');
 for(const fingerName of ['index_02_l','thumb_02_r','ring_03_r','ball_l']){
   const bone=authored.skeleton.bones.findIndex(b=>b.name===fingerName);
   if(bone<0)throw Error('Missing finger/toe bone '+fingerName);
   let articulation=0;
   for(let i=0;i<digits.count;i++)for(let k=0;k<4;k++)
     if(digits.getComponent(i,k)===bone&&weights.getComponent(i,k)>.02)articulation++;
   if(articulation<4)throw Error('Authoring failed to weight '+fingerName);
 }
 const authoredWeights=authored.geometry.getAttribute('skinWeight');
 for(let i=0;i<authoredWeights.count;i++) {
   const total=authoredWeights.getX(i)+authoredWeights.getY(i)
     +authoredWeights.getZ(i)+authoredWeights.getW(i);
   if(Math.abs(total-1)>1e-4)throw Error('Unnormalized anatomical bind weights: '+label);
 }
 if(model.getObjectByName('SuperHero_Male').material.visible!==false)
   throw Error('Legacy template body still rendering: '+label);
 const body=model.getObjectByName('SuperHero_Male');
 if(!(body instanceof THREE.SkinnedMesh))throw Error('A4 pinned skinned body unavailable');
 const barePositions=[...body.geometry.getAttribute('position').array];
 const skinColors=body.geometry.getAttribute('color');
 if(!skinColors)throw Error('A4 no per-vertex skin color');
 const skin=new THREE.Color(profile.skinColor);
 let skinCount=0;
 for(let v=0;v<skinColors.count;v++)
   if(Math.abs(skinColors.getX(v)-skin.r)<1e-5
     && Math.abs(skinColors.getY(v)-skin.g)<1e-5
     && Math.abs(skinColors.getZ(v)-skin.b)<1e-5)skinCount++;
 if(skinCount/skinColors.count<.72)
   throw Error('A4 fake base-clothes tint still present on pinned rig '+label);
 if(ci===0) maleBareVertices=barePositions;
 if(ci===1){
   if(!maleBareVertices || maleBareVertices.length!==barePositions.length)
     throw Error('A4 sex geometry count mismatch');
   let delta=0;for(let v=0;v<barePositions.length;v++)delta+=Math.abs(barePositions[v]-maleBareVertices[v]);
   if(delta<.04)throw Error('A4 male and female actual bare body geometry indistinguishable');
 }
 if(!syncResidentAuthoredHair(model,profile))throw Error('A2 real GLB Head mesh is unavailable');
 if(!syncResidentAuthoredFace(model,profile)){
  const objects=[];
  model.updateMatrixWorld(true);
  model.traverse(o=>{
    if(!o.isMesh && !/head|eye/i.test(o.name))return;
    const info={name:o.name,type:o.type,parent:o.parent?.name};
    if(o.isMesh){
      const local=new THREE.Box3().setFromObject(o);
      const cc=local.getCenter(new THREE.Vector3()), sz=local.getSize(new THREE.Vector3());
      info.center=cc.toArray().map(n=>+n.toFixed(5));
      info.size=sz.toArray().map(n=>+n.toFixed(5));
      info.skin=o.isSkinnedMesh;
    }
    objects.push(info);
  });
  const fit=model.userData.residentHeadFit,eye=model.getObjectByName('Eyes');
  const toHead=fit.bone.matrixWorld.clone().invert();
  const eyeBBox=new THREE.Box3().setFromObject(eye,true);
  const eyeLocal=eyeBBox.getCenter(new THREE.Vector3()).applyMatrix4(toHead);
  throw Error('A2 real GLB face landmark diagnostics '+JSON.stringify({
    headCenter:fit.center.toArray(),headRadii:fit.radii.toArray(),
    boneWorld:fit.bone.getWorldPosition(new THREE.Vector3()).toArray(),
    eyeLocal:eyeLocal.toArray(),eyeWorld:eyeBBox.getCenter(new THREE.Vector3()).toArray(),
    depth:eyeLocal.z-fit.center.z,
    eyeHeight:eyeLocal.y-fit.center.y,
    objects:objects.slice(0,6)
  }));
 }
 // Preview genuine initial state with no owned garment, plus optional crafted item.
 const crafted=axis>=1;
 const worn=crafted?{item:'PrimitiveWrap',material:'Fiber',quantity:1,quality:.72,durability:.88}:null;
 if(syncResidentAuthoredGarment(model,worn,profile)!==crafted)
   throw Error('A3 garment existence did not match Core equipment for '+label);
 if(Boolean(model.getObjectByName('LifeLensResidentAuthoredGarment'))!==crafted)
   throw Error('A4 fake/absent clothing in pinned rig '+label);
  const visual=new THREE.Group();visual.add(model);const root=new THREE.Group();root.add(visual);
 root.scale.set(profile.heightWorldUnits*profile.widthScale,profile.heightWorldUnits,profile.heightWorldUnits*profile.depthScale);
 const props=new ResidentInventoryProps(visual,model);props.setInventory([...items.map(item=>({item,quantity:1})),{item:'RawMaterial',material:clipName==='Farm_Watering'?'Water':'Wood',quantity:2}],{active:true,kind:'Civilization',phase:clipName==='Walk_Carry_Loop'?'Moving':'Interacting',facilityKind:'CultivatedPlot',facilityAction:clipName==='Farm_Watering'?'Water':clipName==='Walk_Carry_Loop'?'DeliverMaterial':'Plant',civilizationMaterial:'Wood'});
 const mixer=new THREE.AnimationMixer(root);const clip=clips.find(c=>c.name===clipName);if(!clip)throw Error(clipName);mixer.clipAction(clip).play();mixer.setTime(clip.duration*phase);
 root.updateMatrixWorld(true);props.update();root.updateMatrixWorld(true);
 const meshes=[];root.traverse(mesh=>{
  if(!mesh.isMesh)return;if(mesh.isSkinnedMesh)mesh.skeleton.update();
  const pos=mesh.geometry.getAttribute('position'),color=mesh.geometry.getAttribute('color'),v=new THREE.Vector3(),positions=[],colors=[];
  const material=Array.isArray(mesh.material)?mesh.material[0]:mesh.material;
  for(let i=0;i<pos.count;i++){
   v.fromBufferAttribute(pos,i);if(mesh.isSkinnedMesh)mesh.applyBoneTransform(i,v);v.applyMatrix4(mesh.matrixWorld);positions.push(v.x,v.y,v.z);
   const c=material.color.clone();if(color && material.vertexColors)c.multiply(new THREE.Color().setRGB(color.getX(i),color.getY(i),color.getZ(i)));colors.push(...c.toArray());
  }
  if(!positions.every(Number.isFinite))throw Error('nonfinite '+label);
  meshes.push({positions,colors,indices:mesh.geometry.index?[...mesh.geometry.index.array]:Array.from({length:pos.count},(_,i)=>i)});
 });
 result.push({label,meshes});
}
writeFileSync(new URL('poses.json',out),JSON.stringify(result));console.log('Verified real rig/clip, face and garment attachments for',result.length,'poses');
