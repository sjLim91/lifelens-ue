import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync,readFileSync} from 'node:fs';
import {resolve} from 'node:path';

// Runs the real React application and its WASM Core. The test changes camera
// presentation and the existing fallback only; new-game, pause and time progression use existing actions.
const root=resolve(import.meta.dirname,'../..');
const out=resolve(import.meta.dirname,'review');mkdirSync(out,{recursive:true});
const port=8766;const base=`http://127.0.0.1:${port}`;
const server=spawn('python3',['-m','http.server',String(port),'--bind','127.0.0.1','--directory',resolve(root,'tests/tiger-observer/preview')],{cwd:root,stdio:'ignore'});
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
let browser;let activePage;let baseline=false;const results=[];const errors=[];
try{
 for(let i=0;i<50;i++){try{if((await fetch(base)).ok)break;}catch{}await delay(100);}
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
  const page=await browser.newPage({viewport:{width:960,height:720},deviceScaleFactor:1});
  activePage=page;
  page.on('console',message=>{
   const text=message.text();if(text.startsWith('LL_TIGER_STAGE'))console.log(text);
   if(message.type()==='error'&&!(baseline&&text.includes('ERR_BLOCKED_BY_CLIENT')))
    errors.push({baseline,error:text,location:message.location()});
  });
  page.on('pageerror',error=>errors.push({baseline,error:String(error)}));
  await page.addInitScript(()=>localStorage.setItem('lifelens.web.renderMode','three-world'));
  await page.goto(base,{waitUntil:'domcontentloaded'});
  await page.waitForFunction(()=>Boolean(window.__tigerObserver),{},{timeout:60000});
  await page.waitForFunction(()=>window.__tigerObserver?.store.getSnapshot().runtime.status==='ready',{},{timeout:120000});
  await page.evaluate(()=>{const q=window.__tigerObserver;q.actions.setSimulationSpeed(0);q.actions.createWorld('77');q.actions.setSimulationSpeed(0);q.actions.moveObserver(-2,0);});
  await page.waitForFunction(()=>{const s=window.__tigerObserver.store.getSnapshot();return String(s.world.worldSeed)==='77'&&s.terrain?.wildlife?.animals.some(a=>a.species==='Tiger');},{},{timeout:30000});
  await page.evaluate(()=>{const q=window.__tigerObserver;q.renderer.stop();q.id=q.store.getSnapshot().terrain.wildlife.animals.find(a=>a.species==='Tiger').id;});
  for(let i=0;i<30;i++){
   const ready=await bounded(page.evaluate(()=>{const q=window.__tigerObserver;q.world.update(.016);return Boolean(q.sample().actor);}),30000,'Observer model priming');
   if(baseline||ready)break;await delay(100);
  }
  if(!baseline&&!await page.evaluate(()=>Boolean(window.__tigerObserver.sample().actor)))throw Error('Actual Observer imported tiger did not load');
  const frozen=await page.evaluate(()=>JSON.stringify(window.__tigerObserver.store.getSnapshot().terrain));
  for(baseline of [false,true]){
   await page.evaluate(baseline=>{const q=window.__tigerObserver;q.baseline=baseline;q.world.update(.016);},baseline);
  writeFileSync(resolve(out,`${baseline?'baseline':'candidate'}-pre-capture.json`),JSON.stringify(await page.evaluate(()=>window.__tigerObserver.sample()),null,2)+'\n');
  for(const view of ['front','side','quarter','back','face']){
   await page.evaluate(view=>{window.__tigerObserver.view=view;},view);
   await captureObserverCanvas(page,resolve(out,`${baseline?'baseline':'candidate'}-${view}.jpg`));
  }
  const after=await page.evaluate(()=>JSON.stringify(window.__tigerObserver.store.getSnapshot().terrain));
  if(after!==frozen)throw Error('Paused Core observation changed during camera-only captures');
  const sample=await page.evaluate(()=>window.__tigerObserver.sample());
  if(!baseline&&sample.actor?.id!==sample.core?.id)throw Error('Observer skin identity differs from WASM observation');
  const result={baseline,pausedCameraObservationUnchanged:true,sample,motion:[]};
  results.push(result);
  }
  baseline=false;
  const candidate=results.find(result=>!result.baseline);
  if(candidate){
   await page.evaluate(()=>{const q=window.__tigerObserver;q.baseline=false;q.view='quarter';q.world.update(.016);q.renderer.start();q.actions.setSimulationSpeed(4);});
   await page.waitForFunction(start=>window.__tigerObserver.store.getSnapshot().world.minute>=start+60,candidate.sample.minute,{timeout:60000});
   await page.evaluate(()=>window.__tigerObserver.actions.setSimulationSpeed(0));
   candidate.motion=await page.evaluate(()=>new Promise(resolve=>{
    const samples=[];const record=()=>{samples.push(window.__tigerObserver.sample());if(samples.length<30)requestAnimationFrame(record);else resolve(samples);};record();
   }));
   candidate.motionObserved=candidate.motion.some(frame=>frame.actor&&['walk','run','hunt'].includes(frame.actor.clip));
   await captureObserverCanvas(page,resolve(out,'candidate-after-core-hour.jpg'));
   await page.setViewportSize({width:412,height:915});await delay(500);
   candidate.mobileCanvas=await captureObserverCanvas(page,resolve(out,'candidate-mobile-viewport.jpg'));
   candidate.mobileViewportOnly=true;
  }
  await page.close();activePage=null;
 if(results[0]?.sample.minute!==results[1]?.sample.minute
   ||JSON.stringify(results[0]?.sample.core)!==JSON.stringify(results[1]?.sample.core))
  errors.push({error:'Baseline and candidate must use the same paused Core minute and observation'});
 const result={status:errors.length?'FAIL_OBSERVER_RUNTIME_ERRORS':'PASS_REACT_WASM_CANVAS_RENDER_PATH_ONLY',
  captureMethod:'Actual React Observer threeWorldCanvas export immediately after its existing WorldRenderer renders',
  source:'PINNED_SOURCE_DERIVATIVE',applicationBuild:'PRODUCTION_REACT',seed:'77',generationVersion:4,results,errors,
  android:'NOT_MEASURED',visualQuality:'REQUIRES_IMAGE_REVIEW',mergeAllowed:false,
  receipts:JSON.parse(readFileSync(resolve(root,'public/vendor/wildlife/receipt.json'),'utf8'))};
 writeFileSync(resolve(out,'observer-validation.json'),JSON.stringify(result,null,2)+'\n');
 console.log(JSON.stringify({status:result.status,motionObserved:results.find(result=>!result.baseline)?.motionObserved,errors}));
 if(errors.length)process.exitCode=1;
}catch(error){
 const diagnostic=await bounded(activePage?.evaluate(()=>window.__tigerObserver?.sample?.()),3000,'Failure diagnostics').catch(()=>null);
 writeFileSync(resolve(out,'observer-validation.json'),JSON.stringify({status:'FAIL_REACT_WASM_CAPTURE',error:String(error),diagnostic,results,errors,android:'NOT_MEASURED',mergeAllowed:false},null,2)+'\n');
 console.error(error);process.exitCode=1;
}finally{await browser?.close();server.kill();}
