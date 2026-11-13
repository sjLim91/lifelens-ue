// Optional UI review of actual native-generated evidence; no model inference.
const { chromium } = require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const assert = require('node:assert/strict');
(async () => {
 const browser = await chromium.launch({ executablePath: process.env.LIFELENS_CHROMIUM_PATH || '/usr/bin/chromium', headless: true, args: ['--no-sandbox','--disable-gpu'] });
 try {
  for (const viewport of [{width:320,height:720},{width:393,height:852},{width:915,height:412}]) {
   const page = await browser.newPage({viewport}), errors=[];page.on('pageerror',e=>errors.push(e.message));
   for (const kind of (process.env.LIFELENS_REVIEW_CASES || 'completed,failure').split(',')) {
    await page.goto((process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:5173/')+'tests/causal-journal/fixture.html?case='+kind);
    await page.getByText('생각과 실제 행동',{exact:true}).click();
    await page.getByText('승인됨',{exact:true}).waitFor();
    assert(await page.getByText('직접 경험으로 믿음 갱신',{exact:true}).count()>=2);
    assert(await page.getByText('행동 실행 성공',{exact:true}).count()>=2);
    if(kind==='failure') assert(await page.getByText('행동 실행 실패',{exact:true}).count()>=1);
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
    await page.screenshot({path:`/tmp/lifelens-causal-${kind}-${viewport.width}.png`,fullPage:true});
   }
   assert.deepEqual(errors,[]);await page.close();
  }
  console.log('PASS actual Core records render approval -> multiple issued actions -> success/failure -> learning at 320/393/915px');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
