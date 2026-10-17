import assert from 'node:assert/strict';
import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
const out=resolve(import.meta.dirname,'visibility-review');mkdirSync(out,{recursive:true});
const server=spawn('python3',['-m','http.server','8771','--bind','127.0.0.1','--directory',resolve(import.meta.dirname,'preview')],{stdio:'ignore'});
let browser;const results=[];
try{
 for(let i=0;i<80;i++){try{if((await fetch('http://127.0.0.1:8771')).ok)break;}catch{}await new Promise(r=>setTimeout(r,200));}
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 for(const [name,width,height] of [['mobile',412,915],['desktop',1000,760]]){
  const page=await browser.newPage({viewport:{width,height},deviceScaleFactor:1});
  const errors=[];
  let releaseAssets;const assetGate=new Promise(resolve=>releaseAssets=resolve);
  await page.route('**/resident-*.glb',async route=>{await assetGate;await route.continue();});
  page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  await page.goto('http://127.0.0.1:8771',{waitUntil:'domcontentloaded'});
  await page.waitForFunction(()=>window.__nativeResidentObserver?.store.getSnapshot().runtime.status==='ready',{},{timeout:180000});
  await page.evaluate(()=>{const q=window.__nativeResidentObserver;q.actions.createWorld('77');q.actions.setSimulationSpeed(0);});
  await page.waitForFunction(()=>window.__nativeResidentObserver.sample().actors.length===4,{},{timeout:90000});
  const whileLoading=await page.evaluate(()=>window.__nativeResidentObserver.sample());
  assert(whileLoading.actors.every(a=>!a.native&&a.visible&&a.bodyVisible),'Held assets must remain pending');
  releaseAssets();
  await page.waitForFunction(()=>window.__nativeResidentObserver.sample().actors.every(a=>a.native),{},{timeout:180000});
  const result=await Promise.race([page.evaluate(()=>window.__nativeResidentObserver.defaultView()),new Promise((_,reject)=>setTimeout(()=>reject(Error('Default frame capture timed out')),120000))]);
  writeFileSync(resolve(out,name+'.jpg'),Buffer.from(result.data.split(',')[1],'base64'));delete result.data;results.push({name,whileLoading,...result,errors});
  writeFileSync(resolve(out,'validation.json'),JSON.stringify(results,null,2));
  assert.equal(result.sample.actors.length,4);
  assert(result.projected.every(a=>a.visible&&Math.abs(a.xy[0])<1&&Math.abs(a.xy[1])<1&&a.heightPixels>=24),'Default camera must show legible residents');
  assert(result.pixels>100,'People must contribute actual visible pixels through scene occlusion');
  assert.equal(errors.length,0,JSON.stringify(errors));
  await page.close();
 }
 console.log(JSON.stringify(results));writeFileSync(resolve(out,'validation.json'),JSON.stringify(results,null,2));
}catch(error){writeFileSync(resolve(out,'failure.json'),JSON.stringify({error:String(error),results},null,2));throw error;}finally{await browser?.close();server.kill();}
