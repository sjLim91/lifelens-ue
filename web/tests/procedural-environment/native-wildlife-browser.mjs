import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import {WildlifeLayer} from './review/src/render/wildlife-layer.mjs';
import {createTerrainGeometryBuilder} from './review/src/render/terrain-geometry.mjs';
import {ResidentGroundSurface} from './review/src/render/resident-grounding.mjs';
import {buildFlowWaterSurfaceGeometry} from './review/src/render/water-geometry.mjs';
import {WORLD_GRID_CONTRACT as grid} from './review/src/runtime/lifelens-contract.mjs';
import {NATIVE_WILDLIFE_SOURCES as sources} from './review/src/render/native-wildlife-manifest.mjs';
const query=new URLSearchParams(location.search),species=query.get('species')??'Hare',baseline=query.get('baseline')==='1';
const fixture=await fetch('./review/core-fixture.json').then(r=>{if(!r.ok)throw Error('Actual Core fixture missing');return r.json();});
const terrain=fixture.terrain,before=JSON.stringify(terrain);
const scene=new THREE.Scene();scene.background=new THREE.Color('#dbe2e3');
scene.add(new THREE.HemisphereLight(0xf3f7ff,0x746550,2.1));
const sun=new THREE.DirectionalLight(0xfff1de,3);sun.position.set(4,9,5);scene.add(sun);
const fill=new THREE.DirectionalLight(0xc8dcff,1);fill.position.set(-5,3,-2);scene.add(fill);
const geometry=createTerrainGeometryBuilder(terrain);
for(const chunk of terrain.chunks){
 const mesh=new THREE.Mesh(geometry(chunk),new THREE.MeshStandardMaterial({vertexColors:true,roughness:1}));
 mesh.position.set((chunk.x-terrain.centerChunkX)*grid.worldUnitsPerChunk,0,(chunk.y-terrain.centerChunkY)*grid.worldUnitsPerChunk);scene.add(mesh);
}
if(species==='FreshwaterFish')scene.add(new THREE.Mesh(buildFlowWaterSurfaceGeometry(terrain),new THREE.MeshPhysicalMaterial({color:0x4a9196,transparent:true,opacity:.24,roughness:.22,depthWrite:false,side:THREE.DoubleSide})));
const loader=new GLTFLoader();
const layer=new WildlifeLayer(async()=>{throw Error('Independent tiger path is outside this subsystem comparison');},baseline?null:(s,t)=>loader.loadAsync(`/public/vendor/wildlife/${sources[s].slug}${t==='near'?'-rigged':'-'+t}.glb`));
scene.add(layer.group);layer.setTerrain(terrain);
const candidates=new Set(layer.candidateCoreIds);
const eligible=terrain.wildlife.animals.filter(a=>a.species===species&&candidates.has(a.id));
const subject=(species==='Deer'?eligible.find(a=>a.sex==='Male'):null)??eligible[0];
if(!subject)throw Error('No physically observable Core subject for '+species);
const x=(subject.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
const z=(subject.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
const ground=new ResidentGroundSurface();ground.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);
const y=ground.height(x,z);if(y===null)throw Error('Core subject outside actual terrain');
const camera=new THREE.PerspectiveCamera(38,innerWidth/innerHeight,.02,300);
const renderer=new THREE.WebGLRenderer({antialias:true,preserveDrawingBuffer:true});renderer.setPixelRatio(1);renderer.setSize(innerWidth,innerHeight);
renderer.outputColorSpace=THREE.SRGBColorSpace;renderer.toneMapping=THREE.ACESFilmicToneMapping;renderer.toneMappingExposure=1;document.body.append(renderer.domElement);
let currentView='quarter',frames=0,last=performance.now();
const size=species==='Deer'?2.1:species==='Wolf'?1.3:species==='Hare'?.8:.14;
function view(name){
 currentView=name;
 const actor=layer.group.getObjectByName('LifeLensNativeAnimal:'+subject.id);
 const centerY=actor?.position.y??y;
 const direction=name==='side'?[1,0]:name==='back'?[0,-1]:name==='front'?[0,1]:[.65,1];
 const distance=name==='far'?42:size*2.5;
 camera.position.set(x+direction[0]*distance,centerY+size*.8,z+direction[1]*distance);
 camera.lookAt(x,centerY+size*.38,z);camera.updateMatrixWorld();
}
view('quarter');
function render(now){const dt=Math.min(.1,(now-last)/1000);last=now;layer.update(dt,camera);renderer.render(scene,camera);frames++;requestAnimationFrame(render);}
requestAnimationFrame(render);
addEventListener('resize',()=>{camera.aspect=innerWidth/innerHeight;camera.updateProjectionMatrix();renderer.setSize(innerWidth,innerHeight);});
document.querySelector('#status').textContent=`${species} · Core ID ${subject.id} · ${subject.behavior} · ${baseline?'기존 표시':'원본 기반 모델'} · 실제 WildlifeLayer`;
window.nativeWildlifeReview={view,layer,renderer,scene,camera,metrics(){
 const actor=layer.group.getObjectByName('LifeLensNativeAnimal:'+subject.id);
 const far=layer.group.getObjectByName('LifeLensNativeFar:'+species);
 return {species,id:subject.id,behavior:subject.behavior,sex:subject.sex,gridX:subject.gridX,gridY:subject.gridY,
  baseline,coreUnchanged:JSON.stringify(terrain)===before,importedActor:!!actor,clip:actor?.userData.clip,tier:actor?.userData.tier,
  farIds:far?.userData.coreIds??[],view:currentView,frames,visibleAnimals:layer.instanceCount,
  renderer:{...renderer.info.render},canvas:{width:renderer.domElement.width,height:renderer.domElement.height,contextLost:renderer.getContext().isContextLost()},
  source:'UNMODIFIED_NATIVE_CORE_FIXTURE',scope:'PRODUCTION_WILDLIFE_LAYER_SUBSYSTEM',fullObserver:'NOT_RUN',physicalAndroid:'NOT_MEASURED'};
}};
