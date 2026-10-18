import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {source} from './compile.mjs';
await source('render/wildlife-layer.ts');await source('render/terrain-geometry.ts');
const root=resolve(import.meta.dirname,'../..'),out=resolve(import.meta.dirname,'review/native-wildlife');mkdirSync(out,{recursive:true});
const port=Number(process.env.LIFELENS_WILDLIFE_REVIEW_PORT??8773),base=`http://127.0.0.1:${port}`;
const server=spawn('python3',['-m','http.server',String(port),'--bind','127.0.0.1','--directory',root],{stdio:'ignore'});
const pause=ms=>new Promise(r=>setTimeout(r,ms)),errors=[],records=[];let browser;
try{
 for(let i=0;i<50;i++){try{if((await fetch(base)).ok)break;}catch{}await pause(100);}
 browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH??chromium.executablePath(),headless:true,args:['--no-sandbox','--no-zygote','--disable-crashpad-for-testing','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:960,height:720},deviceScaleFactor:1});
 page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
 page.on('response',r=>{if(r.status()>=400)errors.push(`HTTP ${r.status()} ${r.url()}`);});
 for(const species of ['Hare','Deer','Wolf','FreshwaterFish']){
  const pair=[];
  for(const baseline of [true,false]){
   await page.goto(`${base}/tests/procedural-environment/native-wildlife.html?species=${species}&baseline=${baseline?1:0}`);
   await page.waitForFunction(()=>!!window.nativeWildlifeReview,{},{timeout:30000});
   if(!baseline)await page.waitForFunction(()=>window.nativeWildlifeReview.metrics().importedActor,{},{timeout:30000});
   for(const view of ['front','side','quarter','back']){
    await page.evaluate(name=>window.nativeWildlifeReview.view(name),view);await pause(150);
    await page.screenshot({path:resolve(out,`${species}-${baseline?'baseline':'native'}-${view}.png`)});
   }
   const record=await page.evaluate(()=>window.nativeWildlifeReview.metrics());if(!record.coreUnchanged)throw Error('Core observation mutated');
   records.push(record);pair.push(record);
   if(!baseline){
    await page.evaluate(()=>window.nativeWildlifeReview.view('far'));
    await page.waitForFunction(()=>{const m=window.nativeWildlifeReview.metrics();return m.farIds.includes(m.id);},{},{timeout:30000});
    records.push(await page.evaluate(()=>window.nativeWildlifeReview.metrics()));
    await page.screenshot({path:resolve(out,species+'-far.png')});
   }
  }
  if(pair[0].id!==pair[1].id||pair[0].gridX!==pair[1].gridX||pair[0].gridY!==pair[1].gridY)throw Error('Comparison used different Core subjects');
 }
 await page.setViewportSize({width:412,height:915});await page.evaluate(()=>window.nativeWildlifeReview.view('quarter'));await pause(300);
 await page.screenshot({path:resolve(out,'mobile-viewport.png')});
 if(errors.length)throw Error('Browser or HTTP errors: '+errors.join('; '));
 writeFileSync(resolve(out,'browser.json'),JSON.stringify({status:'PASS_PRODUCTION_WILDLIFE_LAYER',scope:'SUBSYSTEM_NOT_FULL_REACT_OBSERVER',physicalAndroid:'NOT_MEASURED',records,errors},null,2)+'\n');
 console.log('PASS actual four-species WildlifeLayer WebGL comparison, source-derived far instancing, immutable Core fixture and mobile viewport');
}catch(error){writeFileSync(resolve(out,'browser.json'),JSON.stringify({status:'FAIL',error:String(error),records,errors},null,2)+'\n');console.error(error);process.exitCode=1;}
finally{await browser?.close();server.kill();}
