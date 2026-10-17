import assert from 'node:assert/strict';
import {chromium} from 'playwright';
import {createHash} from 'node:crypto';
import {readFileSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';

const url='https://sjlim91.github.io/lifelens-ue/review/tiger-728/index.html?review=1';
const out=resolve(import.meta.dirname,'review');
const expected=createHash('sha256').update(readFileSync(resolve(import.meta.dirname,'../../public/vendor/wildlife/tiger-rigged.glb'))).digest('hex');
let browser;const errors=[],assets=[],pending=[];
try{
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:960,height:720},deviceScaleFactor:1});
 page.on('pageerror',error=>errors.push(String(error)));
 page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
 page.on('response',response=>{
  if(response.url().includes('/vendor/wildlife/tiger-rigged.glb'))pending.push(response.body().then(bytes=>assets.push({url:response.url(),status:response.status(),sha256:createHash('sha256').update(bytes).digest('hex')})));
 });
 await page.addInitScript(()=>localStorage.setItem('lifelens.web.renderMode','three-world'));
 let deployed=false;
 for(let attempt=0;attempt<30;attempt++){
  const response=await page.goto(url+'&v='+encodeURIComponent(process.env.GITHUB_SHA??'probe')+'&attempt='+attempt,{waitUntil:'domcontentloaded',timeout:60000});
  if(response.status()===200){
   await page.waitForFunction(()=>Boolean(window.__tigerObserver),{},{timeout:30000});
   deployed=await page.evaluate(expected=>!expected||window.__tigerObserver.reviewCommit===expected,process.env.GITHUB_SHA??null);
   if(deployed)break;
  }
  await new Promise(resolve=>setTimeout(resolve,3000));
 }
 assert(deployed,'Pages must serve this exact review build');errors.length=0;
 await page.waitForFunction(()=>window.__tigerObserver?.store.getSnapshot().runtime.status==='ready',{},{timeout:120000});
 assert.equal(await page.getByRole('button',{name:'30초 성능 측정',exact:true}).count(),1);
 await page.evaluate(()=>{const q=window.__tigerObserver;q.actions.setSimulationSpeed(0);q.actions.createWorld('77');q.actions.setSimulationSpeed(0);q.actions.moveObserver(-2,0);});
 await page.waitForFunction(()=>{const s=window.__tigerObserver.store.getSnapshot();return String(s.world.worldSeed)==='77'&&s.terrain?.wildlife?.animals.some(a=>a.species==='Tiger');},{},{timeout:30000});
 await page.getByRole('button',{name:'현재 호랑이 보기',exact:true}).click();
 await page.waitForFunction(()=>Boolean(window.__tigerObserver.sample().actor),{},{timeout:60000});
 const sample=await page.evaluate(async()=>{await window.__tigerObserver.still();return window.__tigerObserver.sample();});
 assert.equal(sample.actor.id,sample.core.id);assert.equal(sample.runtime.status,'ready');
 assert.equal(sample.canvas.contextLost,false);await Promise.all(pending);
 assert(assets.some(asset=>asset.status===200&&asset.sha256===expected),'Published GLB must match the reviewed original-source derivative');
 assert.equal(errors.length,0,JSON.stringify(errors));
 await page.screenshot({path:resolve(out,'published-observer.jpg'),type:'jpeg',quality:85});
 writeFileSync(resolve(out,'published-validation.json'),JSON.stringify({status:'PASS_PUBLISHED_REACT_WASM_AND_PINNED_NATIVE_MODEL',url,expectedSha256:expected,assets,sample,errors,physicalAndroid:'NOT_MEASURED',mergeAllowed:false},null,2)+'\n');
 console.log('PASS published actual Observer, measurement controls and exact native GLB hash',url);
}catch(error){writeFileSync(resolve(out,'published-validation.json'),JSON.stringify({status:'FAIL_PUBLISHED_OBSERVER',url,error:String(error),errors,assets,mergeAllowed:false},null,2)+'\n');throw error;}
finally{await browser?.close();}
