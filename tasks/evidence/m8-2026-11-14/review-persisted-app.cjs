const { chromium } = require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const fs = require('node:fs');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
(async () => {
  const base = process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:5173/';
  const output = process.env.LIFELENS_REVIEW_OUTPUT || '/tmp/lifelens-m8-persisted-review';
  const fixture = process.env.LIFELENS_ARTIFACT_FIXTURE_DIR || '/tmp/lifelens-artifact-fixture';
  fs.mkdirSync(output, { recursive: true });
  const binary = fs.readFileSync(fixture + '/result.bin');
  const record = { key: 'active-world', schemaVersion: 1, worldSeed: '874213954',
    minute: 480 + Number(fs.readFileSync(fixture + '/minutes.txt', 'utf8')),
    savedAtWallMs: Date.now(), snapshotBase64: binary.toString('base64'), earthOrigin: null };
  const proxy = /^https:/.test(base) && (process.env.HTTPS_PROXY || process.env.HTTP_PROXY);
  const browser = await chromium.launch({ executablePath: '/usr/bin/chromium', headless: true,
    ...(proxy ? { proxy: { server: proxy } } : {}), args: ['--no-sandbox','--disable-dev-shm-usage','--disable-gpu'] });
  const errors = [];
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
    // A brand-new isolated browser context: no actual user's saved world is touched.
    await page.goto(base + 'runtime/RUNTIME_COMMIT.txt?m8-controlled-checkpoint', { waitUntil: 'domcontentloaded' });
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
      const list=page.getByRole('dialog',{name:'주민',exact:true});
      assert.equal(await list.locator('.resident-card').count(),4);
      await list.locator('.resident-card').first().click({force:true});
      const detail=page.getByRole('dialog');
      await detail.getByText(/발명과 제작 지식/).click({force:true});
      const text=await detail.innerText();
      for(const value of ['16928123745832484579','15171401966717868459','점토 용기 3단위','제작 성공','원료 3단위 소모','재현 가능']) assert(text.includes(value),value);
      assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
    };
    await page.goto(base+'index.html?m8-controlled-restore',{waitUntil:'domcontentloaded'});
    await show(); await page.screenshot({path:output+'/persisted-invention-mobile.png'});
    await page.evaluate(()=>window.dispatchEvent(new Event('pagehide')));
    await page.waitForFunction(async previous => {
      const db=await new Promise((resolve,reject)=>{const req=indexedDB.open('LifeLensContinuingWorld',1);req.onsuccess=()=>resolve(req.result);req.onerror=()=>reject(req.error);});
      try {const result=await new Promise((resolve,reject)=>{const req=db.transaction('checkpoints').objectStore('checkpoints').get('active-world');req.onsuccess=()=>resolve(req.result);req.onerror=()=>reject(req.error);});return result.savedAtWallMs>previous;}
      finally {db.close();}
    },record.savedAtWallMs,{timeout:30000});
    await page.reload({waitUntil:'domcontentloaded'});await show();
    await page.setViewportSize({width:915,height:412});
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
    await page.screenshot({path:output+'/persisted-invention-reloaded-landscape.png'});
    assert.deepEqual(errors,[]);
    const response=await page.request.get(base+'runtime/RUNTIME_COMMIT.txt');assert.equal(response.status(),200);
    const source=(await response.text()).trim();
    if(process.env.LIFELENS_EXPECTED_RUNTIME_SHA) assert.equal(source,process.env.LIFELENS_EXPECTED_RUNTIME_SHA);
    fs.writeFileSync(output+'/persisted-app-proof.json',JSON.stringify({
      profile:'actual app restored from controlled native production checkpoint in isolated IndexedDB; software 2D fallback',
      controlledFixture:true,source,fixtureSha256:crypto.createHash('sha256').update(binary).digest('hex'),
      uniqueArtifact:'16928123745832484579',design:'15171401966717868459',actualStructureAndPaidEventVisible:true,
      appCheckpointWrite:true,reloadRetainsArtifactAndPrivateKnowledge:true,mobileAndLandscapeNoOverflow:true,errors},null,2));
    console.log('PASS actual app native artifact restore -> Korean structure/paid event/private knowledge -> app save -> reload, errors0');
  } finally { await browser.close(); }
})().catch(e=>{console.error(e);process.exit(1);});
