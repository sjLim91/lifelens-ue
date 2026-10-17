import assert from 'node:assert/strict';
import {chromium} from 'playwright';
import {createHash} from 'node:crypto';
import {mkdirSync,readFileSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';

const url='https://sjlim91.github.io/lifelens-ue/review/tiger-728/index.html?review=1';
const out=resolve(import.meta.dirname,'review');
mkdirSync(out,{recursive:true});
const expected=createHash('sha256').update(readFileSync(resolve(import.meta.dirname,'../../public/vendor/wildlife/tiger-rigged.glb'))).digest('hex');
const characters=JSON.parse(readFileSync(resolve(import.meta.dirname,'../../public/vendor/characters/receipt.json'),'utf8'));
let browser;const errors=[],assets=[],pending=[],httpErrors=[];
const expectedBuild=process.env.LIFELENS_REVIEW_EXPECTED_BUILD??process.env.GITHUB_SHA??null;
const publishedCharacters=[];
async function bounded(promise,ms,label){
 let timer;try{return await Promise.race([promise,new Promise((_,reject)=>{
  timer=setTimeout(()=>reject(Error(`${label} timed out after ${ms}ms`)),ms);
 })]);}finally{clearTimeout(timer);}
}
try{
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:960,height:720},deviceScaleFactor:1});
 // Pages can serve the new JS before its model files become available. Verify
 // the ordinary public asset URLs before boot, rather than accepting a fallback.
 for(const character of characters){
  const assetUrl=new URL('vendor/characters/'+character.file,url).href;
  let verified=false,lastStatus=null;
  for(let attempt=0;attempt<30;attempt++){
   const response=await page.request.get(assetUrl,{timeout:15000,headers:{'Cache-Control':'no-cache'}});
   lastStatus=response.status();
   if(response.ok()){
    const bytes=await response.body();
    const sha256=createHash('sha256').update(bytes).digest('hex');
    verified=bytes.length===character.bytes&&sha256===character.sha256;
    if(verified)publishedCharacters.push({file:character.file,url:assetUrl,status:lastStatus,bytes:bytes.length,sha256});
   }
   await response.dispose();
   if(verified)break;
   await new Promise(resolve=>setTimeout(resolve,3000));
  }
  assert(verified,`Published character asset ${character.file} must match its pinned receipt (last HTTP ${lastStatus})`);
 }
 page.on('pageerror',error=>errors.push(String(error)));
 page.on('console',message=>{if(message.type()==='error')errors.push({text:message.text(),location:message.location()});});
 page.on('response',response=>{
  if(response.status()>=400)httpErrors.push({url:response.url(),status:response.status()});
  if(response.url().includes('/vendor/wildlife/tiger-rigged.glb'))pending.push(response.body().then(bytes=>assets.push({url:response.url(),status:response.status(),sha256:createHash('sha256').update(bytes).digest('hex')})));
 });
 await page.addInitScript(()=>localStorage.setItem('lifelens.web.renderMode','three-world'));
 let deployed=false;
 for(let attempt=0;attempt<30;attempt++){
  const response=await page.goto(url+'&v='+encodeURIComponent(expectedBuild??'probe')+'&attempt='+attempt,{waitUntil:'domcontentloaded',timeout:60000});
  if(response.status()===200){
   await page.waitForFunction(()=>Boolean(window.__tigerObserver),{},{timeout:30000});
   deployed=await page.evaluate(expected=>!expected||window.__tigerObserver.reviewCommit===expected,expectedBuild);
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
 assert.equal(httpErrors.length,0,JSON.stringify(httpErrors));
 // Use the same actual Observer canvas export that passed local production
 // capture. Full-page screenshots can hang on the software WebGL compositor.
 const capture=await bounded(page.evaluate(async()=>{
  const q=window.__tigerObserver;await q.still();
  const canvas=q.renderer.renderer.domElement;
  if(canvas.id!=='threeWorldCanvas'||!document.querySelector('#app .brand'))throw Error('Capture is not the actual Observer canvas');
  if(q.renderer.renderer.getContext().isContextLost())throw Error('Actual Observer WebGL context was lost');
  if(canvas.width*canvas.height>4_000_000)throw Error('Actual Observer canvas exceeds the capture pixel budget');
  return {data:canvas.toDataURL('image/jpeg',.88),width:canvas.width,height:canvas.height,canvasId:canvas.id};
 }),90000,'Published Observer canvas export');
 assert(capture.data.startsWith('data:image/jpeg;base64,'),'Published Observer canvas must produce an image');
 writeFileSync(resolve(out,'published-observer.jpg'),Buffer.from(capture.data.split(',')[1],'base64'));
 await Promise.all(pending);
 assert.equal(errors.length,0,JSON.stringify(errors));
 assert.equal(httpErrors.length,0,JSON.stringify(httpErrors));
 writeFileSync(resolve(out,'published-validation.json'),JSON.stringify({status:'PASS_PUBLISHED_REACT_WASM_AND_PINNED_NATIVE_MODEL',url,expectedBuild,expectedSha256:expected,assets,publishedCharacters,sample,capture:{width:capture.width,height:capture.height,canvasId:capture.canvasId},errors,httpErrors,physicalAndroid:'NOT_MEASURED',mergeAllowed:false},null,2)+'\n');
 console.log('PASS published actual Observer, measurement controls and exact native GLB hash',url);
}catch(error){writeFileSync(resolve(out,'published-validation.json'),JSON.stringify({status:'FAIL_PUBLISHED_OBSERVER',url,expectedBuild,error:String(error),errors,httpErrors,assets,publishedCharacters,mergeAllowed:false},null,2)+'\n');throw error;}
finally{await browser?.close();}

