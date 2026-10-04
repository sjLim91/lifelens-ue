import assert from 'node:assert/strict';
import { chromium } from 'playwright';
import { createServer } from 'vite';
import { mkdirSync, writeFileSync } from 'node:fs';
import { resolve } from 'node:path';
const out=resolve(import.meta.dirname,'review');mkdirSync(out,{recursive:true});
const server=await createServer({root:resolve(import.meta.dirname,'../..'),server:{host:'127.0.0.1',port:0}});await server.listen();
const origin=server.resolvedUrls.local[0];const browser=await chromium.launch({args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});const results=[];
try{for(const [device,width,height] of [['mobile',390,844],['desktop',1280,900]]){
 const page=await browser.newPage({viewport:{width,height}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 for(const mode of ['single','grown','two','active','inactive','migration','frontier','many','invalid','popup','min','max']){
  await page.goto(`${origin}tests/settlement-trade/fixture.html?mode=${mode}`);await page.waitForFunction(()=>window.fixtureReady);
  const result=await page.evaluate(()=>window.review.refresh1000());assert.deepEqual(result.before,result.after);assert(result.identity);
  if(mode==='popup'){const p=await page.evaluate(()=>window.review.anchor());await page.mouse.click(p.x,p.y);await page.waitForSelector('dialog[open]');assert.match(await page.locator('dialog').innerText(),/인구/);await page.keyboard.press('Escape');await page.waitForSelector('dialog:not([open])',{state:'attached'});await page.evaluate(()=>window.review.open());await page.waitForSelector('dialog[open]');}
  if(mode==='frontier'){await page.evaluate(()=>document.getElementById('ui').style.visibility='hidden');await page.screenshot({path:resolve(out,`${device}-frontier-world.png`)});await page.evaluate(()=>document.getElementById('ui').style.visibility='visible');}
  if(['frontier','migration'].includes(mode))await page.getByRole('heading',{name:'이동 / 이주 압력'}).scrollIntoViewIfNeeded();
  await page.screenshot({path:resolve(out,`${device}-${mode}.png`)});results.push({device,mode,...result.after});
  if(mode==='invalid')assert.equal(result.after.overlay.anchors,0);
  if(mode==='frontier')assert(result.after.overlay.guideSegments>0);
  const reset=await page.evaluate(()=>window.review.reset());assert.equal(reset.anchors,0);assert.equal(reset.routeSegments,0);
 }
 assert.deepEqual(errors,[]);await page.close();
}writeFileSync(resolve(out,'browser-results.json'),JSON.stringify(results,null,2));console.log('24 browser scenes / 26 screenshots PASS',JSON.stringify(results));}
finally{await browser.close();await server.close();}
