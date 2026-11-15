const { chromium } = require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const fs = require('node:fs'), assert = require('node:assert/strict'), crypto = require('node:crypto');
(async () => {
  const base = process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:5173/';
  const output = process.env.LIFELENS_REVIEW_OUTPUT || '/tmp/lifelens-al2-browser';
  const directory = process.env.LIFELENS_PRODUCTION_FIXTURE_DIR || '/tmp/lifelens-production-fixture';
  const proof = JSON.parse(fs.readFileSync(directory + '/wasm-proof.json'));
  const residents = JSON.parse(fs.readFileSync(directory + '/residents.json')).residents;
  const index = residents.findIndex(r => r.id === proof.actor); assert(index >= 0);
  const binary = fs.readFileSync(directory + '/result.bin');
  const record = { key: 'active-world', schemaVersion: 1, worldSeed: proof.world.worldSeed,
    minute: proof.world.minute, savedAtWallMs: Date.now(), snapshotBase64: binary.toString('base64'), earthOrigin: null };
  fs.mkdirSync(output, { recursive: true });
  const proxy = /^https:/.test(base) && (process.env.HTTPS_PROXY || process.env.HTTP_PROXY);
  const browser = await chromium.launch({ executablePath: '/usr/bin/chromium', headless: true,
    ...(proxy ? { proxy: { server: proxy } } : {}), args: ['--no-sandbox','--disable-dev-shm-usage','--disable-gpu'] });
  const errors = [], widths = [];
  try {
    const context = await browser.newContext({ viewport: { width: 393, height: 852 }, isMobile: true, hasTouch: true });
    await context.addInitScript(() => {
      Object.defineProperty(navigator, 'gpu', { value: undefined });
      const get = HTMLCanvasElement.prototype.getContext;
      HTMLCanvasElement.prototype.getContext = function(kind, ...args) {
        return ['webgl','webgl2','experimental-webgl'].includes(kind) ? null : get.call(this,kind,...args);
      };
    });
    const page = await context.newPage(); page.on('pageerror', e => errors.push(e.message));
    // Isolated browser context, actual native checkpoint from an untouched
    // world. No real user's database or simulation world is overwritten.
    await page.goto(base + 'runtime/RUNTIME_COMMIT.txt?al2-review', { waitUntil: 'domcontentloaded' });
    await page.evaluate(record => new Promise((resolve,reject) => {
      const open = indexedDB.open('LifeLensContinuingWorld', 1);
      open.onupgradeneeded = () => open.result.createObjectStore('checkpoints', { keyPath: 'key' });
      open.onerror = () => reject(open.error);
      open.onsuccess = () => {
        const db = open.result, tx = db.transaction('checkpoints','readwrite');
        tx.objectStore('checkpoints').put(record);
        tx.oncomplete = () => { db.close(); resolve(); }; tx.onerror = () => reject(tx.error);
      };
    }), record);
    const show = async () => {
      await page.waitForFunction(() => document.querySelector('#status')?.textContent?.includes('시뮬레이션 코어 연결됨'),null,{timeout:120000});
      await page.getByRole('button',{name:'일시정지',exact:true}).click({force:true});
      await page.getByRole('navigation',{name:'주요 관찰 메뉴'}).getByRole('button',{name:'주민',exact:true}).click({force:true});
      const list = page.getByRole('dialog',{name:'주민',exact:true});
      assert.equal(await list.locator('.resident-card').count(), residents.length);
      await list.locator('.resident-card').nth(index).click({force:true});
      const detail=page.getByRole('dialog');
      await detail.getByText(/이어가는 제작 목표/).click({force:true});
      const panel=detail.locator('details').filter({hasText:'이어가는 제작 목표'}).first();
      const text=await panel.innerText();
      for(const value of ['목표 달성','실제 생활 판단에서 선택한 목표','실제 실패','재료 확보','실제 성공',`목표 #${proof.plan.id}`]) assert(text.includes(value),value);
      for(const step of proof.plan.steps)assert(text.includes(`명령 #${step.token}`));
      assert(!(text.includes('모델에서 선택')));
    };
    await page.goto(base+'index.html?al2-native-checkpoint',{waitUntil:'domcontentloaded'});await show();
    for(const width of [320,393,915,1440]) {
      await page.setViewportSize({width,height:width===915?412:852});
      const currentPanel=page.locator('details:visible').filter({hasText:'이어가는 제작 목표'}).first();
      if(!await currentPanel.evaluate(el=>el.open))await currentPanel.locator('summary').click({force:true});
      const actualText=await currentPanel.innerText();
      assert(actualText.includes(`목표 #${proof.plan.id}`));
      for(const step of proof.plan.steps)assert(actualText.includes(`명령 #${step.token}`));
      await currentPanel.scrollIntoViewIfNeeded();
      assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
      widths.push(width);await page.screenshot({path:`${output}/production-${width}.png`});
    }
    // Desktop uses its own side rail; perform the reload navigation through
    // the same mobile entry controls used when this review began.
    await page.setViewportSize({width:393,height:852});
    await page.evaluate(()=>window.dispatchEvent(new Event('pagehide')));
    await page.waitForFunction(async previous => {
      const db=await new Promise((resolve,reject)=>{const req=indexedDB.open('LifeLensContinuingWorld',1);req.onsuccess=()=>resolve(req.result);req.onerror=()=>reject(req.error);});
      try {const result=await new Promise((resolve,reject)=>{const req=db.transaction('checkpoints').objectStore('checkpoints').get('active-world');req.onsuccess=()=>resolve(req.result);req.onerror=()=>reject(req.error);});return result.savedAtWallMs>previous;}
      finally {db.close();}
    },record.savedAtWallMs,{timeout:30000});
    await page.reload({waitUntil:'domcontentloaded'});await show();
    assert.deepEqual(errors,[]);
    const response=await page.request.get(base+'runtime/RUNTIME_COMMIT.txt');assert.equal(response.status(),200);
    const source=(await response.text()).trim();
    if(process.env.LIFELENS_EXPECTED_RUNTIME_SHA)assert.equal(source,process.env.LIFELENS_EXPECTED_RUNTIME_SHA);
    fs.writeFileSync(output+'/app-proof.json',JSON.stringify({
      profile:'actual app restored from untouched native Core production checkpoint in isolated IndexedDB; software 2D fallback',
      source,fixtureSha256:crypto.createHash('sha256').update(binary).digest('hex'),actor:proof.actor,goal:proof.plan.id,
      actualFailureGatherSuccessVisible:true,actualNativeOriginVisible:true,actualTokensVisible:true,widths,
      noOverflow:true,appCheckpointWrite:true,reloadRetainsGoalAndSteps:true,errors},null,2));
    console.log('PASS actual Core natural production goal/failure/gather/result UI, 320/393/915/1440, app save and reload, errors0');
  } finally { await browser.close(); }
})().catch(e=>{console.error(e);process.exit(1);});
