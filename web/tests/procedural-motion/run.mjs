import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {pathToFileURL} from 'node:url';
import ts from 'typescript';
import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {clone} from 'three/addons/utils/SkeletonUtils.js';
const root=resolve(import.meta.dirname,'../..'),out=resolve(import.meta.dirname,'review'),compiled=new Set();
function compile(path){
 const target=resolve(out,path.replace(/\.ts$/,'.mjs'));if(compiled.has(path))return target;compiled.add(path);
 let code=ts.transpileModule(readFileSync(resolve(root,'src',path),'utf8'),{compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022}}).outputText;
 code=code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g,(_,a,p,b)=>{compile(resolve(dirname(path),p+'.ts').slice(resolve('.').length+1));return a+p+'.mjs'+b;});
 code=code.replaceAll('import.meta.env.BASE_URL',"'/'");mkdirSync(dirname(target),{recursive:true});writeFileSync(target,code);return target;
}
const source=async p=>import(pathToFileURL(compile(p)));
const {ResidentRig,ResidentTwoBoneIK}=await source('render/resident-rig.ts');
const {createResidentAuthoredMotions}=await source('render/resident-authored-motion.ts');
const {ResidentProceduralMotion,residentMotionStyle}=await source('render/resident-procedural-motion.ts');
const {ResidentGroundSurface}=await source('render/resident-grounding.ts');
const {ResidentPhysicalTargets}=await source('render/resident-interaction-context.ts');
const {ResidentWorldLayer}=await source('render/resident-world-layer.ts');
const {createResidentAppearanceProfile,applyResidentMaterialVariant}=
  await source('render/resident-appearance.ts');
const {syncResidentAnatomicalBody,ANATOMICAL_BODY_NAME}=
  await source('render/resident-anatomical-body.ts');
