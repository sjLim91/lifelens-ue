import assert from 'node:assert/strict';
import { chromium } from 'playwright';
import {readFileSync,existsSync,mkdirSync,writeFileSync} from 'node:fs';
import {resolve,extname} from 'node:path';
const root=resolve(import.meta.dirname,'../../..'),out=resolve(import.meta.dirname,'review');mkdirSync(out,{recursive:true});
const types={'.mjs':'text/javascript','.js':'text/javascript','.html':'text/html','.glb':'model/gltf-binary'};
let browser;
try{
 browser=await chromium.launch({headless:true,executablePath:process.env.CHROMIUM_PATH || (existsSync('/usr/bin/chromium') ? '/usr/bin/chromium' : undefined),args:['--no-sandbox','--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 const results=[];
 for(const [label,width,height] of [['desktop',960,720],['mobile',390,844]]){
 const page=await browser.newPage({viewport:{width,height}}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.route('http://fixture.local/**',async route=>{const pathname=new URL(route.request().url()).pathname;const file=resolve(root,'.'+decodeURIComponent(pathname));if(!file.startsWith(root+'/')||!existsSync(file))return route.fulfill({status:404,body:''});await route.fulfill({contentType:types[extname(file)]??'application/octet-stream',body:readFileSync(file)});});
 await page.goto('http://fixture.local/web/tests/procedural-motion/fixture.html');await page.waitForFunction(()=>window.ready);
 for(const name of ['idle','walk','slope','drink','no-water','sleep','injured']){const facts=await page.evaluate(n=>window.setScenario(n),name);assert.equal(facts.motion,({walk:'walk',slope:'walk',drink:'consume',sleep:'sleep'})[name]??'idle');assert(facts.drawCalls>0);assert(facts.inFrame,`${label} ${name}: body clipped`);await page.screenshot({path:resolve(out,`${label}-${name}.png`)});results.push({label,name,...facts});}
 if(errors.length)throw Error(errors.join('\n'));await page.close();}
 writeFileSync(resolve(out,'browser-results.json'),JSON.stringify(results,null,2));console.log('PASS browser WebGL screenshots:',results.length,'page errors: 0');
}finally{await browser?.close();}
