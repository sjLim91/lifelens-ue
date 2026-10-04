import assert from 'node:assert/strict';
import { chromium } from 'playwright';
import { createServer } from 'node:http';
import { readFileSync, mkdirSync, writeFileSync } from 'node:fs';
import { extname, resolve } from 'node:path';

const root = resolve(import.meta.dirname, '../..'), out = resolve(import.meta.dirname,'review');
mkdirSync(out,{recursive:true});
const server=createServer((req,res)=>{
 try {
  const path=resolve(root,'.'+new URL(req.url,'http://fixture').pathname);
  if(!path.startsWith(root+'/')) {res.writeHead(403);res.end();return;}
  res.setHeader('Content-Type',({'.html':'text/html','.mjs':'text/javascript','.js':'text/javascript'})[extname(path)]??'application/octet-stream');
  res.end(readFileSync(path));
 }catch {res.writeHead(404);res.end();}
});
await new Promise(r=>server.listen(0,'127.0.0.1',r));
const browser=await chromium.launch({headless:true,args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
const results=[], images=[];
const types=['PositiveInteraction','Help','Comfort','Conflict','Betrayal','Rejection','Apology','Intimacy','Commitment'];
try {
 for(const [device,width,height] of [['mobile',390,844],['desktop',1280,900]]) {
  const page=await browser.newPage({viewport:{width,height}});
  const errors=[]; page.on('pageerror',e=>errors.push(e.message));
  page.on('console',m=>{if(m.type()==='error')errors.push(m.text());});
  for(const type of types) {
   await page.goto(`http://127.0.0.1:${server.address().port}/tests/social-events/fixture.html?type=${type}`);
   await page.waitForFunction(()=>window.fixtureReady===true);
   const stats=await page.evaluate(()=>window.renderProgress(.55));
   assert.equal(stats.active,1); assert.equal(stats.cueChildren,1);
   const before=await page.evaluate(()=>window.renderProgress(.9));
   assert.equal(before.geometries,stats.geometries);
   await page.evaluate(()=>window.renderProgress(.55));
   const filename=`${device}-${type}.png`;
   await page.screenshot({path:resolve(out,filename)});
   images.push({device,type,filename});
   const ended=await page.evaluate(()=>window.renderProgress(1.1));
   assert.equal(ended.calls,stats.calls-1); assert.equal(ended.active,0);
   results.push({device,type,...stats,drawCallDelta:stats.calls-ended.calls});
  }
  // Camera minimum-zoom clutter guard and unsuccessful visual rendering.
  for(const query of ['type=Conflict&zoom=min','type=Apology&failed=true']) {
   await page.goto(`http://127.0.0.1:${server.address().port}/tests/social-events/fixture.html?${query}`);
   await page.waitForFunction(()=>window.fixtureReady===true);
   const stats=await page.evaluate(()=>window.renderProgress(.55));assert.equal(stats.active,1);
  }
  assert.deepEqual(errors,[]); await page.close();
 }
 // Contact sheets retain full mobile/desktop screenshots in the artifact.
 const page=await browser.newPage({viewport:{width:900,height:690}});
 for(const device of ['mobile','desktop']) {
  const tiles=images.filter(i=>i.device===device).map(i=>`<figure><img src="data:image/png;base64,${readFileSync(resolve(out,i.filename)).toString('base64')}"><figcaption>${i.type}</figcaption></figure>`).join('');
  await page.setContent(`<style>body{margin:0;background:#202922;color:#e6e9df;font:12px sans-serif;display:grid;grid-template-columns:repeat(3,300px)}figure{margin:0;text-align:center}img{display:block;width:300px;height:205px;object-fit:contain;background:#536052}figcaption{padding:6px}</style>${tiles}`);
  const screenshot=await page.screenshot({path:resolve(out,`${device}-contact.png`)});
  // Read-only image evidence can be recovered through the GitHub log connector.
  console.log(`SOCIAL_SCREENSHOT_${device.toUpperCase()} ${screenshot.toString('base64')}`);
 }
 await page.close();
 writeFileSync(resolve(out,'browser-results.json'),JSON.stringify(results,null,2));
 console.log('Social browser review PASS',JSON.stringify(results));
} finally {await browser.close();await new Promise(r=>server.close(r));}
