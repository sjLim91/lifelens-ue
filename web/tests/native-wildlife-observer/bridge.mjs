// Camera-only instrumentation of the actual production React/WASM Observer.
import {observerStore} from '../../src/state/observer-store.ts';
import {observerActions} from '../../src/state/observer-actions.ts';
import {WorldScene} from '../../src/render/world-scene.ts';
import {WorldRenderer} from '../../src/render/world-renderer.ts';
import {ResidentGroundSurface} from '../../src/render/resident-grounding.ts';
import {NativeWildlifePool} from '../../src/render/native-wildlife.ts';
import {WORLD_GRID_CONTRACT as grid} from '../../src/runtime/lifelens-contract.ts';
import {Box3,Vector3,Matrix4,Mesh,Raycaster,FrontSide} from 'three';
const qa=window.__nativeWildlifeObserver={store:observerStore,actions:observerActions,id:null,view:'quarter',baseline:false,choices:{},world:null,renderer:null};
const draw=NativeWildlifePool.prototype.draw;
NativeWildlifePool.prototype.draw=function(...args){return qa.baseline?false:draw.apply(this,args);};
const setCamera=WorldRenderer.prototype.setCamera;
WorldRenderer.prototype.setCamera=function(state){qa.renderer=this;setCamera.call(this,state);};
const applyCamera=WorldScene.prototype.applyCamera,ground=new ResidentGroundSurface();let identity=null;
function choose(scene,actor,view,camera){
 const box=new Box3(),p=new Vector3();actor.updateMatrixWorld(true);
 actor.traverse(o=>{if(!o.isSkinnedMesh||!o.visible)return;o.skeleton.update();const count=o.geometry.attributes.position.count;
  for(let i=0;i<count;i+=Math.max(1,Math.floor(count/240))){o.getVertexPosition(i,p);box.expandByPoint(p.applyMatrix4(o.matrixWorld));}});
 if(box.isEmpty())return null;
 const target=box.getCenter(new Vector3()),extent=box.getSize(new Vector3()).length(),points=[target.clone()];
 for(const x of [box.min.x,box.max.x])for(const y of [box.min.y,box.max.y])for(const z of [box.min.z,box.max.z])points.push(new Vector3(x,y,z));
 const colliders=[],matrix=new Matrix4(),origin=actor.position.clone();
 scene.traverse(o=>{if(!o.isInstancedMesh||!o.geometry.getAttribute('llLeafRank')||o.material.side!==FrontSide)return;
  for(let i=0;i<o.count;i++){o.getMatrixAt(i,matrix);matrix.premultiply(o.matrixWorld);const point=new Vector3().setFromMatrixPosition(matrix);
   if(Math.hypot(point.x-origin.x,point.z-origin.z)>10)continue;const copy=new Mesh(o.geometry,o.material);copy.matrixAutoUpdate=false;copy.matrixWorld.copy(matrix);colliders.push(copy);}});
 const yaw=actor.rotation.y+({front:0,side:-Math.PI/2,quarter:-Math.PI/4,back:Math.PI}[view]??0),probe=camera.clone(),ray=new Raycaster();let best=null;
 for(const d of [0,-.25,.25])for(const r of [1.4,2,2.7,3.5])for(const h of [.5,.9,1.3]){
  const radius=Math.max(1,extent)*r,position=origin.clone().add(new Vector3(Math.sin(yaw+d)*radius,Math.max(.35,extent*h),Math.cos(yaw+d)*radius));
  probe.position.copy(position);probe.lookAt(target);probe.updateMatrixWorld(true);
  if(points.some(v=>{const n=v.clone().project(probe);return Math.abs(n.x)>.9||Math.abs(n.y)>.9||n.z< -1||n.z>1;}))continue;
  let clear=0;for(const point of points){const dir=point.clone().sub(position);ray.set(position,dir.clone().normalize());ray.near=.02;ray.far=dir.length()-.08;if(!ray.intersectObjects(colliders,false).length)clear++;}
  const score=clear-.08*Math.abs(d)-.003*r-.001*h;if(!best||score>best.score)best={score,clear,total:points.length,position:position.sub(origin).toArray(),target:target.clone().sub(origin).toArray()};
 }
 return best;
}
WorldScene.prototype.applyCamera=function(state){
 applyCamera.call(this,state);qa.world=this;const terrain=observerStore.getSnapshot().terrain;
 const animal=terrain?.wildlife?.animals.find(a=>a.id===qa.id);if(!animal)return;
 if(terrain!==identity){ground.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);identity=terrain;}
 const actor=this.scene.getObjectByName('LifeLensNativeAnimal:'+animal.id);
 const x=actor?.position.x??(animal.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
 const z=actor?.position.z??(animal.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
 const y=actor?.position.y??ground.height(x,z);if(y===null)return;
 state.panX=x;state.panZ=z;applyCamera.call(this,state);
 if(actor&&qa.view!=='far'&&!qa.choices[qa.view])qa.choices[qa.view]=choose(this.scene,actor,qa.view,this.camera);
 const chosen=qa.choices[qa.view];
 if(chosen){this.camera.position.set(x+chosen.position[0],y+chosen.position[1],z+chosen.position[2]);this.camera.lookAt(x+chosen.target[0],y+chosen.target[1],z+chosen.target[2]);}
 else if(qa.view==='far'){this.camera.position.set(x+28,y+14,z+28);this.camera.lookAt(x,y+.4,z);}
 else{this.camera.position.set(x+2,y+1.6,z+3);this.camera.lookAt(x,y+.45,z);}
 this.camera.updateMatrixWorld();
};
qa.sample=()=>{
 const s=observerStore.getSnapshot(),animal=s.terrain?.wildlife?.animals.find(a=>a.id===qa.id),actor=qa.world?.scene.getObjectByName('LifeLensNativeAnimal:'+qa.id);
 return {minute:s.world.minute,worldSeed:s.world.worldSeed,runtime:s.runtime,
  core:animal?{id:animal.id,species:animal.species,sex:animal.sex,gridX:animal.gridX,gridY:animal.gridY,behavior:animal.behavior}:null,
  actor:actor?{id:actor.userData.coreId,position:actor.position.toArray(),clip:actor.userData.clip,tier:actor.userData.tier}:null,
  farIds:animal?qa.world?.scene.getObjectByName('LifeLensNativeFar:'+animal.species)?.userData.coreIds??[]:[],
  cameraChoice:qa.choices[qa.view]??null,renderStats:qa.renderer?{...qa.renderer.renderer.info.render}:null,
  canvas:qa.renderer?{id:qa.renderer.renderer.domElement.id,width:qa.renderer.renderer.domElement.width,height:qa.renderer.renderer.domElement.height,contextLost:qa.renderer.renderer.getContext().isContextLost()}:null};
};
qa.still=async()=>{qa.renderer.stop();qa.world.update(.016);await qa.renderer.renderer.compileAsync(qa.world.scene,qa.world.camera);qa.renderer.renderer.render(qa.world.scene,qa.world.camera);};
