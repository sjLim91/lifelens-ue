import * as THREE from 'three';
import { createRoot } from 'react-dom/client';
import '../../src/styles.css';
import { WorldScene } from '../../src/render/world-scene';
import { SettlementDetail } from '../../src/ui/settlement-detail';
import { SelectedResidentReadout } from '../../src/ui/observer-readout';
import { observerActions } from '../../src/state/observer-actions';
const mode = new URL(location.href).searchParams.get('mode') ?? 'two';
const world = new WorldScene();
const renderer = new THREE.WebGLRenderer({antialias:true});
renderer.setPixelRatio(1); renderer.setSize(innerWidth, innerHeight);document.body.append(renderer.domElement);
world.resize(innerWidth,innerHeight);
const grid = (x:number) => (x/8+.5)*32;
const settlement=(id:string,x:number,z:number,extra={})=>({id,gridX:grid(x),gridY:grid(z),residentCount:4,facilityCount:3,
  operationalFacilityCount:2,plannedFacilityCount:1,storageSiteCount:1,active:true,established:true,...extra});
let settlements=[settlement('2',-3,0),settlement('4',3,0)];
if(['single','grown','migration','frontier'].includes(mode))settlements=[settlement('2',0,0,mode==='grown'?{residentCount:11,facilityCount:18,operationalFacilityCount:15,plannedFacilityCount:3,storageSiteCount:3}:{})];
if(mode==='many')settlements=Array.from({length:10},(_,i)=>settlement(String(i+1),(i%5-2)*6,Math.floor(i/5)*8-4));
if(mode==='invalid')settlements=[settlement('',NaN,0)];
const route=(id:string,a:any,b:any,active=true)=>({id,firstSettlement:a.id,secondSettlement:b.id,
  firstGridX:a.gridX,firstGridY:a.gridY,secondGridX:b.gridX,secondGridY:b.gridY,
  partnerCount:active?3:0,exchangeCount:active?8:0,distanceGrid:24,active});
let routes=['active','inactive','popup','min','max'].includes(mode)?[route('2:4',settlements[0],settlements[1],mode!=='inactive')]:[];
if(mode==='many')routes=settlements.slice(1).map((s,i)=>route(String(i+1),settlements[0],s));
if(mode==='invalid')routes=[route('bad',settlements[0],{id:'missing',gridX:0,gridY:Infinity})];
const facility=(id:string,kind:string,x:number,z:number)=>({id,kind,gridX:grid(x),gridY:grid(z),state:'Operational',active:true,lit:false,durability:1,workProgress:1,constructionWork:100,requiredWork:100,requiredMaterialUnits:8,deliveredMaterialUnits:8,requirements:[{material:'Wood',required:8,delivered:8}]});
const facilities=mode==='invalid'?[]:settlements.flatMap((s,i)=>{
 const x=(s.gridX/32-.5)*8,z=(s.gridY/32-.5)*8;
 return [facility(`${i*3+1}`,'PrimitiveStorage',x-.7,z-.8),facility(`${i*3+2}`,'Shelter',x+.6,z-.8),facility(`${i*3+3}`,'WorkSurface',x,z+1.2)];
});
if(mode==='grown')for(let i=0;i<15;i++)facilities.push(facility(String(i+4),['SleepingPlace','WorkSurface','FirePit'][i%3],(i%5-2)*3,Math.floor(i/5)*3-5));
const residents=mode==='invalid'?[]:settlements.flatMap((s,i)=>Array.from({length:mode==='grown'?11:2},(_,j)=>({
 id:`r${i}-${j}`,name:j?'수림':'가람',alive:true,hasPosition:true,gridX:s.gridX+(j%4-1)*2,gridY:s.gridY+8+Math.floor(j/4)*2,
 sex:j?'Female':'Male',lifeStage:'Adult',ageYears:30,lifeHistory:[],genetics:{heightPotential:.5,buildPotential:.5,faceShape:.5,skinTone:.5,hairPigment:.5},
 civilization:{inventory:[{item:'RawMaterial',material:'Wood',quantity:2}]},
 ...(i===0&&j===0&&['migration','frontier'].includes(mode)?{migration:{candidate:true,pressure01:.72,resourceScarcity01:.8,travelBurden01:.6,populationPressure01:.7,settlementAttachment01:.4,explorationDisposition01:.65,bottleneckMaterial:'Wood',hasFrontierTarget:mode==='frontier',frontierGridX:300,frontierGridY:30,frontierDistanceChunks:8}}:{}),
 presentation:{active:true,kind:'Trade',phase:'Moving',hasTargetGrid:true,targetGridX:s.gridX+4,targetGridY:s.gridY+4}
 })));
