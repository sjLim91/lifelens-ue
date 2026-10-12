import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {ResidentWorldLayer} from './review/render/resident-world-layer.mjs';
import {buildChunkGeometry} from './review/render/terrain-geometry.mjs';
const bytes=await fetch('/web/public/vendor/characters/character.glb').then(r=>r.arrayBuffer());
const asset=await new GLTFLoader().parseAsync(bytes,'');
GLTFLoader.prototype.loadAsync=async url=>{if(url.includes('character.glb'))return asset;throw Error('fixture: vendor animations unavailable');};
const canvas=document.querySelector('#scene'),renderer=new THREE.WebGLRenderer({canvas,antialias:true});
renderer.setSize(innerWidth,innerHeight);renderer.setPixelRatio(1);renderer.outputColorSpace=THREE.SRGBColorSpace;
const scene=new THREE.Scene();scene.background=new THREE.Color('#283b36');
const camera=new THREE.PerspectiveCamera(40,innerWidth/innerHeight,.01,100);camera.position.set(3,2.4,4);camera.lookAt(0,.85,0);
scene.add(new THREE.HemisphereLight(0xffffff,0x45483d,2));const light=new THREE.DirectionalLight(0xffffff,2);light.position.set(2,5,4);scene.add(light);
const layer=new ResidentWorldLayer();scene.add(layer.group);
for(let i=0;i<40&&!layer.ready;i++)await new Promise(r=>setTimeout(r,10));if(!layer.ready)throw Error('layer not ready');
const terrain={available:true,worldSeed:'motion-fixture',centerChunkX:0,centerChunkY:0,chunks:[],humanTraces:{entries:[]}};
for(let y=-1;y<=1;y++)for(let x=-1;x<=1;x++)terrain.chunks.push({x,y,elevation01:0,waterKind:'None',waterAvailability:0});
const groundGroup=new THREE.Group();scene.add(groundGroup);
function buildGround(slope){for(const m of [...groundGroup.children]){groundGroup.remove(m);m.geometry.dispose();m.material.dispose();}
for(const c of terrain.chunks)c.elevation01=slope ? .03+.009*c.x+.004*c.y : 0;
for(const c of terrain.chunks){const g=buildChunkGeometry(c,terrain),m=new THREE.Mesh(g,new THREE.MeshStandardMaterial({color:0x688169,roughness:1}));m.position.set(c.x*8,0,c.y*8);groundGroup.add(m);}}
const r={id:'qa',name:'검증 주민',hasPosition:true,gridX:16,gridY:16,ageYears:30,sex:'Female'};
window.setScenario=async name=>{
 layer.setSimulationSpeed(1);buildGround(name==='slope');const resident={...r,id:`qa-${name}`};
 if(name==='walk'||name==='slope'){layer.setResidents([resident],terrain,0,0);resident.gridY=20;}
 if(name==='drink'||name==='no-water'){const c=terrain.chunks.find(c=>c.x===0&&c.y===0);c.waterKind=name==='drink'?'Spring':'None';c.waterAvailability=name==='drink'?.8:0;c.salinity='Fresh';resident.presentation={active:true,kind:'Physical',phase:'Interacting',physicalGoal:'Drink',directNaturalWaterSource:true,hasTargetGrid:true,targetGridX:16,targetGridY:16};}
 if(name==='sleep'){resident.presentation={active:true,kind:'Physical',phase:'Interacting',physicalGoal:'Sleep',hasTargetGrid:true,targetGridX:16,targetGridY:16};}
 if(name==='injured'){resident.needs={sleep:.9};resident.health={injurySeverity:.7};resident.emotion={fear:.7};}
 layer.setResidents([resident],terrain,0,0);for(let i=0;i<(name==='sleep'?60:18);i++)layer.update(1/60,camera);
 const actor=layer.actors.get(resident.id);camera.position.set(actor.current.x+3,actor.current.y+2.4,actor.current.z+4);camera.lookAt(actor.current.x,actor.current.y+.8,actor.current.z);camera.updateMatrixWorld(true);
 // Labels are validated elsewhere; hide only the fixture label sprite so it
 // cannot obscure the inspected limbs at narrow portrait aspect ratios.
 if(actor.actionCue)actor.actionCue.sprite.visible=false;
 actor.root.updateMatrixWorld(true);
 const body=new THREE.Box3().setFromObject(actor.model),points=[body.min,body.max];
 const inFrame=points.every(p=>{const projected=p.clone().project(camera);return Math.abs(projected.x)<.96 && Math.abs(projected.y)<.96;});
 renderer.render(scene,camera);document.querySelector('#label').textContent=`테스트용 관찰 데이터 · ${name} · 모션: ${actor.active}`;
 return {motion:actor.active,inFrame,drawCalls:renderer.info.render.calls,triangles:renderer.info.render.triangles};
};
window.ready=true;await window.setScenario('idle');