const {buildChunkGeometry}=await source('render/terrain-geometry.ts');
const bytes=readFileSync(resolve(root,'public/vendor/characters/character.glb'));
const asset=await new GLTFLoader().parseAsync(bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength),'');
const model=asset.scene;model.updateMatrixWorld(true);
const box=new THREE.Box3().setFromObject(model),center=box.getCenter(new THREE.Vector3()),height=box.max.y-box.min.y;
model.position.set(-center.x,-box.min.y,-center.z);
const normalized=new THREE.Group();normalized.add(model);normalized.scale.setScalar(1/height);
let passed=0;function test(name,fn){fn();passed++;console.log('PASS',name);}
const flat={available:true,centerChunkX:0,centerChunkY:0,chunks:[{x:0,y:0,elevation01:0,waterKind:'None',waterAvailability:0}]};
const ground=new ResidentGroundSurface();ground.setTerrain(flat,0,0);
test('actual production GLB has required skin and all 18 semantic joints',()=>{
 const rig=new ResidentRig(normalized);assert(rig.skinned);assert.deepEqual(rig.missing,[]);
 assert.equal(rig.joints.size,18);console.log('model meshes',asset.parser.json.meshes.length,'skins',asset.parser.json.skins.length,'base clips',asset.animations.length);
});
test('A6/A7 authored male/female anatomy and articulated digits reproduce from bind pose during animation',()=>{
  const individuals=[];
  for(const sex of ['Male','Female']){
    const scene=clone(normalized);
    const phenotype=createResidentAppearanceProfile({id:'a6-a7-repeat',name:'test',sex,ageYears:30,lifeStage:'Adult',
      genetics:{heightPotential:.5,buildPotential:.5,faceShape:.5,skinTone:.5}});
    applyResidentMaterialVariant(scene,phenotype);
    assert(syncResidentAnatomicalBody(scene,phenotype));
    const anatomy=scene.getObjectByName(ANATOMICAL_BODY_NAME);
    assert(anatomy instanceof THREE.SkinnedMesh);
    assert.equal(anatomy.geometry.userData.anatomyVersion,'A6-A7');
    const geometry=anatomy.geometry,positions=geometry.getAttribute('position');
    const skinIndices=geometry.getAttribute('skinIndex'),skinWeights=geometry.getAttribute('skinWeight');
    const initial=[...positions.array];
    assert(positions.count>=9000,'fully generated face, torso, hands and toes');
    for(let i=0;i<positions.count;i++){
      assert(Number.isFinite(positions.getX(i)+positions.getY(i)+positions.getZ(i)));
      let total=0;
      for(let k=0;k<4;k++){
        const bone=skinIndices.getComponent(i,k);
        assert(Number.isInteger(bone)&&bone>=0&&bone<anatomy.skeleton.bones.length);
        total+=skinWeights.getComponent(i,k);
      }
      assert(Math.abs(total-1)<2e-4);
    }
    for(const name of ['index_02_l','middle_03_r','thumb_01_l','ball_l']){
      const bone=anatomy.skeleton.bones.findIndex(b=>b.name===name);
      assert(bone>=0,'missing rig '+name);
      let used=0;
      for(let i=0;i<skinIndices.count;i++)for(let k=0;k<4;k++)
        if(skinIndices.getComponent(i,k)===bone&&skinWeights.getComponent(i,k)>.02)used++;
      assert(used>3,'authored body lacks flexion on '+name);
    }
    for(const side of ['left','right'])
      assert(geometry.userData.residentFootSole[side]>=.014
        && geometry.userData.residentFootSole[side]<=.095);
    // Recreate geometry while moving: underlying boneInverse rest coordinates
    // must not stamp the middle of a walk into permanent body proportions.
    const clip=createResidentAuthoredMotions(scene).get('walk');
    const mixer=new THREE.AnimationMixer(scene);
    mixer.clipAction(clip).play();mixer.setTime(clip.duration*.33);
    scene.updateMatrixWorld(true);
    assert(syncResidentAnatomicalBody(scene,phenotype));
    const after=scene.getObjectByName(ANATOMICAL_BODY_NAME);
    assert.deepEqual([...after.geometry.getAttribute('position').array],initial);
    individuals.push(initial);
  }
  assert.equal(individuals[0].length,individuals[1].length);
  let difference=0;
  for(let i=0;i<individuals[0].length;i++)
    difference+=Math.abs(individuals[0][i]-individuals[1][i]);
  assert(difference>.1,'sex-dependent body geometry must actually differ');
});
test('authored library works without vendor clips and preserves finite loop endpoints',()=>{
 const library=createResidentAuthoredMotions(clone(normalized));assert(library.size>=29);
 for(const clip of library.values()){assert(clip.duration>0);for(const t of clip.tracks){assert([...t.values].every(Number.isFinite));const n=t.getValueSize();for(let j=0;j<n;j++)assert(Math.abs(t.values[j]-t.values[t.values.length-n+j])<1e-5);}}
 assert.notDeepEqual([...library.get('idle').tracks.find(t=>t.name==='thigh_l.quaternion').values],[...library.get('walk').tracks.find(t=>t.name==='thigh_l.quaternion').values]);
});
test('surface sampler matches rendered triangles across curved patches and rebasing',()=>{
 const terrain={...flat,chunks:[]};for(let y=-1;y<=1;y++)for(let x=-1;x<=1;x++)terrain.chunks.push({x,y,elevation01:.15+.03*x+.02*y+.08*x*y,waterKind:'None',waterAvailability:0});
 ground.setTerrain(terrain,0,0);const geometry=buildChunkGeometry(terrain.chunks.find(c=>c.x===0&&c.y===0),terrain),mesh=new THREE.Mesh(geometry,new THREE.MeshBasicMaterial({side:THREE.DoubleSide}));mesh.updateMatrixWorld(true);
 const ray=new THREE.Raycaster();for(const [x,z] of [[-.63,.47],[1.24,-2.32],[-3.17,2.08],[.02,.09]]){ray.set(new THREE.Vector3(x,100,z),new THREE.Vector3(0,-1,0));const hit=ray.intersectObject(mesh)[0];assert(hit);assert(Math.abs(ground.height(x,z)-hit.point.y)<2e-6);}
 const h=ground.height(.3,.4);ground.setTerrain(terrain,2,-1);assert(Math.abs(ground.height(.3-16,.4+8)-h)<1e-8);assert.equal(ground.height(1e4,1e4),null);geometry.dispose();mesh.material.dispose();ground.setTerrain(flat,0,0);
});
test('two-bone solver reaches, clamps and rejects malformed targets without scaling',()=>{
 const root=new THREE.Group(),a=new THREE.Bone(),b=new THREE.Bone(),c=new THREE.Bone();root.add(a);a.add(b);b.add(c);b.position.y=-1;c.position.y=-1;root.updateMatrixWorld(true);
 const ik=new ResidentTwoBoneIK(),target=new THREE.Vector3(.6,-1.6,.2);assert(ik.solve(a,b,c,target,1));assert(c.getWorldPosition(new THREE.Vector3()).distanceTo(target)<1e-5);
 assert(ik.solve(a,b,c,new THREE.Vector3(10,0,0),1));assert.equal(b.position.y,-1);assert.equal(c.scale.x,1);assert(!ik.solve(a,b,c,new THREE.Vector3(NaN,0,0),1));
});
test('physical target validation denies absent water, stale objects and remote gestures',()=>{
 const index=new ResidentPhysicalTargets(),r={id:'r',civilization:{inventory:[]}},p={active:true,kind:'Physical',phase:'Interacting',hasTargetGrid:true,targetGridX:16,targetGridY:16,physicalGoal:'Drink',directNaturalWaterSource:true};
 index.setTerrain(flat);assert(!index.allows(r,p,0,0,0,0));index.setTerrain({...flat,chunks:[{...flat.chunks[0],waterKind:'Spring',salinity:'Fresh',waterAvailability:.8}]});assert(index.allows(r,p,0,0,0,0));assert(!index.allows(r,p,2,0,0,0));
 assert(!index.allows(r,{...p,directNaturalWaterSource:false},0,0,0,0));const inventory={id:'r',civilization:{inventory:[{item:'RawMaterial',material:'Water',quantity:1}]}};assert(index.allows(inventory,{...p,directNaturalWaterSource:false},0,0,0,0));
 assert(!index.allows(r,{...p,hasObjectTarget:true,objectId:'missing',objectKind:'Sink'},0,0,0,0));
 index.setObjects({available:true,smartObjects:[{id:'sink',kind:'Sink',gridX:16,gridY:16,reservedById:'r'}]});assert(index.allows(r,{...p,hasObjectTarget:true,objectId:'sink',objectKind:'Sink'},0,0,0,0));
});
test('state style uses actual bounded facts without mutating the resident',()=>{
 const r={id:'r',needs:{sleep:.8},health:{injurySeverity:.5},emotion:{fear:.9,joy:.7},personality:{introversion:.3}};const original=JSON.stringify(r),s=residentMotionStyle(r);assert.equal(s.fatigue,.8);assert.equal(s.injury,.5);assert.equal(s.fear,.9);assert.equal(JSON.stringify(r),original);assert.equal(residentMotionStyle({id:'r'}).injury,0);
});
test('procedural overlays restore the exact mixer pose and do not accumulate on pause',()=>{
 const root=new THREE.Group(),m=clone(normalized);root.add(m);root.scale.setScalar(1.7);const proc=new ResidentProceduralMotion(root,m);proc.setFacts({id:'r',needs:{sleep:.8},emotion:{fear:.8}});
 const poses=new Map([...proc.rig.joints].map(([name,o])=>[name,o.quaternion.clone()]));
 const frame={dt:1/60,moving:false,sleeping:false,detailed:true,speed:0,ground,handTarget:null,gazeTarget:new THREE.Vector3(1,1,2)};
 for(let i=0;i<100;i++){proc.afterMotion(frame);proc.beforeMotion();for(const [name,o] of proc.rig.joints)assert(o.quaternion.equals(poses.get(name)));}
 proc.afterMotion({...frame,dt:0});const head=proc.rig.joints.get('head').quaternion.clone();proc.beforeMotion();proc.afterMotion({...frame,dt:0});assert(head.angleTo(proc.rig.joints.get('head').quaternion)<1e-7);proc.beforeMotion();
});
// Deliberately unavailable vendor libraries: exercise the actual runtime fallback.
GLTFLoader.prototype.loadAsync=async url=>{if(url.includes('character.glb'))return asset;throw Error('test: external animation unavailable');};
const layer=new ResidentWorldLayer();for(let i=0;i<30&&!layer.ready;i++)await new Promise(r=>setImmediate(r));assert(layer.ready);
const resident=(id,extra={})=>({id,name:id,hasPosition:true,gridX:16,gridY:16,ageYears:30,...extra});
const review=[];
function bake(actor,label){actor.root.updateMatrixWorld(true);const meshes=[],point=new THREE.Vector3();actor.model.traverse(mesh=>{if(!mesh.isMesh)return;if(mesh.isSkinnedMesh)mesh.skeleton.update();const p=mesh.geometry.getAttribute('position'),colors=mesh.geometry.getAttribute('color'),positions=[],color=[];for(let i=0;i<p.count;i++){point.fromBufferAttribute(p,i);if(mesh.isSkinnedMesh)mesh.applyBoneTransform(i,point);point.applyMatrix4(mesh.matrixWorld);positions.push(point.x,point.y,point.z);color.push(...(colors?[colors.getX(i),colors.getY(i),colors.getZ(i)]:[.6,.5,.4]));}meshes.push({positions,colors:color,indices:mesh.geometry.index?[...mesh.geometry.index.array]:Array.from({length:p.count},(_,i)=>i)});});review.push({label,origin:actor.current.toArray(),meshes});}
test('real renderer uses original walking and faces current displacement on reversal',()=>{
 layer.setResidents([resident('a')],flat,0,0);let a=layer.actors.get('a');assert(a.actions.get('walk').getClip().name.startsWith('LifeLens_Authored'));
 layer.setResidents([resident('a',{gridY:26})],flat,0,0);for(let i=0;i<25;i++)layer.update(1/60);assert(a.current.z>0);assert.equal(a.active,'walk');bake(a,'authored walking');
 layer.setResidents([resident('a',{gridY:8})],flat,0,0);const z=a.current.z;layer.update(1/60);assert(Math.abs(a.current.z-z)<1e-8);for(let i=0;i<35;i++)layer.update(1/60);assert(a.current.z<z);
});
test('no-water physical gesture is suppressed in the actual actor',()=>{
 layer.setResidents([resident('a',{presentation:{active:true,kind:'Physical',phase:'Interacting',physicalGoal:'Drink',hasTargetGrid:true,targetGridX:16,targetGridY:16,directNaturalWaterSource:true}})],flat,0,0);for(let i=0;i<120;i++)layer.update(1/60);assert.equal(layer.actors.get('a').active,'idle');bake(layer.actors.get('a'),'no water neutral');
});
test('moving root samples terrain at its displayed position on every frame',()=>{
 const terrain={...flat,chunks:[]};for(let y=-1;y<=1;y++)for(let x=-1;x<=1;x++)terrain.chunks.push({x,y,elevation01:.05+.008*x+.012*y+.008*x*y,waterKind:'None',waterAvailability:0});
 layer.setResidents([resident('slope')],terrain,0,0);layer.setResidents([resident('slope',{gridX:26,gridY:25})],terrain,0,0);const a=layer.actors.get('slope');const surface=new ResidentGroundSurface();surface.setTerrain(terrain,0,0);
 for(let i=0;i<45;i++){layer.update(1/60);assert(Math.abs(a.current.y-surface.height(a.current.x,a.current.z))<1e-8);}bake(a,'slope grounding');
});
test('core-confirmed drink and sleep keep their motion without inventing a resource',()=>{
 const terrain={...flat,chunks:[{...flat.chunks[0],waterKind:'Spring',salinity:'Fresh',waterAvailability:.8}]};const drink={active:true,kind:'Physical',phase:'Interacting',physicalGoal:'Drink',hasTargetGrid:true,targetGridX:16,targetGridY:16,directNaturalWaterSource:true};
 layer.setResidents([resident('drink',{presentation:drink})],terrain,0,0);for(let i=0;i<30;i++)layer.update(1/60);assert.equal(layer.actors.get('drink').active,'consume');bake(layer.actors.get('drink'),'authored drinking');
 const sleep={...drink,physicalGoal:'Sleep',directNaturalWaterSource:false};layer.setResidents([resident('sleep',{presentation:sleep})],flat,0,0);for(let i=0;i<60;i++)layer.update(1/60);assert.equal(layer.actors.get('sleep').active,'sleep');bake(layer.actors.get('sleep'),'existing sleep');
});
test('core input facts remain byte-identical through 300 actors, LOD and pause',()=>{
 const rs=Array.from({length:300},(_,i)=>resident(String(i),{gridX:16+i%20,gridY:16+Math.floor(i/20),emotion:{fear:.4}})),before=JSON.stringify(rs);layer.setResidents(rs,flat,0,0);
 const camera=new THREE.PerspectiveCamera(42,1,.1,5000);camera.position.set(100,100,100);camera.lookAt(0,0,0);camera.updateMatrixWorld(true);
 const start=performance.now();for(let i=0;i<60;i++)layer.update(1/60,camera);console.log('300 actors 60 frames CPU ms',Math.round(performance.now()-start));
 assert.equal(JSON.stringify(rs),before);const actor=layer.actors.get('0'),pos=actor.current.clone();layer.setSimulationSpeed(0);for(let i=0;i<10;i++)layer.update(1/60,camera);assert(actor.current.equals(pos));
 layer.setSimulationSpeed(1);layer.setResidents([resident('0')],flat,0,0);assert(layer.actors.size<=33);layer.dispose();layer.dispose();
});
mkdirSync(out,{recursive:true});writeFileSync(resolve(out,'poses.json'),JSON.stringify(review));
console.log(`${passed} procedural motion checks passed`);
