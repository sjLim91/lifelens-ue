// Offline inspection of the real pinned rig and clips; no simulation fixtures are shipped.
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import ts from 'typescript';
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { clone } from 'three/addons/utils/SkeletonUtils.js';
const out=new URL('./review/',import.meta.url);mkdirSync(out,{recursive:true});
for(const name of ['resident-appearance','resident-props'])writeFileSync(new URL(name+'.mjs',out),ts.transpileModule(readFileSync(new URL('../../src/render/'+name+'.ts',import.meta.url),'utf8'),{compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022}}).outputText);
const {createResidentAppearanceProfile,applyResidentMaterialVariant}=await import(new URL('resident-appearance.mjs',out));
const {ResidentInventoryProps}=await import(new URL('resident-props.mjs',out));
const loader=new GLTFLoader();
async function load(file){const b=readFileSync(new URL('../../public/vendor/characters/'+file,import.meta.url));return loader.parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),'');}
const base=await load('character.glb'),ual1=await load('ual1.glb'),ual2=await load('ual2.glb');
const clips=[...ual1.animations,...ual2.animations];
base.scene.updateMatrixWorld(true);const box=new THREE.Box3().setFromObject(base.scene),size=box.getSize(new THREE.Vector3()),center=box.getCenter(new THREE.Vector3());
base.scene.position.sub(new THREE.Vector3(center.x,box.min.y,center.z));
const template=new THREE.Group();template.add(base.scene);template.scale.setScalar(1/size.y);
const result=[];
const cases=[
 ['Male / low','Male',0,'Idle_Loop',.7, ['StoneHammer','SharpFlake']],
 ['Female / low','Female',0,'Idle_Loop',.7,['SimpleContainer','DiggingStick']],
 ['Male / high','Male',1,'Idle_Loop',.7,['BronzeAxe','RecordTablet']],
 ['Female / high','Female',1,'Idle_Loop',.7,['Cordage','SharpFlake']],
 ...['Farm_PlantSeed','Farm_Watering','Farm_Harvest','Walk_Carry_Loop'].flatMap(c=>[.2,.5,.8].map(t=>[c+' '+t,'Female',.5,c,t,['SimpleContainer','DiggingStick']])),
];
for(let ci=0;ci<cases.length;ci++){
 const [label,sex,axis,clipName,phase,items]=cases[ci];
 const profile=createResidentAppearanceProfile({id:'same-person',sex,ageYears:30,genetics:{heightPotential:axis,buildPotential:axis,faceShape:axis,skinTone:axis,hairPigment:axis,eyePigment:axis}});
 const model=clone(template);applyResidentMaterialVariant(model,profile);
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
writeFileSync(new URL('poses.json',out),JSON.stringify(result));console.log('Verified real rig/clip skinning for',result.length,'poses');
