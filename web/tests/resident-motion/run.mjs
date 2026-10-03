import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {pathToFileURL} from 'node:url';
import ts from 'typescript';
import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
const root=resolve(import.meta.dirname,'../..'), out=resolve(import.meta.dirname,'review');
const compiled=new Set();
function compile(path){
 const target=resolve(out,path.replace(/\.ts$/,'.mjs'));if(compiled.has(path))return target;compiled.add(path);
 let code=ts.transpileModule(readFileSync(resolve(root,'src',path),'utf8'),{compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022}}).outputText;
 code=code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g,(_,a,p,b)=>{const dependency=resolve(dirname(path),p+'.ts');const relative=dependency.slice(resolve('.').length+1);compile(relative);return a+p+'.mjs'+b;});
 code=code.replaceAll('import.meta.env.BASE_URL',"'/'");mkdirSync(dirname(target),{recursive:true});writeFileSync(target,code);return target;
}
const source=async path=>import(pathToFileURL(compile(path)));
const {resolveResidentSemanticMotion:motion}=await source('render/resident-semantic-motion.ts');
const {createResidentMotionLibrary}=await source('render/resident-motion-library.ts');
const {ResidentWorldLayer}=await source('render/resident-world-layer.ts');
const loader=new GLTFLoader(),assets=new Map();
for(const name of ['character','ual1','ual2']){const b=readFileSync(resolve(root,'public/vendor/characters/'+name+'.glb'));assets.set(name,await loader.parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),''));}
let loads=0,failUAL2=false,failBoth=false;
GLTFLoader.prototype.loadAsync=async function(url){loads++;const name=url.includes('character.glb')?'character':url.includes('ual2')||url.includes('UAL2')?'ual2':'ual1';if(name!=='character'&&(failBoth||name==='ual2'&&failUAL2))throw Error('expected asset failure');return assets.get(name);};
let passed=0;function test(name,fn){fn();passed++;console.log('PASS',name);}
const p=(extra={})=>({active:true,kind:'Civilization',phase:'Interacting',hasTargetGrid:true,targetGridX:64,targetGridY:64,...extra});
const c={moving:false,nearbyResident:true,hasWaterContainer:true};
test('complete deterministic semantic matrix and pre-arrival guard',()=>{
 const rows=[
 ...['Stone','Flint','CopperOre','TinOre'].map(m=>[p({civilizationIntent:'Gather',civilizationMaterial:m}),'gatherMineral']),
 ...[['Wood','gatherWood'],['Clay','gatherClay'],['Fiber','gatherFiber'],['PlantFood','harvest']].map(([m,k])=>[p({civilizationIntent:'Gather',civilizationMaterial:m}),k]),
 ...[['Work','construct'],['Repair','repair'],['Fuel','fuel'],['Ignite','ignite'],['LoadSmeltCharge','loadFurnace'],['CollectCharcoal','collect'],['CollectMetal','collect'],['DeliverMaterial','store']].map(([a,k])=>[p({facilityAction:a}),k]),
 ...['Craft','Experiment','Store','Retrieve'].map(a=>[p({civilizationIntent:a}),a.toLowerCase()]),
 ...[['Plant','plant'],['Water','water'],['Tend','tend'],['Harvest','harvest']].map(([a,k])=>[p({facilityKind:'CultivatedPlot',facilityAction:a}),k]),
 ...['Eat','Drink'].map(a=>[p({kind:'Physical',physicalGoal:a}),'consume']),
 [p({kind:'Physical',physicalGoal:'Sleep'}),'sleep'],[p({kind:'Physical',physicalGoal:'Wash',directNaturalWaterSource:true}),'wash'],
 [p({kind:'Social',socialIntent:'Comfort'}),'comfort'],[p({kind:'Social',socialIntent:'Repair'}),'reconcile'],[p({kind:'Social'}),'talk'],
 [p({kind:'KnowledgeTeaching'}),'teach'],[p({kind:'Parenting',parentingAction:'Educate'}),'teach'],[p({kind:'Parenting',parentingAction:'Feed'}),'care'],
 ];
 for(const [d,want] of rows){for(let i=0;i<5;i++)assert.equal(motion(d,c),want);assert.equal(motion(d,{...c,moving:true}),'walk');assert.equal(motion({...d,phase:'Moving'},c),'walk');}
 assert.equal(motion(p({facilityAction:'DeliverMaterial'}),{...c,moving:true,hasCarriedLoad:true}),'carry');
 assert.equal(motion(p({kind:'Social'}),{...c,nearbyResident:false}),'idle');
});
test('fallback library uses exact verified safe names and bounded shared clips',()=>{
 const library=createResidentMotionLibrary(assets.get('ual1').animations);assert.equal(library.get('walk').name,'Walk_Loop');assert.equal(library.get('carry').name,'Walk_Loop');
 assert.equal(library.get('harvest').name,'Fixing_Kneeling');assert(![...library.values()].some(clip=>/Sword|Punch|Death/.test(clip.name)));
 assert.equal(createResidentMotionLibrary([]).size,0);
});
const terrain={available:true,worldSeed:'motion',centerChunkX:0,centerChunkY:0,chunks:[{x:0,y:0,elevation01:0}],humanTraces:{entries:[]}};
const resident=(id='a',presentation=p({kind:'Physical',physicalGoal:'Sleep'}),extra={})=>({id,hasPosition:true,gridX:64,gridY:64,sex:'Female',ageYears:30,presentation,...extra});
async function layer(){const l=new ResidentWorldLayer();for(let i=0;i<10&&!l.ready;i++)await new Promise(r=>setImmediate(r));assert(l.ready);return l;}
function bodyBounds(actor){actor.root.updateMatrixWorld(true);const b=new THREE.Box3(),v=new THREE.Vector3();actor.model.traverse(mesh=>{if(!mesh.isMesh)return;if(mesh.isSkinnedMesh)mesh.skeleton.update();const positions=mesh.geometry.getAttribute('position');for(let i=0;i<positions.count;i++){v.fromBufferAttribute(positions,i);if(mesh.isSkinnedMesh)mesh.applyBoneTransform(i,v);v.applyMatrix4(mesh.matrixWorld);b.expandByPoint(v);}});return b;}
const review=[];
function bake(actor,label){
 actor.root.updateMatrixWorld(true);const meshes=[],point=new THREE.Vector3();
 actor.model.traverse(mesh=>{if(!mesh.isMesh)return;if(mesh.isSkinnedMesh)mesh.skeleton.update();const p=mesh.geometry.getAttribute('position'),color=mesh.geometry.getAttribute('color'),positions=[],colors=[],mat=Array.isArray(mesh.material)?mesh.material[0]:mesh.material;
  for(let i=0;i<p.count;i++){point.fromBufferAttribute(p,i);if(mesh.isSkinnedMesh)mesh.applyBoneTransform(i,point);point.applyMatrix4(mesh.matrixWorld);positions.push(point.z-actor.current.z,point.y-actor.current.y,-point.x+actor.current.x);const c=mat.color.clone();if(color&&mat.vertexColors)c.multiply(new THREE.Color(color.getX(i),color.getY(i),color.getZ(i)));colors.push(...c.toArray());}
  meshes.push({positions,colors,indices:mesh.geometry.index?[...mesh.geometry.index.array]:Array.from({length:p.count},(_,i)=>i)});
 });review.push({label,meshes});
}
const l=await layer();
for(const [name,support,window] of [['ground',0,terrain],['bedding',.14,{...terrain,humanTraces:{entries:[{id:'bed',kind:'Facility',facilityKind:'SleepingPlace',state:'Operational',gridX:64,gridY:64}]}}]]){
 test('native sleep enter/rest/wake stays above '+name,()=>{
  const r=resident(name);l.setResidents([r],window,0,0);const a=l.actors.get(name),initial=a.current.clone();
  for(let i=0;i<100;i++){l.update(1/60);assert(bodyBounds(a).min.y>=support-.005);assert.equal(a.active,'sleep');assert.equal(a.visual.rotation.z,0);if(i===5||i===99)bake(a,name+' sleep '+i);}
  assert(a.sleepMotion.resting);const stable=bodyBounds(a).clone(),yaw=a.root.rotation.y;
  for(let i=0;i<80;i++)l.update(1/60);assert(bodyBounds(a).equals(stable));assert.equal(a.root.rotation.y,yaw);assert(a.current.equals(initial));
  l.setResidents([resident(name,p({kind:'Physical',physicalGoal:'Eat'}))],terrain,0,0);
  let previous=a.visual.position.clone();for(let i=0;i<110;i++){l.update(1/60);assert(bodyBounds(a).min.y >= (a.sleepMotion.active ? -.005 : -.025));assert(a.visual.position.distanceTo(previous)<.3);previous.copy(a.visual.position);}
  assert(!a.sleepMotion.active);assert.equal(a.active,'consume');assert.equal(a.visual.position.length(),0);
 });
}
test('sleep clearance survives phenotype/age extremes and sloping ground/bedding support',()=>{
 const slope={...terrain,chunks:[{x:0,y:0,elevation01:.01},{x:1,y:0,elevation01:.02},{x:0,y:1,elevation01:.015}],humanTraces:{entries:[]}};
 for(const [sex,ageYears,axis] of [['Male',30,1],['Female',30,0],['Female',4,1],['Male',85,0]]){
  for (const bedding of [false,true]) {
  const window={...slope,humanTraces:{entries:bedding ? [{id:'slope-mat',kind:'Facility',facilityKind:'SleepingPlace',state:'Operational',gridX:64,gridY:64}] : []}};
 const r=resident('slope-'+sex+ageYears+bedding,undefined,{sex,ageYears,genetics:{heightPotential:axis,buildPotential:axis,faceShape:axis}});
  l.setResidents([r],window,0,0);const a=l.actors.get(r.id);for(let i=0;i<70;i++)l.update(1/60);
  assert(a.sleepMotion.resting);assert(bodyBounds(a).min.y>=a.current.y+a.sleepSupportHeightWorldUnits-.005);
  const before=bodyBounds(a).clone();l.setResidents([r],window,0,0);l.update(1/60);assert(bodyBounds(a).equals(before));
 }
 }
});
test('resource facing uses Core node center without moving from access cell',()=>{
 l.setCivilization({available:true,resources:[{id:'wood',gridX:80,gridY:64}]});
 l.setResidents([resident('face',p({civilizationIntent:'Gather',civilizationMaterial:'Wood',civilizationResourceNode:'wood'}))],terrain,0,0);const a=l.actors.get('face'),before=a.current.clone();
 for(let i=0;i<30;i++)l.update(1/60);assert(Math.abs(a.root.rotation.y-Math.PI/2)<.01);assert(a.current.equals(before));
 a.presentation=p({hasTargetGrid:false});assert.equal(l.interactionTargetYaw(a),null);
});
test('visible arrival guards interactions and preserves walk grace',()=>{
 const r=resident('arrival',p({kind:'Physical',physicalGoal:'Eat'}));l.setResidents([r],terrain,0,0);l.setResidents([{...r,gridX:80}],terrain,0,0);const a=l.actors.get('arrival');
 for(let i=0;i<5;i++){l.update(1/60);assert.equal(a.active,'walk');assert(a.current.distanceTo(a.target)>.008);}
 a.current.copy(a.target);l.update(1/60);assert.equal(a.active,'walk');
 for(let i=0;i<30;i++)l.update(1/60);assert.equal(a.active,'consume');
});
test('social facing/connector cache follows the real partner and visible arrival',()=>{
 const rs=[resident('speaker',p({kind:'Social',socialIntent:'Comfort',targetResidentId:'listener'})),resident('listener',null,{gridX:68})];
 l.setResidents(rs,terrain,0,0);for(let i=0;i<20;i++){l.setResidents(rs,terrain,0,0);l.update(1/60);}const a=l.actors.get('speaker');
 assert.equal(a.active,'comfort');assert(Math.abs(a.root.rotation.y-Math.PI/2)<.01);assert.equal(l.socialConnectors.size,1);
 a.target.x+=1;l.update(1/60);assert(![...l.socialConnectors.values()][0].line.visible);
});
test('water prop cannot pour before visual arrival',()=>{
 const r=resident('pour',p({facilityKind:'CultivatedPlot',facilityAction:'Water'}),{civilization:{inventory:[{item:'SimpleContainer',quantity:1},{item:'RawMaterial',material:'Water',quantity:1}]}});
 l.setResidents([r],terrain,0,0);l.setResidents([{...r,gridX:80}],terrain,0,0);l.update(1/60);const a=l.actors.get(r.id);assert(!a.inventoryProps.hasWaterContainer);assert.equal(a.active,'walk');
 a.current.copy(a.target);for(let i=0;i<30;i++)l.update(1/60);assert(a.inventoryProps.hasWaterContainer);assert.equal(a.active,'water');
});
test('heading reversal turns before translating backward; pause freezes sleep',()=>{
 l.setResidents([resident('turn',p({phase:'Moving'}))],terrain,0,0);const a=l.actors.get('turn');
 a.root.rotation.y=0;a.targetYaw=Math.PI;a.target.z-=5;a.targetTravelSpeedWorldUnitsPerSecond=1;
 const z=a.current.z;l.update(1/60);assert.equal(a.current.z,z);for(let i=0;i<20;i++)l.update(1/60);assert(a.current.z<z);
 assert(Math.cos(a.targetYaw-a.root.rotation.y)>0);
 l.setResidents([resident('pause')],terrain,0,0);for(let i=0;i<60;i++)l.update(1/60);const sleeper=l.actors.get('pause');
 l.setSimulationSpeed(0);const bounds=bodyBounds(sleeper).clone();for(let i=0;i<10;i++)l.update(1/60);assert(bodyBounds(sleeper).equals(bounds));l.setSimulationSpeed(1);
});
test('1x/4x affect real travel and mixer equally, carry shares gait rate',()=>{
 l.setResidents([resident('speed',p({phase:'Moving',facilityAction:'DeliverMaterial',civilizationMaterial:'Wood'}),{civilization:{inventory:[{item:'RawMaterial',material:'Wood',quantity:2}]}})],terrain,0,0);const a=l.actors.get('speed');
 a.target.x+=100;a.targetYaw=Math.PI/2;a.root.rotation.y=Math.PI/2;a.targetTravelSpeedWorldUnitsPerSecond=1;a.smoothedTravelSpeedWorldUnitsPerSecond=1;
 l.setSimulationSpeed(1);const x=a.current.x,t=a.mixer.time;l.update(.05);const d1=a.current.x-x,t1=a.mixer.time-t;
 a.smoothedTravelSpeedWorldUnitsPerSecond=4;l.setSimulationSpeed(4);const x4=a.current.x,t4=a.mixer.time;l.update(.05);
 assert(Math.abs((a.current.x-x4)/d1-4)<1e-6);assert(Math.abs((a.mixer.time-t4)/t1-4)<1e-6);
 assert.equal(a.actions.get('walk').getEffectiveTimeScale(),a.actions.get('carry').getEffectiveTimeScale());
 l.setSimulationSpeed(0);const frozen=a.current.clone(),mt=a.mixer.time;l.update(.05);assert(a.current.equals(frozen));assert.equal(a.mixer.time,mt);
 l.setSimulationSpeed(1);
});
test('resident count scales once; repeated directives and updates allocate no new actions/meshes',()=>{
 const rs=Array.from({length:32},(_,i)=>resident('many'+i,p({kind:'Physical',physicalGoal:'Eat'})));l.setResidents(rs,terrain,0,0);
 const count=()=>{let meshes=0;for(const a of l.actors.values())a.root.traverse(o=>{if(o.isMesh)meshes++;});return [l.actors.size,meshes,[...l.actors.values()].reduce((n,a)=>n+a.mixer.stats.actions.total,0)];};
 const before=count(),downloads=loads;for(let i=0;i<10;i++){l.setResidents(rs,terrain,0,0);l.update(.016);}assert.deepEqual(count(),before);assert.equal(loads,downloads);
 assert([...l.actors.values()].every(a=>a.mixer.stats.actions.total<=20));
 console.log('CACHE METRICS',JSON.stringify({actors:before[0],meshes:before[1],actions:before[2],actionsPerActor:rs.map(r=>l.actors.get(r.id).mixer.stats.actions.total)[0]}));
 const frames=[];for(let i=0;i<200;i++){const start=performance.now();l.update(.016);frames.push(performance.now()-start);}frames.sort((a,b)=>a-b);console.log('CPU UPDATE 32 residents ms',JSON.stringify({median:frames[100],p95:frames[190]}));
 const clone=THREE.Vector3.prototype.clone;let clones=0;THREE.Vector3.prototype.clone=function(){clones++;return clone.call(this);};try{l.update(.016);}finally{THREE.Vector3.prototype.clone=clone;}assert.equal(clones,0);
});
for(const [name,d] of [
 ['wood',p({civilizationIntent:'Gather',civilizationMaterial:'Wood'})],['mineral',p({civilizationIntent:'Gather',civilizationMaterial:'Stone'})],
 ['clay',p({civilizationIntent:'Gather',civilizationMaterial:'Clay'})],['fiber',p({civilizationIntent:'Gather',civilizationMaterial:'Fiber'})],
 ['food',p({civilizationIntent:'Gather',civilizationMaterial:'PlantFood'})],['construct',p({facilityAction:'Work'})],['repair',p({facilityAction:'Repair'})],
 ['craft',p({civilizationIntent:'Craft'})],['experiment',p({civilizationIntent:'Experiment'})],['fuel',p({facilityAction:'Fuel'})],
 ['ignite',p({facilityAction:'Ignite'})],['teach',p({kind:'KnowledgeTeaching',targetResidentId:'partner'})],['comfort',p({kind:'Social',socialIntent:'Comfort',targetResidentId:'partner'})],['care',p({kind:'Parenting',parentingAction:'Feed',targetResidentId:'partner'})],
 ]){
 l.setResidents([resident('review-'+name,d),resident('partner',null,{gridX:68})],terrain,0,0);for(let i=0;i<8;i++)l.update(1/60);bake(l.actors.get('review-'+name),name);
}
writeFileSync(resolve(out,'poses.json'),JSON.stringify(review));
test('inactive skeleton cache is bounded and dispose uncaches all actor actions',()=>{l.setResidents([],terrain,0,0);assert(l.actors.size<=32);});
l.dispose();assert.equal(l.actors.size,0);
for(const both of [false,true]){
 failUAL2=true;failBoth=both;const warn=console.warn;console.warn=()=>{};let fallback;try{fallback=await layer();}finally{console.warn=warn;}
 test('missing '+(both?'all animation libraries':'UAL2')+' retains stable actual lying fallback',()=>{
  fallback.setResidents([resident('fallback')],terrain,0,0);const a=fallback.actors.get('fallback');
  for(let i=0;i<100;i++){fallback.update(1/60);assert(bodyBounds(a).min.y>=-.005);}assert(a.sleepMotion.resting);assert(bodyBounds(a).getSize(new THREE.Vector3()).y<.65);
  const b=bodyBounds(a).clone();for(let i=0;i<50;i++)fallback.update(1/60);assert(bodyBounds(a).equals(b));if(!both)assert.equal(a.actions.get('walk').getClip().name,'Walk_Loop');
 });fallback.dispose();
}
console.log('Resident motion checks:',passed,'PASS');

