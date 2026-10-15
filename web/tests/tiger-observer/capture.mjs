import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync,readFileSync} from 'node:fs';
import {resolve} from 'node:path';

// Runs the real React application and its WASM Core. The test changes camera
// presentation only; new-game, pause and time progression use existing actions.
const root=resolve(import.meta.dirname,'../..');
const out=resolve(import.meta.dirname,'review');mkdirSync(out,{recursive:true});
const port=8766;const base=`http://127.0.0.1:${port}`;
const server=spawn(process.execPath,[resolve(root,'node_modules/vite/bin/vite.js'),'--host','127.0.0.1','--port',String(port)],{cwd:root,stdio:'ignore'});
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function bounded(promise,ms,label){
 let timer;try{return await Promise.race([promise,new Promise((_,reject)=>{
  timer=setTimeout(()=>reject(Error(`${label} timed out after ${ms}ms`)),ms);
 })]);}finally{clearTimeout(timer);}
}
async function captureObserverCanvas(page,path){
 const result=await bounded(page.evaluate(async()=>{
  const q=window.__tigerObserver;await q.still();
  const canvas=q.renderer.renderer.domElement;
  if(canvas.id!=='threeWorldCanvas'||!document.querySelector('#app .brand'))throw Error('Capture is not the actual Observer canvas');
  if(q.renderer.renderer.getContext().isContextLost())throw Error('Actual Observer WebGL context was lost');
  if(canvas.width*canvas.height>4_000_000)throw Error('Actual Observer canvas exceeds the capture pixel budget');
  console.log('LL_TIGER_STAGE canvas-export-start',canvas.width,canvas.height);
  const data=canvas.toDataURL('image/jpeg',.88);console.log('LL_TIGER_STAGE canvas-export-done',data.length);
  return {data,width:canvas.width,height:canvas.height,canvasId:canvas.id};
 }),90000,'Actual Observer canvas export');
 if(!result.data.startsWith('data:image/jpeg;base64,'))throw Error('Observer canvas could not be exported');
 writeFileSync(path,Buffer.from(result.data.split(',')[1],'base64'));
 return {width:result.width,height:result.height,canvasId:result.canvasId};
}
let browser;let activePage;const results=[];const errors=[];
try{
 for(let i=0;i<50;i++){try{if((await fetch(base)).ok)break;}catch{}await delay(100);}
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 for(const baseline of [true,false]){
  const page=await browser.newPage({viewport:{width:960,height:720},deviceScaleFactor:1});
  activePage=page;
  page.on('console',message=>{if(message.text().startsWith('LL_TIGER_STAGE'))console.log(message.text());});
  page.on('pageerror',error=>errors.push({baseline,error:String(error)}));
  if(baseline)await page.route('**/vendor/wildlife/*.glb',route=>route.abort('blockedbyclient'));
  await page.addInitScript(()=>localStorage.setItem('lifelens.web.renderMode','three-world'));
  await page.goto(base,{waitUntil:'domcontentloaded'});
  await page.evaluate(async()=>{
   const {observerStore}=await import('/src/state/observer-store.ts');
   const {observerActions}=await import('/src/state/observer-actions.ts');
   const {WorldScene}=await import('/src/render/world-scene.ts');
   const {WorldRenderer}=await import('/src/render/world-renderer.ts');
   const {ResidentGroundSurface}=await import('/src/render/resident-grounding.ts');
   const {WORLD_GRID_CONTRACT:grid}=await import('/src/runtime/lifelens-contract.ts');
   const qa=window.__tigerObserver={store:observerStore,actions:observerActions,world:null,view:'quarter',id:null};
   const setCamera=WorldRenderer.prototype.setCamera;
   WorldRenderer.prototype.setCamera=function(state){qa.renderer=this;setCamera.call(this,state);};
   const original=WorldScene.prototype.applyCamera;
   const ground=new ResidentGroundSurface();let terrainIdentity=null;
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
  });
  await page.waitForFunction(()=>window.__tigerObserver?.store.getSnapshot().runtime.status==='ready',{},{timeout:120000});
  await page.evaluate(()=>{const q=window.__tigerObserver;q.actions.createWorld('77');q.actions.setSimulationSpeed(0);q.actions.moveObserver(-2,0);});
  await page.waitForFunction(()=>{const s=window.__tigerObserver.store.getSnapshot();return String(s.world.worldSeed)==='77'&&s.terrain?.wildlife?.animals.some(a=>a.species==='Tiger');},{},{timeout:30000});
  await page.evaluate(()=>{const q=window.__tigerObserver;q.renderer.stop();q.id=q.store.getSnapshot().terrain.wildlife.animals.find(a=>a.species==='Tiger').id;});
  for(let i=0;i<30;i++){
   const ready=await bounded(page.evaluate(()=>{const q=window.__tigerObserver;q.world.update(.016);return Boolean(q.sample().actor);}),30000,'Observer model priming');
   if(baseline||ready)break;await delay(100);
  }
  if(!baseline&&!await page.evaluate(()=>Boolean(window.__tigerObserver.sample().actor)))throw Error('Actual Observer imported tiger did not load');
  writeFileSync(resolve(out,`${baseline?'baseline':'candidate'}-pre-capture.json`),JSON.stringify(await page.evaluate(()=>window.__tigerObserver.sample()),null,2)+'\n');
  const frozen=await page.evaluate(()=>JSON.stringify(window.__tigerObserver.store.getSnapshot().terrain));
  for(const view of ['front','side','quarter','back','face']){
   await page.evaluate(view=>{window.__tigerObserver.view=view;},view);
   await captureObserverCanvas(page,resolve(out,`${baseline?'baseline':'candidate'}-${view}.jpg`));
  }
  const after=await page.evaluate(()=>JSON.stringify(window.__tigerObserver.store.getSnapshot().terrain));
  if(after!==frozen)throw Error('Paused Core observation changed during camera-only captures');
  const sample=await page.evaluate(()=>window.__tigerObserver.sample());
  if(!baseline&&sample.actor?.id!==sample.core?.id)throw Error('Observer skin identity differs from WASM observation');
  const result={baseline,pausedCameraObservationUnchanged:true,sample,motion:[]};
  if(!baseline){
   await page.evaluate(()=>{const q=window.__tigerObserver;q.view='quarter';q.renderer.start();q.actions.setSimulationSpeed(4);});
   await page.waitForFunction(start=>window.__tigerObserver.store.getSnapshot().world.minute>=start+60,sample.minute,{timeout:60000});
   await page.evaluate(()=>window.__tigerObserver.actions.setSimulationSpeed(0));
   result.motion=await page.evaluate(()=>new Promise(resolve=>{
    const samples=[];const record=()=>{samples.push(window.__tigerObserver.sample());if(samples.length<30)requestAnimationFrame(record);else resolve(samples);};record();
   }));
   result.motionObserved=result.motion.some(frame=>frame.actor&&['walk','run','hunt'].includes(frame.actor.clip));
   await captureObserverCanvas(page,resolve(out,'candidate-after-core-hour.jpg'));
   await page.setViewportSize({width:412,height:915});await delay(500);
   result.mobileCanvas=await captureObserverCanvas(page,resolve(out,'candidate-mobile-viewport.jpg'));
   result.mobileViewportOnly=true;
  }
  results.push(result);await page.close();activePage=null;
 }
 const result={status:errors.length?'FAIL_OBSERVER_RUNTIME_ERRORS':'PASS_REACT_WASM_CANVAS_RENDER_PATH_ONLY',
  captureMethod:'Actual React Observer threeWorldCanvas export immediately after its existing WorldRenderer renders',
  source:'PINNED_SOURCE_DERIVATIVE',seed:'77',generationVersion:4,results,errors,
  android:'NOT_MEASURED',visualQuality:'REQUIRES_IMAGE_REVIEW',mergeAllowed:false,
  receipts:JSON.parse(readFileSync(resolve(root,'public/vendor/wildlife/receipt.json'),'utf8'))};
 writeFileSync(resolve(out,'observer-validation.json'),JSON.stringify(result,null,2)+'\n');
 console.log(JSON.stringify({status:result.status,motionObserved:results[1]?.motionObserved,errors}));
 if(errors.length)process.exitCode=1;
}catch(error){
 const diagnostic=await bounded(activePage?.evaluate(()=>window.__tigerObserver?.sample?.()),3000,'Failure diagnostics').catch(()=>null);
 writeFileSync(resolve(out,'observer-validation.json'),JSON.stringify({status:'FAIL_REACT_WASM_CAPTURE',error:String(error),diagnostic,results,errors,android:'NOT_MEASURED',mergeAllowed:false},null,2)+'\n');
 console.error(error);process.exitCode=1;
}finally{await browser?.close();server.kill();}
