import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
const root=resolve(import.meta.dirname,'../..');
const review=resolve(import.meta.dirname,'review');mkdirSync(review,{recursive:true});
const port=Number(process.env.LIFELENS_TIGER_PORT??8765);
const base=`http://127.0.0.1:${port}`;
const inspection=process.env.LIFELENS_TIGER_INSPECTION==='1';
const server=spawn('python3',['-m','http.server',String(port),'--bind','127.0.0.1','--directory',root],{stdio:'ignore'});
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
let browser;
const errors=[];
try{
 for(let i=0;i<30;i++){try{const r=await fetch(base);if(r.ok)break;}catch{}await pause(100);}
 browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH??chromium.executablePath(),headless:true,
  args:['--no-sandbox','--no-zygote','--single-process','--disable-crashpad-for-testing','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:960,height:720},deviceScaleFactor:1});
 page.on('pageerror',error=>errors.push(String(error)));
 page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
 const metrics=[];
 for(const baseline of [true,false]){
  await page.goto(`${base}/tests/procedural-environment/tiger-integration.html?inspection=${inspection?1:0}&baseline=${baseline?1:0}`);
  await page.waitForFunction(()=>Boolean(window.tigerReview),{},{timeout:20000});
  if(!baseline)await page.waitForFunction(()=>window.tigerReview.metrics().importedActor,{},{timeout:20000});
  for(const view of ['front','side','quarter','back','face']){
   await page.evaluate(name=>window.tigerReview.view(name),view);await pause(250);
   await page.screenshot({path:resolve(review,`tiger-layer-${baseline?'baseline':'candidate'}-${view}.png`)});
  }
  const result=await page.evaluate(()=>window.tigerReview.metrics());
  if(!result.coreUnchanged)throw Error('Renderer mutated authoritative fixture');metrics.push({...result,baseline});
 }
 await page.setViewportSize({width:412,height:915});await pause(1000);
 await page.screenshot({path:resolve(review,'tiger-layer-mobile-viewport.png')});
 metrics.push({...await page.evaluate(()=>window.tigerReview.metrics()),mobileViewportOnly:true});
 const result={status:errors.length?'FAIL_BROWSER_ERRORS':'PASS_PRODUCTION_LAYER_SUBSYSTEM_ONLY',
  source:inspection?'LOCAL_INSPECTION_DERIVATIVE':'STAGED_PINNED_SOURCE_DERIVATIVE',coreFixture:'UNMODIFIED_CORE_NATIVE_HARNESS',
  fullObserver:'NOT_RUN',android:'NOT_MEASURED',mergeAllowed:false,metrics,errors};
 writeFileSync(resolve(review,'tiger-browser-validation.json'),JSON.stringify(result,null,2)+'\n');
 console.log(JSON.stringify({status:result.status,errors,actors:metrics.map(x=>x.importedActor)}));
 if(errors.length)process.exitCode=1;
}catch(error){
 writeFileSync(resolve(review,'tiger-browser-validation.json'),JSON.stringify({
  status:'FAIL_BROWSER_STARTUP_OR_CAPTURE',error:String(error),errors,
  source:inspection?'LOCAL_INSPECTION_DERIVATIVE':'STAGED_PINNED_SOURCE_DERIVATIVE',
  fullObserver:'NOT_RUN',android:'NOT_MEASURED',mergeAllowed:false},null,2)+'\n');
 console.error(error);process.exitCode=1;
}finally{await browser?.close();server.kill();}
