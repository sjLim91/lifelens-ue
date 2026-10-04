import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {source} from './compile.mjs';
const root=resolve(import.meta.dirname,'../..'),loader=new GLTFLoader(),assets=new Map();
for(const name of ['character','ual1','ual2']){const b=readFileSync(resolve(root,'public/vendor/characters/'+name+'.glb'));assets.set(name,await loader.parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),''));}
GLTFLoader.prototype.loadAsync=async url=>assets.get(url.includes('character.glb')?'character':url.includes('ual2')||url.includes('UAL2')?'ual2':'ual1');
const {ResidentWorldLayer}=await source('render/resident-world-layer.ts');
const {WORLD_GRID_CONTRACT:G}=await source('runtime/lifelens-contract.ts');
const l=new ResidentWorldLayer();for(let i=0;!l.ready&&i<200;i++)await new Promise(r=>setTimeout(r,5));assert(l.ready);
const terrain={available:true,worldSeed:'life-read-only',centerChunkX:0,centerChunkY:0,chunks:[{x:0,y:0,elevation01:0,waterKind:'None'}]};
const stages=['Baby','Toddler','Child','Teen','Adult','Elderly'];
const resident=(id,stage='Adult',extra={})=>({id,name:id,alive:true,hasPosition:true,gridX:64,gridY:64,lifeStage:stage,ageYears:stages.indexOf(stage)*15,sex:'Female',lifeHistory:[],...extra});
const facts=Array.from({length:300},(_,i)=>resident(String(i),stages[i%6],{gridX:16+i%16,gridY:16+Math.floor(i/16),pregnancy:i%6===4?{role:'GestationalParent',stage:'ThirdTrimester'}:undefined}));
function resources(){const gs=new Set(),ms=new Set();for(const a of l.actors.values())a.root.traverse(o=>{if(o.geometry)gs.add(o.geometry);if(o.material)for(const m of Array.isArray(o.material)?o.material:[o.material])ms.add(m);});return{roots:l.actors.size,geometry:gs.size,material:ms.size};}
for(const count of [100,300]){l.setResidents(facts.slice(0,count),terrain,0,0);const before=resources();const actors=new Map([...l.actors].map(([id,a])=>[id,a.root]));for(let n=0;n<100;n++){l.setResidents(facts.slice(0,count),terrain,0,0);l.update(.016);}assert.deepEqual(resources(),before);for(const [id,a]of l.actors)assert.equal(a.root,actors.get(id));console.log('LIFECYCLE RESOURCES',JSON.stringify({residents:count,refreshes:100,...before}));}
const original=l.actors.get('0'),position=original.current.clone(),oldScale=original.root.scale.y;
l.setResidents([{...facts[0],lifeStage:'Child',ageYears:8}],terrain,0,0);assert.equal(l.actors.get('0'),original);assert.equal(original.root.scale.y,oldScale);l.update(.1);assert(original.root.scale.y>oldScale);for(let n=0;n<60;n++)l.update(.02);assert(original.current.equals(position));
const pregnant=resident('p','Adult',{ageYears:30,pregnancy:{role:'GestationalParent',stage:'ThirdTrimester'}});l.setResidents([pregnant],terrain,0,0);const actor=l.actors.get('p'),profileResources=resources();
let mesh;actor.model.traverse(o=>{if(o.isSkinnedMesh&&o.geometry.attributes.lifeTorsoMask)mesh=o;});assert(mesh);assert([...mesh.geometry.attributes.lifeTorsoMask.array].some(v=>v>0));
const shader={uniforms:{},vertexShader:'#include <begin_vertex>'};mesh.material.onBeforeCompile(shader,{});assert.equal(shader.uniforms.lifeBelly.value,.065);
l.setResidents([{...pregnant,pregnancy:{role:'GeneticPartner',stage:'ThirdTrimester'}}],terrain,0,0);assert.equal(shader.uniforms.lifeBelly.value,0);assert.deepEqual(resources(),profileResources);
l.setResidents([{...pregnant,pregnancy:{role:'GestationalParent',stage:'Completed'}}],terrain,0,0);assert.equal(shader.uniforms.lifeBelly.value,0);
const sleep={active:true,kind:'Physical',phase:'Interacting',physicalGoal:'Sleep',hasTargetGrid:true,targetGridX:64,targetGridY:64};
for(const stage of ['Baby','Toddler','Child','Adult','Elderly']){l.setResidents([resident('s',stage,{presentation:sleep,pregnancy:stage==='Adult'?pregnant.pregnancy:undefined})],terrain,0,0);for(let n=0;n<180;n++)l.update(.02);const a=l.actors.get('s');assert(a.sleepMotion.active);a.root.updateMatrixWorld(true);let min=Infinity;a.root.traverse(o=>{if(!o.isSkinnedMesh)return;const p=o.geometry.attributes.position,v=new THREE.Vector3();for(let i=0;i<p.count;i++){o.getVertexPosition(i,v);v.applyMatrix4(o.matrixWorld);min=Math.min(min,v.y);}});assert(min>=-.011,`${stage} body penetrates ground ${min}`);}
l.setSimulationSpeed(0);const frozen=l.actors.get('s').root.scale.clone();l.update(1);assert(l.actors.get('s').root.scale.equals(frozen));l.dispose();assert.equal(l.actors.size,0);console.log('Lifecycle real-model identity, pregnancy, pause and sleep PASS');
