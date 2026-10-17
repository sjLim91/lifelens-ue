import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {WildlifeLayer} from './review/src/render/wildlife-layer.mjs';
import {createTerrainGeometryBuilder} from './review/src/render/terrain-geometry.mjs';
import {ResidentGroundSurface} from './review/src/render/resident-grounding.mjs';
import {WORLD_GRID_CONTRACT as grid} from './review/src/runtime/lifelens-contract.mjs';

// This deliberately exercises the production layer, not a separate model viewer.
// It remains a subsystem test; it is not the React/WASM Observer acceptance gate.
const query=new URLSearchParams(location.search);
const baseline=query.get('baseline')==='1';
const inspection=query.get('inspection')==='1';
const fixture=await fetch('./review/core-fixture.json').then(r=>{if(!r.ok)throw Error('Missing authoritative Core fixture');return r.json();});
const terrain=fixture.terrain;
const before=JSON.stringify(terrain);
const scene=new THREE.Scene();scene.background=new THREE.Color('#d9e1e3');
scene.add(new THREE.HemisphereLight(0xf3f7ff,0x746550,2.1));
const sun=new THREE.DirectionalLight(0xfff1de,3);sun.position.set(4,9,5);scene.add(sun);
const fill=new THREE.DirectionalLight(0xc8dcff,1);fill.position.set(-5,3,-2);scene.add(fill);
const geometry=createTerrainGeometryBuilder(terrain);
const terrainMeshes=[];
for(const chunk of terrain.chunks){
  const mesh=new THREE.Mesh(geometry(chunk),new THREE.MeshStandardMaterial({vertexColors:true,roughness:1}));
  mesh.position.set((chunk.x-terrain.centerChunkX)*grid.worldUnitsPerChunk,0,
    (chunk.y-terrain.centerChunkY)*grid.worldUnitsPerChunk);
  scene.add(mesh);mesh.updateMatrixWorld(true);terrainMeshes.push(mesh);
}
const loader=new GLTFLoader();
const layer=new WildlifeLayer(baseline?async()=>{throw Error('Requested baseline fallback');}:tier=>{
  const file=tier==='near'?'tiger-rigged.glb':`tiger-${tier}.glb`;
  return loader.loadAsync(inspection?`./review/inspection-assets/${file}`:`/public/vendor/wildlife/${file}`);
});
scene.add(layer.group);layer.setTerrain(terrain);
const tigers=terrain.wildlife?.animals.filter(animal=>animal.species==='Tiger')??[];
if(!tigers.length)throw Error('Core fixture contains no tiger; do not invent one for the capture');
const tiger=tigers[0];
const x=(tiger.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
const z=(tiger.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
const ground=new ResidentGroundSurface();ground.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);
const y=ground.height(x,z);if(y===null)throw Error('Observed tiger has no visible Core terrain');
const ray=new THREE.Raycaster(new THREE.Vector3(x,y+10,z),new THREE.Vector3(0,-1,0));
const terrainHit=ray.intersectObjects(terrainMeshes,false)[0];
const terrainRayError=terrainHit?Math.abs(terrainHit.point.y-y):Infinity;
if(terrainRayError>2e-5)throw Error('Displayed terrain does not match Core grounding at the tiger');
const camera=new THREE.PerspectiveCamera(38,innerWidth/innerHeight,.03,300);
const views={front:[0,1.1,4.4],side:[4.4,1.1,0],quarter:[3,1.6,4],back:[0,1.1,-4.4],face:[.45,1.2,2]};
function view(name){const v=views[name]??views.quarter;camera.position.set(x+v[0],y+v[1],z+v[2]);camera.lookAt(x,y+.65,z);camera.updateMatrixWorld();}
view(query.get('view')??'quarter');
const renderer=new THREE.WebGLRenderer({antialias:true,preserveDrawingBuffer:true});
renderer.setPixelRatio(1);renderer.setSize(innerWidth,innerHeight);renderer.outputColorSpace=THREE.SRGBColorSpace;
renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1;document.body.append(renderer.domElement);
addEventListener('resize',()=>{camera.aspect=innerWidth/innerHeight;camera.updateProjectionMatrix();renderer.setSize(innerWidth,innerHeight);});
const times=[];let last=performance.now();let frames=0;
function render(now){const dt=Math.min(.1,(now-last)/1000);last=now;
 layer.update(dt,camera);renderer.render(scene,camera);if(frames++>30)times.push(dt*1000);
 requestAnimationFrame(render);
}
requestAnimationFrame(render);
const status=document.querySelector('#status');
status.textContent=`Core ID ${tiger.id} · ${tiger.behavior} · ${baseline?'기존 fallback':inspection?'검사 파생본':'원본 기반 후보'} · 실제 WildlifeLayer · 전체 Observer/Android 미검증`;
window.tigerReview={view,layer,scene,camera,renderer,
 metrics(){const actor=layer.group.getObjectByName(`LifeLensTiger:${tiger.id}`);
  return {coreId:tiger.id,coreBehavior:tiger.behavior,coreUnchanged:JSON.stringify(terrain)===before,
   importedActor:Boolean(actor),clip:actor?.userData.clip,tier:actor?.userData.tier,
   visibleAnimals:layer.instanceCount,terrainRayError,source:inspection?'LOCAL_INSPECTION_DERIVATIVE':'STAGED_PINNED_SOURCE_DERIVATIVE',
   subsystemOnly:true,fullObserver:'NOT_RUN',android:'NOT_MEASURED',
   renderer:renderer.info.render,frames,samples:times.slice(-120),width:innerWidth,height:innerHeight};}};
