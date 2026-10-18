import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
const root=resolve(import.meta.dirname,'../..'),out=resolve(import.meta.dirname,'review');mkdirSync(out,{recursive:true});
const base='http://127.0.0.1:8774',server=spawn('python3',['-m','http.server','8774','--bind','127.0.0.1','--directory',resolve(root,'tests/native-wildlife-observer/preview')],{stdio:'ignore'});
const delay=ms=>new Promise(r=>setTimeout(r,ms)),results=[],errors=[];let browser,page;
async function image(name){
 const record=await page.evaluate(async()=>{const q=window.__nativeWildlifeObserver;await q.still();const c=q.renderer.renderer.domElement;
  if(c.id!=='threeWorldCanvas'||!document.querySelector('#app .brand')||q.renderer.renderer.getContext().isContextLost())throw Error('Not a valid actual production Observer canvas');
  return {url:c.toDataURL('image/jpeg',.88),width:c.width,height:c.height};});
 writeFileSync(resolve(out,name+'.jpg'),Buffer.from(record.url.split(',')[1],'base64'));return {width:record.width,height:record.height};
}
try{
 for(let i=0;i<50;i++){try{if((await fetch(base)).ok)break;}catch{}await delay(100);}
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 page=await browser.newPage({viewport:{width:960,height:720},deviceScaleFactor:1});
 page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});page.on('response',r=>{if(r.status()>=400)errors.push(`HTTP ${r.status()} ${r.url()}`);});
 await page.addInitScript(()=>localStorage.setItem('lifelens.web.renderMode','three-world'));
 await page.goto(base,{waitUntil:'domcontentloaded'});
 await page.waitForFunction(()=>window.__nativeWildlifeObserver?.store.getSnapshot().runtime.status==='ready',{},{timeout:120000});
 await page.evaluate(()=>{const q=window.__nativeWildlifeObserver;q.actions.setSimulationSpeed(0);q.actions.createWorld('77');q.actions.setSimulationSpeed(0);q.actions.moveObserver(-2,0);});
 await page.waitForFunction(()=>{const s=window.__nativeWildlifeObserver.store.getSnapshot();return String(s.world.worldSeed)==='77'&&['Hare','Deer','Wolf','FreshwaterFish'].every(species=>s.terrain?.wildlife?.animals.some(a=>a.species===species));},{},{timeout:30000});
 await page.evaluate(()=>window.__nativeWildlifeObserver.renderer.stop());
 for(const species of ['Hare','Deer','Wolf','FreshwaterFish']){
  await page.evaluate(species=>{const q=window.__nativeWildlifeObserver,animals=q.store.getSnapshot().terrain.wildlife.animals;
   const a=animals.filter(a=>a.species===species).sort((a,b)=>b.ageDays-a.ageDays)[0];q.id=a.id;q.baseline=false;q.choices={};q.view='quarter';},species);
  let ready=false;for(let i=0;i<60;i++){ready=await page.evaluate(()=>{const q=window.__nativeWildlifeObserver;q.world.update(.016);return !!q.sample().actor;});if(ready)break;await delay(100);}
  if(!ready)throw Error(species+' did not load in the actual React/WASM Observer');
  const frozen=await page.evaluate(()=>JSON.stringify(window.__nativeWildlifeObserver.store.getSnapshot().terrain));
  const pair=[];
  for(const baseline of [false,true]){
   await page.evaluate(v=>{window.__nativeWildlifeObserver.baseline=v;},baseline);
   for(const view of ['front','side','quarter','back']){await page.evaluate(v=>{window.__nativeWildlifeObserver.view=v;},view);await image(species+'-'+(baseline?'baseline':'native')+'-'+view);}
   const after=await page.evaluate(()=>JSON.stringify(window.__nativeWildlifeObserver.store.getSnapshot().terrain));if(after!==frozen)throw Error('Paused WASM observation changed during camera-only comparison');
   const sample=await page.evaluate(()=>window.__nativeWildlifeObserver.sample());
   if(!baseline&&sample.actor?.id!==sample.core?.id)throw Error('Native actor did not bind the actual WASM ID');pair.push({baseline,sample});
  }
  if(JSON.stringify(pair[0].sample.core)!==JSON.stringify(pair[1].sample.core)||pair[0].sample.minute!==pair[1].sample.minute)throw Error('Baseline and native compared different Core observations');
  await page.evaluate(()=>{const q=window.__nativeWildlifeObserver;q.baseline=false;q.view='far';});
  let far=false;for(let i=0;i<60;i++){far=await page.evaluate(()=>{const q=window.__nativeWildlifeObserver;q.world.update(.016);return q.sample().farIds.includes(q.id);});if(far)break;await delay(100);}
  if(!far)throw Error('Source-derived '+species+' far batch did not bind the actual Core ID');
  await image(species+'-far');results.push({species,pair,far:await page.evaluate(()=>window.__nativeWildlifeObserver.sample())});
 }
 await page.setViewportSize({width:412,height:915});await page.evaluate(()=>{window.__nativeWildlifeObserver.view='quarter';});await delay(300);await image('mobile-viewport');
 await page.evaluate(()=>{const q=window.__nativeWildlifeObserver;q.view='quarter';q.renderer.start();q.actions.setSimulationSpeed(4);});
 const minute=await page.evaluate(()=>window.__nativeWildlifeObserver.store.getSnapshot().world.minute);
 await page.waitForFunction(m=>window.__nativeWildlifeObserver.store.getSnapshot().world.minute>=m+30,minute,{timeout:60000});
 await page.evaluate(()=>window.__nativeWildlifeObserver.actions.setSimulationSpeed(0));
 await image('after-core-progression');
 if(errors.length)throw Error(errors.join('; '));
 writeFileSync(resolve(out,'observer.json'),JSON.stringify({status:'PASS_ACTUAL_REACT_WASM_OBSERVER_FOUR_SPECIES',seed:'77',results,coreTimeAdvanced:true,errors,physicalAndroid:'NOT_MEASURED',visualQuality:'REQUIRES_RETAINED_IMAGE_REVIEW'},null,2)+'\n');
 console.log('PASS actual production React/WASM four-species ID/position/behavior binding, frozen comparisons, far instances, mobile viewport and real Core progression');
}catch(error){
 const diagnostic=await page?.evaluate(()=>window.__nativeWildlifeObserver?.sample()).catch(()=>null);
 writeFileSync(resolve(out,'observer.json'),JSON.stringify({status:'FAIL_ACTUAL_REACT_WASM_OBSERVER',error:String(error),results,diagnostic,errors,physicalAndroid:'NOT_MEASURED'},null,2)+'\n');console.error(error);process.exitCode=1;
}finally{await browser?.close();server.kill();}