const traces=facilities.map(f=>({...f,id:'facility:'+f.id,kind:'Facility',facilityKind:f.kind,progress01:1}));
const terrain={available:true,worldSeed:'settlement-fixture',centerChunkX:0,centerChunkY:0,humanTraces:{available:true,entries:traces},
 chunks:Array.from({length:25},(_,i)=>({x:i%5-2,y:Math.floor(i/5)-2,elevation01:0,waterKind:'None',grassCoverage01:.1,shrubCoverage01:0,rockCoverage01:0}))};
const civilization={available:true,settlements,tradeRoutes:routes,facilities,storages:[],resources:[]};
const refresh=()=>{world.setTerrain(terrain as any);world.setAuthoritativeSpatialTargets(civilization as any,{available:false} as any,terrain as any);world.setResidents(residents as any,terrain as any,0,0);};
refresh();
for(let n=0;!(world as any).residentLayer.ready&&n<600;n++)await new Promise(r=>setTimeout(r,10));
if(!(world as any).residentLayer.ready)throw Error('production resident GLB missing');
refresh();world.setSimulationSpeed(1);world.setSimulationMinute(720);
const state={centerChunkX:0,centerChunkY:0,zoom:mode==='min'?.55:mode==='many'?(innerWidth<500?2:4):mode==='grown'?(innerWidth<500?2.8:4.5):6.4,angle:Math.PI/3,elevation:.85,panX:0,panZ:0};
world.setCamera(state);for(let i=0;i<100;i++)world.update(.02);
const ui=createRoot(document.getElementById('ui')!);let selected:string|null=null;
const drawUI=()=>ui.render(<><SettlementDetail snapshot={{selectedSettlementId:selected,civilization,observations:[]} as any}/>{['migration','frontier'].includes(mode)&&<div id="readout"><SelectedResidentReadout resident={residents[0] as any} residents={residents as any} onClear={()=>{world.setSelectedResident(null);ui.render(null);}}/></div>}</>);
observerActions.bind({selectSettlement(id){selected=id;world.setSelectedSettlement(id);drawUI();}} as any);
renderer.domElement.addEventListener('click',event=>{const id=world.pickSettlement(event.clientX/innerWidth*2-1,-event.clientY/innerHeight*2+1);observerActions.selectSettlement(id);});
if(['migration','frontier'].includes(mode))world.setSelectedResident(residents[0].id);
drawUI();
const render=()=>{world.update(0);for(const a of (world as any).residentLayer.actors.values())a.actionCue.sprite.visible=false;renderer.render(world.scene,world.camera);};render();
const overlay=(world as any).settlementFocusLayer;
overlay.group.visible=false;renderer.render(world.scene,world.camera);const baselineCalls=renderer.info.render.calls;overlay.group.visible=true;render();
const stats=()=>{let objects=0;const materials=new Set();world.scene.traverse((o:any)=>{objects++;if(o.material)for(const m of Array.isArray(o.material)?o.material:[o.material])materials.add(m);});return {calls:renderer.info.render.calls,baselineCalls,overlayCalls:renderer.info.render.calls-baselineCalls,geometries:renderer.info.memory.geometries,materials:materials.size,objects,overlay:overlay.diagnostics};};
(window as any).review={stats,refresh1000(){render();const before=stats(),ids=overlay.group.children.map((o:any)=>[o.uuid,o.geometry.uuid,o.material.uuid]);for(let i=0;i<1000;i++)overlay.setTargets(structuredClone(civilization),terrain);render();return{before,after:stats(),identity:JSON.stringify(ids)===JSON.stringify(overlay.group.children.map((o:any)=>[o.uuid,o.geometry.uuid,o.material.uuid]))};},anchor(){const s=settlements[0],p=new THREE.Vector3((s.gridX/32-.5)*8,0,(s.gridY/32-.5)*8).project(world.camera);return{x:(p.x+1)*innerWidth/2,y:(1-p.y)*innerHeight/2};},open(){observerActions.selectSettlement(settlements[0].id);},reset(){world.resetSocialEvents();render();return overlay.diagnostics;}};
(window as any).fixtureReady=true;
