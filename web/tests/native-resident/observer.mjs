import assert from 'node:assert/strict';
import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
const out=resolve(import.meta.dirname,'observer-review');mkdirSync(out,{recursive:true});
const server=spawn('python3',['-m','http.server','8771','--bind','127.0.0.1','--directory',resolve(import.meta.dirname,'preview')],{stdio:'ignore'});
let browser;const errors=[],httpErrors=[],samples=[];
try{
 for(let i=0;i<80;i++){try{if((await fetch('http://127.0.0.1:8771')).ok)break;}catch{}await new Promise(r=>setTimeout(r,200));}
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1000,height:760},deviceScaleFactor:1});
 page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
 page.on('response',r=>{if(r.status()>=400)httpErrors.push({status:r.status(),url:r.url()});});
 await page.addInitScript(()=>localStorage.setItem('lifelens.web.renderMode','three-world'));
 await page.goto('http://127.0.0.1:8771',{waitUntil:'domcontentloaded'});
 await page.waitForFunction(()=>window.__nativeResidentObserver?.store.getSnapshot().runtime.status==='ready',{},{timeout:180000});
 await page.evaluate(()=>{const q=window.__nativeResidentObserver;q.actions.setSimulationSpeed(0);q.actions.createWorld('77');q.actions.setSimulationSpeed(0);});
 await page.waitForFunction(()=>window.__nativeResidentObserver.sample().actors.length===4&&window.__nativeResidentObserver.sample().actors.every(a=>a.native),{},{timeout:180000});
 const initial=await page.evaluate(()=>window.__nativeResidentObserver.sample());samples.push(initial);
 assert.equal(initial.actors.filter(a=>a.sex==='Male').length,2);assert.equal(initial.actors.filter(a=>a.sex==='Female').length,2);
 assert(initial.actors.every(a=>a.version==='MakeHuman-native-v1'));
 for(const sex of ['Female','Male'])for(const view of ['front','side']){
  const data=await page.evaluate(async({sex,view})=>{const q=window.__nativeResidentObserver;q.focusId=q.sample().actors.find(a=>a.sex===sex).id;q.view=view;return q.capture();},{sex,view});
  writeFileSync(resolve(out,sex+'-'+view+'.jpg'),Buffer.from(data.split(',')[1],'base64'));
 }
 await page.evaluate(()=>{const q=window.__nativeResidentObserver;q.renderer.start();q.actions.setSimulationSpeed(4);});
 await page.waitForFunction(start=>window.__nativeResidentObserver.store.getSnapshot().world.minute>start+10,initial.minute,{timeout:120000});
 await page.evaluate(()=>window.__nativeResidentObserver.actions.setSimulationSpeed(0));
 samples.push(await page.evaluate(()=>window.__nativeResidentObserver.sample()));
 assert.equal(errors.length,0,JSON.stringify(errors));assert.equal(httpErrors.length,0,JSON.stringify(httpErrors));
 writeFileSync(resolve(out,'validation.json'),JSON.stringify({status:'PASS_NATIVE_RESIDENT_ACTUAL_REACT_WASM_OBSERVER',samples,errors,httpErrors,physicalAndroid:'NOT_MEASURED'},null,2)+'\n');
 console.log('PASS actual React production app -> WASM -> four Core residents -> new source-derived male/female skins');
}catch(error){writeFileSync(resolve(out,'validation.json'),JSON.stringify({status:'FAIL_NATIVE_RESIDENT_ACTUAL_OBSERVER',error:String(error),samples,errors,httpErrors},null,2)+'\n');throw error;}
finally{await browser?.close();server.kill();}
