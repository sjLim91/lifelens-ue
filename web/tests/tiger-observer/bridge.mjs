// Camera and measurement instrumentation for the actual production application.
import {observerStore} from '../../src/state/observer-store.ts';
import {observerActions} from '../../src/state/observer-actions.ts';
import {WorldScene} from '../../src/render/world-scene.ts';
import {WorldRenderer} from '../../src/render/world-renderer.ts';
import {ResidentGroundSurface} from '../../src/render/resident-grounding.ts';
import {WORLD_GRID_CONTRACT as grid} from '../../src/runtime/lifelens-contract.ts';
import {Vector3} from 'three';
import {ImportedTigerPool} from '../../src/render/imported-tiger.ts';
   const qa=window.__tigerObserver={store:observerStore,actions:observerActions,world:null,view:'quarter',id:null,baseline:false,faceTarget:null};
   const draw=ImportedTigerPool.prototype.draw;
   ImportedTigerPool.prototype.draw=function(...args){return qa.baseline?false:draw.apply(this,args);};
   const setCamera=WorldRenderer.prototype.setCamera;
   WorldRenderer.prototype.setCamera=function(state){qa.renderer=this;setCamera.call(this,state);};
   const original=WorldScene.prototype.applyCamera;
   const ground=new ResidentGroundSurface();let terrainIdentity=null;
   const headPoint=new Vector3();
   const views={front:[0,1.3,4.4],side:[4.4,1.3,0],quarter:[3,1.8,4],back:[0,1.3,-4.4],face:[.5,1.5,1.8]};
   WorldScene.prototype.applyCamera=function(state){
    original.call(this,state);qa.world=this;
    const terrain=observerStore.getSnapshot().terrain;
    if(!qa.id||!terrain)return;
    const tiger=terrain.wildlife?.animals.find(animal=>animal.id===qa.id);if(!tiger)return;
    if(terrain!==terrainIdentity){ground.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);terrainIdentity=terrain;}
    const actor=this.scene.getObjectByName(`LifeLensTiger:${tiger.id}`);
    const x=actor?.position.x??(tiger.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
    const z=actor?.position.z??(tiger.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
    const y=actor?.position.y??ground.height(x,z);if(y===null)return;
    // Match the ordinary observer's pan/focus state so its existing foliage
    // clarity follows the actual animal; do not remove or move vegetation.
    state.panX=x;state.panZ=z;original.call(this,state);
    if(qa.view==='face'){
     const head=actor?.getObjectByName('Head');
     if(head){
      head.getWorldPosition(headPoint);headPoint.y+=.07;headPoint.z+=.1;
      qa.faceTarget=headPoint.clone();
     }
     if(qa.faceTarget){
      headPoint.copy(qa.faceTarget);
      this.camera.position.set(headPoint.x+.9,headPoint.y+.25,headPoint.z+1.6);
      this.camera.lookAt(headPoint);this.camera.updateMatrixWorld();return;
     }
    }
    const v=views[qa.view]??views.quarter;
    this.camera.position.set(x+v[0],y+v[1],z+v[2]);this.camera.lookAt(x,y+.65,z);this.camera.updateMatrixWorld();
   };
   qa.sample=()=>{
    const snapshot=observerStore.getSnapshot();const tiger=snapshot.terrain?.wildlife?.animals.find(animal=>animal.id===qa.id);
    const actor=qa.world?.scene.getObjectByName(`LifeLensTiger:${qa.id}`);
    return {minute:snapshot.world.minute,worldSeed:snapshot.world.worldSeed,
     core:tiger?{id:tiger.id,species:tiger.species,gridX:tiger.gridX,gridY:tiger.gridY,behavior:tiger.behavior}:null,
     actor:actor?{id:actor.userData.coreId,position:actor.position.toArray(),clip:actor.userData.clip,tier:actor.userData.tier}:null,
     runtime:snapshot.runtime,simulationSpeed:snapshot.simulationSpeed,
     renderStats:qa.renderer?{...qa.renderer.renderer.info.render}:null,
     renderMemory:qa.renderer?{...qa.renderer.renderer.info.memory}:null,
     canvas:qa.renderer?{width:qa.renderer.renderer.domElement.width,height:qa.renderer.renderer.domElement.height,
      contextLost:qa.renderer.renderer.getContext().isContextLost()}:null};
   };
   qa.still=async()=>{
    qa.renderer.stop();console.log('LL_TIGER_STAGE world-update-start',qa.view);
    qa.world.update(.016);console.log('LL_TIGER_STAGE world-update-done');
    await qa.renderer.renderer.compileAsync(qa.world.scene,qa.world.camera);
    console.log('LL_TIGER_STAGE shader-compile-done');
    qa.renderer.renderer.render(qa.world.scene,qa.world.camera);
    console.log('LL_TIGER_STAGE scene-render-done');
   };

