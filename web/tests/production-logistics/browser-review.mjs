import assert from 'node:assert/strict';
import {chromium} from 'playwright';
import {createServer} from 'node:http';
import {readFileSync,mkdirSync,writeFileSync} from 'node:fs';
import {resolve,extname} from 'node:path';
const root=resolve(import.meta.dirname,'../..'),out=resolve(import.meta.dirname,'review');mkdirSync(out,{recursive:true});
const server=createServer((req,res)=>{try{const p=resolve(root,'.'+new URL(req.url,'http://fixture').pathname);if(!p.startsWith(root+'/'))throw Error();res.setHeader('Content-Type',({'.html':'text/html','.mjs':'text/javascript','.js':'text/javascript'})[extname(p)]??'application/octet-stream');res.end(readFileSync(p));}catch{res.writeHead(404);res.end();}});
await new Promise(r=>server.listen(0,'127.0.0.1',r));const browser=await chromium.launch({args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});const results=[];
try{for(const [device,width,height] of [['mobile',390,844],['desktop',1280,900]]){const page=await browser.newPage({viewport:{width,height}});const errors=[];page.on('pageerror',e=>errors.push(e.message));
for(const mode of ['gather','gather-after','storage','storage-after','construction','construction-after','craft','craft-after','furnace','plot','fail-closed']){await page.goto(`http://127.0.0.1:${server.address().port}/tests/production-logistics/fixture.html?mode=${mode}`);await page.waitForFunction(()=>window.fixtureReady);const {before,after,sameIdentity}=await page.evaluate(()=>window.refresh100());assert.deepEqual(after,before);assert(sameIdentity);await page.screenshot({path:resolve(out,`${device}-${mode}.png`)});results.push({device,mode,...after});}
assert.deepEqual(errors,[]);await page.close();}writeFileSync(resolve(out,'browser-results.json'),JSON.stringify(results,null,2));console.log('Production logistics browser review PASS',JSON.stringify(results));}finally{await browser.close();await new Promise(r=>server.close(r));}


