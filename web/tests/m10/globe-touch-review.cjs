// Optional actual-browser review; run against a built preview. No simulated Core events.
const {chromium}=require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const assert=require('node:assert/strict'),fs=require('node:fs');
const base=process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:4173/';
const output=process.env.LIFELENS_M10_REVIEW_DIR || '/tmp/lifelens-m10-full';fs.mkdirSync(output,{recursive:true});
(async()=>{const b=await chromium.launch({executablePath:process.env.LIFELENS_CHROMIUM_PATH || '/usr/bin/chromium',headless:true,args:['--no-sandbox','--disable-dev-shm-usage','--use-angle=swiftshader','--enable-unsafe-swiftshader']});try{
const c=await b.newContext({viewport:{width:393,height:852},hasTouch:true,isMobile:true});await c.addInitScript(()=>{Object.defineProperty(navigator,'gpu',{value:undefined});localStorage.setItem('lifelens.web.renderMode','legacy-canvas');});const p=await c.newPage(),errors=[];p.on('pageerror',e=>errors.push(e.message));
await p.goto(base+'index.html');await p.waitForFunction(()=>document.querySelector('#status')?.textContent?.includes('시뮬레이션 코어 연결됨'),null,{timeout:60000});await p.getByRole('button',{name:'일시정지',exact:true}).click();await p.getByRole('button',{name:'지구 관찰',exact:true}).click();
const g=p.getByRole('dialog',{name:'지구 자연지리 관찰 미리보기'}),canvas=g.locator('canvas');await canvas.waitFor();await p.waitForTimeout(300);const box=await canvas.boundingBox(),x=box.x+box.width*.5,y=box.y+box.height*.5;
const d=await c.newCDPSession(p);const touch=(type,points)=>d.send('Input.dispatchTouchEvent',{type,touchPoints:points.map(([id,x,y])=>({id,x,y,radiusX:1,radiusY:1,force:1}))});const snap=async()=>{await p.waitForTimeout(90);return canvas.screenshot()};
const before=await snap();await touch('touchStart',[[1,x,y]]);await touch('touchMove',[[1,x+65,y+30]]);await touch('touchEnd',[]);const orbit=await snap();assert(!orbit.equals(before),'one finger orbit must change actual globe');
await touch('touchStart',[[1,x-45,y],[2,x+45,y]]);await touch('touchMove',[[1,x-75,y],[2,x+75,y]]);const pinch=await snap();assert(!pinch.equals(orbit),'two finger pinch must change actual globe');
await touch('touchEnd',[[2,x+75,y]]);await touch('touchMove',[[1,x-30,y+25]]);const resumed=await snap();assert(!resumed.equals(pinch),'remaining globe finger must rebase and rotate without jumping');await touch('touchCancel',[]);
await touch('touchStart',[[3,x,y]]);await touch('touchMove',[[3,x-40,y-15]]);await touch('touchEnd',[]);assert(!(await snap()).equals(resumed),'new gesture must work after cancellation');
assert.equal(await g.getByText('좌표 미선택',{exact:true}).count(),2,'orbit/pinch must not accidentally select region');await g.getByRole('button',{name:'지구본 닫기'}).click();assert.deepEqual(errors,[]);
const result={source:process.env.LIFELENS_REVIEW_SOURCE || null,applicationUrl:base,viewport:[393,852],coreConnected:true,actualCdpTouch:true,orbit:true,pinch:true,twoToOneRebase:true,cancelRecovery:true,accidentalRegionSelection:false,pageErrors:errors};fs.writeFileSync(output+'/globe-touch-acceptance.json',JSON.stringify(result,null,2));console.log(JSON.stringify(result));
}finally{await b.close()}})().catch(e=>{console.error(e);process.exitCode=1});
