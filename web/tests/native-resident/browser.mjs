import assert from 'node:assert/strict';
import {chromium} from 'playwright';
import {spawn} from 'node:child_process';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
const out=resolve(import.meta.dirname,'review');mkdirSync(out,{recursive:true});
const server=spawn('node',['node_modules/vite/bin/vite.js','--host','127.0.0.1','--port','8770'],{stdio:'ignore'});
let browser;const errors=[],reports={};
try{
 for(let i=0;i<80;i++){try{if((await fetch('http://127.0.0.1:8770')).ok)break;}catch{}await new Promise(r=>setTimeout(r,200));}
 browser=await chromium.launch({headless:true,args:['--no-sandbox','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const page=await browser.newPage({viewport:{width:1000,height:760},deviceScaleFactor:1});
 page.on('pageerror',e=>errors.push(String(e)));page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
 page.on('response',r=>{if(r.status()>=400)errors.push('HTTP '+r.status()+' '+r.url());});
 await page.goto('http://127.0.0.1:8770/tests/native-resident/fixture.html',{waitUntil:'domcontentloaded'});
 await page.waitForFunction(()=>window.nativeReady,{},{timeout:180000});
 reports.adults=await page.evaluate(()=>window.nativeQA.sample());
 reports.phenotypeRefresh=await page.evaluate(()=>window.nativeQA.phenotypeRefreshInvariant());
 assert(reports.phenotypeRefresh.coreFactsUnchanged);assert(reports.phenotypeRefresh.maximumRestDifference<1e-5,'Late genetics must not bake the animated actor world transform into the source body');
 for(const a of reports.adults){assert(a.native&&a.sameSkeleton&&a.uv&&a.texture);assert.equal(a.version,'MakeHuman-native-v1');assert.deepEqual(a.meshCount,[3,3,3]);assert(a.tiers[0].triangles>a.tiers[1].triangles&&a.tiers[1].triangles>a.tiers[2].triangles);assert(a.bounds.max[1]-a.bounds.min[1]>.8);assert(a.bounds.min[1]>=-.01,'Native soles must not penetrate the actual ground');}
 for(const view of ['front','side']){const data=await page.evaluate(view=>window.nativeQA.view(view),view);writeFileSync(resolve(out,'adults-'+view+'.png'),Buffer.from(data.split(',')[1],'base64'));}
 reports.lods=[];for(const distance of [4,12,25])reports.lods.push(await page.evaluate(d=>window.nativeQA.lod(d),distance));
 for(const [i,rows] of reports.lods.entries())for(const row of rows)assert.equal(row.tiers.findIndex(t=>t.visible),i);
 await page.evaluate(()=>window.nativeQA.lod(6.5));
 reports.life=[];
 for(const [sex,age,stage] of [['Female',.3,'Baby'],['Male',5,'Child'],['Female',16,'Teen'],['Male',80,'Elderly']]){
  const sample=await page.evaluate(([sex,age,stage])=>window.nativeQA.stage(sex,age,stage),[sex,age,stage]);assert(sample.every(a=>a.native&&a.sameSkeleton));reports.life.push({sex,age,stage,sample});
  const data=await page.evaluate(()=>window.nativeQA.render());writeFileSync(resolve(out,stage+'.png'),Buffer.from(data.split(',')[1],'base64'));
 }
 reports.pregnancy=await page.evaluate(()=>window.nativeQA.stage('Female',30,'Adult',{role:'GestationalParent',stage:'ThirdTrimester'}));
 reports.sleep=await page.evaluate(()=>window.nativeQA.motion('Sleep'));assert(reports.sleep.every(a=>a.active==='sleep'));
 const data=await page.evaluate(()=>window.nativeQA.render());writeFileSync(resolve(out,'sleep.png'),Buffer.from(data.split(',')[1],'base64'));
 assert.equal(errors.length,0,JSON.stringify(errors));
 writeFileSync(resolve(out,'validation.json'),JSON.stringify({status:'PASS_NATIVE_RESIDENT_BROWSER_CONTRACT',reports,errors,physicalAndroid:'NOT_MEASURED'},null,2)+'\n');
 console.log('PASS real WebGL native male/female source, common skeleton, UV/PBR, LOD, growth, pregnancy and sleep');
}catch(error){writeFileSync(resolve(out,'validation.json'),JSON.stringify({status:'FAIL_NATIVE_RESIDENT_BROWSER',error:String(error),reports,errors},null,2)+'\n');throw error;}
finally{await browser?.close();server.kill();}
