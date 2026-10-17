// Camera and measurement instrumentation for the actual production application.
import {observerStore} from '../../src/state/observer-store.ts';
import {observerActions} from '../../src/state/observer-actions.ts';
import {WorldScene} from '../../src/render/world-scene.ts';
import {WorldRenderer} from '../../src/render/world-renderer.ts';
import {ResidentGroundSurface} from '../../src/render/resident-grounding.ts';
import {WORLD_GRID_CONTRACT as grid} from '../../src/runtime/lifelens-contract.ts';
import {Vector3} from 'three';
import {ImportedTigerPool} from '../../src/render/imported-tiger.ts';
import {chooseTigerCamera} from './camera-visibility.mjs';
   const qa=window.__tigerObserver={reviewCommit:import.meta.env.VITE_REVIEW_COMMIT,store:observerStore,actions:observerActions,world:null,view:'quarter',id:null,baseline:false,faceTarget:null,cameraChoices:{}};
   const draw=ImportedTigerPool.prototype.draw;
   ImportedTigerPool.prototype.draw=function(...args){return qa.baseline?false:draw.apply(this,args);};
   const setCamera=WorldRenderer.prototype.setCamera;
   WorldRenderer.prototype.setCamera=function(state){qa.renderer=this;setCamera.call(this,state);};
   const original=WorldScene.prototype.applyCamera;
   const ground=new ResidentGroundSurface();let terrainIdentity=null;
   const headPoint=new Vector3();
   const views={front:[0,1.3,4.4],side:[-4.4,1.3,0],quarter:[-3,1.8,4],back:[0,2.3,-4.4],far:[26,14,26],face:[.5,1.5,1.8]};
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
    if(actor&&['front','side','quarter','back'].includes(qa.view)&&!qa.cameraChoices[qa.view]){
     qa.cameraChoices[qa.view]=chooseTigerCamera(this.scene,actor,qa.view);
    }
    const chosen=qa.cameraChoices[qa.view];
    if(chosen){
     this.camera.position.set(x+chosen.position[0],y+chosen.position[1],z+chosen.position[2]);
     this.camera.lookAt(x+chosen.target[0],y+chosen.target[1],z+chosen.target[2]);this.camera.updateMatrixWorld();return;
    }
    if(qa.view==='face'){
     const head=actor?.getObjectByName(actor.userData.headJoint??'Head');
     if(head){
      head.getWorldPosition(headPoint);headPoint.y+=.07;headPoint.z+=.1;
      qa.faceTarget=headPoint.clone();
     }
     if(qa.faceTarget){
      headPoint.copy(qa.faceTarget);
      this.camera.position.set(headPoint.x-.9,headPoint.y+.25,headPoint.z+1.6);
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
     cameraChoice:qa.cameraChoices[qa.view]??null,
     farIds:qa.world?.scene.getObjectByName('LifeLensTigerFarInstances')?.userData.coreIds??[],
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

// Present only on the explicit Draft review URL. Measure the running Observer,
// recording hardware context instead of treating a mobile viewport as a phone.
if(new URLSearchParams(location.search).get('review')==='1'){
 addEventListener('DOMContentLoaded',()=>{
  const panel=document.createElement('aside');
  panel.style.cssText='position:fixed;bottom:8px;left:8px;z-index:10000;max-width:340px;padding:10px;background:#10251fee;color:white;font:13px/1.5 system-ui;border-radius:8px';
  const label=document.createElement('div');label.textContent='검증용 미리보기 · 최종 품질 검증 전';panel.append(label);
  const status=document.createElement('div');status.textContent='Android 성능은 실기기에서 측정해 주세요.';
  const button=(title,run)=>{const b=document.createElement('button');b.textContent=title;b.style.margin='4px';b.onclick=run;panel.append(b);return b;};
  button('현재 호랑이 보기',()=>{
   const tiger=qa.store.getSnapshot().terrain?.wildlife?.animals.find(a=>a.species==='Tiger');
   if(!tiger){status.textContent='현재 관찰 영역에 호랑이가 없습니다.';return;}
   qa.id=tiger.id;qa.cameraChoices={};qa.view='face';qa.renderer?.start();status.textContent=`Core ID ${tiger.id}`;
  });
  button('가까이 / 멀리',()=>{qa.view=qa.view==='far'?'face':'far';});
  button('자유 카메라',()=>{qa.id=null;});
  const measure=button('30초 성능 측정',async()=>{
   if(!qa.renderer||!qa.world){status.textContent='월드를 불러온 뒤 측정해 주세요.';return;}
   measure.disabled=true;status.textContent='측정 중 · 이 창을 화면에 유지해 주세요.';
   const renderer=qa.renderer.renderer,gl=renderer.getContext(),extension=gl.getExtension('WEBGL_debug_renderer_info');
   const startSample=qa.sample(),times=[];let previous=performance.now();const start=previous;
   qa.renderer.start();
   await new Promise(resolve=>{const frame=now=>{if(now-start>2000)times.push(now-previous);previous=now;if(now-start<32000)requestAnimationFrame(frame);else resolve();};requestAnimationFrame(frame);});
   const sorted=times.slice().sort((a,b)=>a-b),mean=times.reduce((a,b)=>a+b,0)/times.length;
   const result={status:'USER_BROWSER_MEASUREMENT_REQUIRES_DEVICE_CONFIRMATION',commit:import.meta.env.VITE_REVIEW_COMMIT??'unknown',
    measuredAt:new Date().toISOString(),samples:times.length,durationSeconds:30,fps:1000/mean,
    frameMsP50:sorted[Math.floor(sorted.length*.5)],frameMsP95:sorted[Math.floor(sorted.length*.95)],
    physicalDevice:'USER_CONFIRMATION_REQUIRED',userAgent:navigator.userAgent,devicePixelRatio,
    viewport:[innerWidth,innerHeight],canvas:[renderer.domElement.width,renderer.domElement.height],
    gpu:extension?gl.getParameter(extension.UNMASKED_RENDERER_WEBGL):null,
    visibleAtEnd:document.visibilityState,start:startSample,end:qa.sample(),
    visibleWildlife:qa.store.getSnapshot().terrain?.wildlife?.animals.length,mergeAllowed:false};
   const blob=new Blob([JSON.stringify(result,null,2)],{type:'application/json'}),url=URL.createObjectURL(blob);
   const a=document.createElement('a');a.href=url;a.download='lifelens-tiger-device-performance.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),10000);
   status.textContent=`${result.fps.toFixed(1)} FPS · p95 ${result.frameMsP95.toFixed(1)} ms · JSON 저장됨`;measure.disabled=false;
  });
  panel.append(status);document.body.append(panel);
 });
}

