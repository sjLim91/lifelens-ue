import assert from 'node:assert/strict';
import * as THREE from 'three';
import {source} from './compile.mjs';
const {deriveResidentLifeVisualState:life,parentingPresentationPair:parenting}=await source('render/resident-life-presentation.ts');
const {ResidentLifeEventPresentation:Timeline}=await source('render/resident-life-event-presentation.ts');
const {ResidentLifeEventLayer,lifeEventStrokes}=await source('render/resident-life-event-layer.ts');
const {SocialEventLayer}=await source('render/social-event-layer.ts');
const {ResidentLifeShape}=await source('render/resident-life-shape.ts');
const {createResidentAppearanceProfile:appearance}=await source('render/resident-appearance.ts');
const {WORLD_PRESENTATION:{lifeEvents:C}}=await source('render/world-presentation-config.ts');
let passed=0;let now=0;function test(name,fn){now=0;fn();passed++;console.log('PASS',name);}
const resident=(id='a',extra={})=>({id,name:id,alive:true,hasPosition:true,gridX:0,gridY:0,ageYears:30,lifeStage:'Adult',lifeHistory:[],...extra});
const event=(type,minute=101,relatedCharacterIds=[])=>({type,minute,relatedCharacterIds});
const anchors=new Map([['a',{x:0,y:0,z:0}],['b',{x:1,y:0,z:0}],['child',{x:.5,y:0,z:0}]]);
const resolve=id=>anchors.get(id)??null;const timeline=()=>new Timeline(resolve,()=>now);
const parent=(events=[])=>resident('a',{family:{children:[{id:'child'}]},lifeHistory:events});
const child=(events=[])=>resident('child',{ageYears:0,lifeStage:'Baby',family:{parents:[{id:'a'}]},lifeHistory:events});
test('exact eight Core stages; age mismatch never corrects explicit lifeStage',()=>{
 for(const [stage,expected] of [['Baby','Baby'],['Toddler','Toddler'],['Child','Child'],['Teen','Teen'],['YoungAdult','Adult'],['Adult','Adult'],['MiddleAge','Adult'],['Elderly','Elderly']])assert.equal(life(resident('a',{lifeStage:stage,ageYears:30})).stage,expected);
 assert.equal(life(resident('a',{lifeStage:'Adult',ageYears:1})).stage,'Adult');assert.equal(life(resident('a',{lifeStage:'Future',ageYears:1})).stage,'Adult');
 assert(appearance(resident('a',{lifeStage:'Baby'})).heightWorldUnits<appearance(resident()).heightWorldUnits);
});
test('gestational parent only and Completed/unknown/absent stages remove pregnancy',()=>{
 for(const stage of ['FirstTrimester','SecondTrimester','ThirdTrimester','Due']){
 assert.equal(life(resident('a',{pregnancy:{role:'GestationalParent',stage}})).pregnancyStage,stage);
 assert.equal(life(resident('a',{pregnancy:{role:'GeneticPartner',stage}})).belly,0);}
 for(const stage of ['Completed','Unknown'])assert.equal(life(resident('a',{pregnancy:{role:'GestationalParent',stage}})).belly,0);
 assert.equal(life(resident('a',{family:{expectingChild:true,isGestationalParent:true}})).belly,0);
});
test('growth keeps phenotype identity and first/third stage appearance deterministic',()=>{
 const a=appearance(resident('a',{genetics:{heightPotential:.8,hairPigment:.2,faceShape:.4}}));
 const b=appearance(resident('a',{lifeStage:'Child',ageYears:10,genetics:{heightPotential:.8,hairPigment:.2,faceShape:.4}}));
 assert.equal(a.seed,b.seed);assert.equal(a.hairColor,b.hairColor);assert(b.heightWorldUnits<a.heightWorldUnits);assert(b.headWidthScale>a.headWidthScale);
});
test('first snapshot old history baseline and same snapshot never replays beyond seen cap',()=>{
 const t=timeline(),rs=Array.from({length:300},(_,i)=>resident(String(i),{lifeHistory:Array.from({length:30},(_,n)=>event('LifeStageChanged',n))}));
 for(let n=0;n<500;n++)t.observe(rs,100);assert.equal(t.activeCues.length,0);assert(t.seenCount<=C.maxSeen);assert.equal(t.cursorCount,300);
 t.reset();t.observe(rs,100);assert.equal(t.activeCues.length,0);
});
test('exact new event repeats once; old events, future enums and unavailable history fail closed',()=>{
 const t=timeline();t.observe([resident()],100);for(let n=0;n<10;n++)t.observe([resident('a',{lifeHistory:[event('LifeStageChanged')]})],101);assert.equal(t.activeCues.length,1);
 now=4;t.update();t.observe([resident('a',{lifeHistory:[event('LifeStageChanged')]})],101);assert.equal(t.activeCues.length,0);
 t.observe([resident('a',{lifeHistory:[event('LifeStageChanged'),event('FutureType',102),event('Married',1)]})],102);assert.equal(t.activeCues.length,0);
});
test('new newborn residents require exact Birth/ChildBorn; missing child never creates cue',()=>{
 const t=timeline();t.observe([parent()],100);t.observe([parent([event('ChildBorn',101,['child'])])],101);assert.equal(t.activeCues.length,0);
 const t2=timeline();t2.observe([parent()],100);t2.observe([parent(),child()],101);assert.equal(t2.activeCues.length,0);
 t2.observe([parent([event('ChildBorn',102,['child'])]),child([event('Birth',102,['a'])])],102);assert.equal(t2.activeCues.length,1);assert.equal(t2.activeCues[0].actorId,'child');
 const t3=timeline();t3.observe([resident()],100);t3.observe([resident('a',{lifeHistory:[event('ChildBorn',101,['b'])]}),resident('b',{lifeStage:'Baby'})],101);assert.equal(t3.activeCues.length,0);
});
test('all exact lifecycle types have bounded semantic geometry and reciprocal marriage coalesces',()=>{
 for(const type of ['PregnancyStarted','LifeStageChanged','DatingStarted','Engaged','Married','CohabitationStarted','ParentingMilestone','Separated','Divorced','PartnerWidowed','HouseholdChanged','Bereavement']){
  const t=timeline();t.observe([resident(),resident('b')],100);t.observe([resident('a',{lifeHistory:[event(type,101,['b'])]}),resident('b')],101);assert.equal(t.activeCues.length,1,type);
  const strokes=[];lifeEventStrokes(t.activeCues[0],.5,(...s)=>strokes.push(s));assert(strokes.length>0&&strokes.length<=C.segmentsPerCue);assert(strokes.flat().every(Number.isFinite));
 }
 const t=timeline();t.observe([resident(),resident('b')],100);t.observe([resident('a',{lifeHistory:[event('Married',101,['b'])]}),resident('b',{lifeHistory:[event('Married',101,['a'])]})],101);assert.equal(t.activeCues.length,1);
});
test('exact Parenting action/phase/target, dead or missing residents guarded',()=>{
 const byId=new Map([['b',resident('b',{lifeStage:'Child'})]]);
 const p={active:true,kind:'Parenting',phase:'Interacting',parentingAction:'Feed',targetResidentId:'b'};
 assert(parenting(resident('a',{presentation:p}),byId));assert(parenting(resident('a',{presentation:{...p,phase:'Moving'}}),byId));
 for(const patch of [{targetResidentId:'wrong'},{active:false},{phase:'None'},{parentingAction:'Carry'}])assert.equal(parenting(resident('a',{presentation:{...p,...patch}}),byId),null);
 assert.equal(parenting(resident('a',{alive:false,presentation:p}),byId),null);byId.set('b',resident('b',{alive:false}));assert.equal(parenting(resident('a',{presentation:p}),byId),null);
});
test('no inferred couple/pregnancy/birth event; distance and death clear living connections',()=>{
 const t=timeline();t.observe([resident(),resident('b')],100);t.observe([resident('a',{family:{hasActivePartner:true,expectingChild:true},relationships:[{socialBond:1}]}),resident('b')],101);assert.equal(t.activeCues.length,0);
 anchors.set('b',{x:100,y:0,z:0});t.observe([resident('a',{lifeHistory:[event('Married',102,['b'])]}),resident('b')],102);assert.equal(t.activeCues[0].target,undefined);anchors.set('b',{x:1,y:0,z:0});
 anchors.delete('a');t.update();assert.equal(t.activeCues.length,0);anchors.set('a',{x:0,y:0,z:0});
});
test('anchors snapshot and rebase; Death uses prior rendered site, no dead partner line',()=>{
 const t=timeline();t.observe([resident()],100);t.captureDepartures([resident('a',{alive:false,lifeHistory:[event('Death')]})]);t.rebase(8,-4);anchors.delete('a');t.observe([resident('a',{alive:false,lifeHistory:[event('Death')]})],101);
 assert.equal(t.activeCues.length,1);assert.equal(t.activeCues[0].target,undefined);t.rebase(-16,32);assert.equal(t.activeCues[0].actor.x,-8);assert.equal(t.activeCues[0].actor.z,28);now=4;t.update();assert.equal(t.activeCues.length,0);anchors.set('a',{x:0,y:0,z:0});
});
test('GPU one fixed buffer/material, pause, 1000 refreshes, bounds and disposal',()=>{
 const l=new ResidentLifeEventLayer(resolve),camera=new THREE.PerspectiveCamera();camera.position.set(5,8,10);l.observe([resident()],100);const mesh=l.group.children[0],geo=mesh.geometry,mat=mesh.material;let gd=0,md=0;geo.addEventListener('dispose',()=>gd++);mat.addEventListener('dispose',()=>md++);
 const history=[];for(let n=0;n<1000;n++){history.push(event('LifeStageChanged',101+n));l.observe([resident('a',{lifeHistory:[...history]})],101+n);l.update(camera,.001);assert(l.presentation.activeCues.length<=C.maxActive);assert(l.presentation.seenCount<=C.maxSeen);assert.equal(mesh.geometry,geo);assert.equal(mesh.material,mat);assert.equal(l.group.children.length,1);}
 const before=l.presentation.now();l.setSimulationSpeed(0);l.update(camera,100);assert.equal(l.presentation.now(),before);
 l.setSimulationSpeed(16);l.update(camera,10);assert.equal(l.presentation.activeCues.length,0);l.dispose();l.dispose();assert.equal(gd,1);assert.equal(md,1);
});
test('Married/Commitment and Dating/Intimacy coalesce visuals only; unmatched social remains',()=>{
 let n=0;const l=new SocialEventLayer(resolve,()=>n),camera=new THREE.PerspectiveCamera();camera.position.set(5,8,10);
 const e={sequence:'1',actorId:'a',targetId:'b',type:'Commitment',intensity:1,importance:1,minute:101,presentationLevel:'Important',successful:true};l.observe({available:true,events:[e]},101);n=.5;l.update(camera);assert(l.geometry.drawRange.count>0);
 l.update(camera,new Map([[JSON.stringify(['a','b']),101]]));assert.equal(l.geometry.drawRange.count,0);assert.equal(l.presentation.activeCues.length,1);
 l.update(camera,new Map([[JSON.stringify(['a','b']),90]]));assert(l.geometry.drawRange.count>0);l.dispose();
});
test('scale settles around feet pivot and freezes without mutation of Core facts',()=>{
 const root=new THREE.Group(),model=new THREE.Group(),shape=new ResidentLifeShape(root,model),r=resident();root.position.set(2,0,3);shape.setFacts(r,new THREE.Vector3(1,1,1),true);shape.setFacts({...r,lifeStage:'Child'},new THREE.Vector3(1.4,1.4,1.4));shape.updateScale(.1);assert(root.scale.y>1&&root.scale.y<1.4);const previous=root.scale.y;shape.updateScale(0);assert.equal(root.scale.y,previous);shape.updateScale(1);assert.equal(root.scale.y,1.4);assert.deepEqual(root.position.toArray(),[2,0,3]);assert.equal(r.lifeStage,'Adult');
});
test('late initial full observation is baseline, rather than old-event replay',()=>{
 const t=timeline();t.observe([resident(),resident('b',{lifeHistory:undefined})],100);
 t.observe([resident(),resident('b',{lifeHistory:[event('Married',100,['a'])]})],101);assert.equal(t.activeCues.length,0);
 const t2=timeline();t2.observe([resident(),resident('b')],100);
 t2.observe([resident('a',{lifeHistory:[event('Engaged',101,['b']),event('Married',101,['b'])]}),resident('b')],101);
 assert.equal(t2.activeCues.length,1);assert.equal(t2.activeCues[0].type,'Married');
});
console.log(`Resident lifecycle: ${passed} tests passed`);
