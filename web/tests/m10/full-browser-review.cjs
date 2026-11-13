const { chromium } = require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const assert = require('node:assert/strict'), fs = require('node:fs');
const base = process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:5173/';
const output = process.env.LIFELENS_M10_REVIEW_DIR || '/tmp/lifelens-m10-full'; fs.mkdirSync(output, { recursive: true });
(async () => {
 const proxy = /^https:/.test(base) && (process.env.HTTPS_PROXY || process.env.HTTP_PROXY);
 const browser = await chromium.launch({ ...(proxy ? { proxy: { server: proxy } } : {}), executablePath: process.env.LIFELENS_CHROMIUM_PATH || '/usr/bin/chromium',
  headless: true, args: ['--no-sandbox','--disable-dev-shm-usage','--use-angle=swiftshader','--enable-unsafe-swiftshader'] });
 const results = [], errors = [];
 try {
  if (!process.env.LIFELENS_M10_VISUAL_ONLY) {
  const context = await browser.newContext({ viewport: { width: 393, height: 852 }, hasTouch: true, isMobile: true });
  await context.addInitScript(() => {
   Object.defineProperty(navigator, 'gpu', { value: undefined });
   const request = window.requestAnimationFrame.bind(window);
   window.__m10Frames = 0;
   window.requestAnimationFrame = callback => request(time => { window.__m10Frames++; callback(time); });
  });
  const page = await context.newPage(); page.on('pageerror', e => errors.push(e.message));
  await page.goto(base+'index.html?m10-full-acceptance', { waitUntil: 'domcontentloaded' });
  await page.waitForFunction(() => document.querySelector('#status')?.textContent?.includes('시뮬레이션 코어 연결됨'), null, { timeout: 90000 });
  await page.getByRole('button', { name: '일시정지', exact: true }).click();
  const nav = page.getByRole('navigation', { name: '주요 관찰 메뉴' });
  await nav.getByRole('button', { name: '주민', exact: true }).click();
  await page.getByRole('dialog').locator('.resident-card').first().click();
  let panel = page.getByRole('dialog');
  await panel.locator('.resident-mind-panel').first().locator('summary').click();
  assert.equal(await panel.locator('.resident-mind-panel').first().locator('.focused-life-chips > span').count(), 14);
  await panel.getByText('목표와 계획', { exact: true }).click();
  await panel.getByRole('region', { name: '현재 실행 계획' }).waitFor();
  await panel.locator('.resident-mind-panel').nth(1).getByText(/기준$/).waitFor();
  assert((await panel.innerText()).includes('현재 실행 계획'));
  assert(!(await panel.innerText()).includes('목표 후보를 읽을 수 없습니다'), 'actual Core goal composer must be attached');
  await page.screenshot({ path: output+'/resident-personality-plan.png', fullPage: true });
  await page.keyboard.press('Escape');
  for (const size of [{ width: 320, height: 568 }, { width: 393, height: 852 }, { width: 915, height: 412 }]) {
   await page.setViewportSize(size);
   await page.getByRole('button', { name: '지구 관찰', exact: true }).click();
   const globe = page.getByRole('dialog', { name: '지구 자연지리 관찰 미리보기' });
   await globe.locator('canvas').waitFor(); await page.waitForTimeout(400);
   assert(await globe.evaluate(el => el.scrollWidth <= el.clientWidth));
   assert((await globe.locator('canvas').boundingBox()).height >= 100);
   const still = await globe.locator('canvas').screenshot();
   const frames = await page.evaluate(() => window.__m10Frames);
   await page.waitForTimeout(500);
   assert((await page.evaluate(() => window.__m10Frames)) - frames <= 1, 'globe and concealed world must not keep rendering');
   assert((await globe.locator('canvas').screenshot()).equals(still), 'idle globe must stay at the chosen direction');
   await globe.getByRole('button', { name: '지구본 확대', exact: true }).click(); await page.waitForTimeout(100);
   assert(!(await globe.locator('canvas').screenshot()).equals(still), 'globe zoom must change the displayed image');
   await page.screenshot({ path: output+`/globe-${size.width}.png` });
   await globe.getByRole('button', { name: '지구본 닫기' }).click();
   const resumed = await page.evaluate(() => window.__m10Frames); await page.waitForTimeout(150);
   assert((await page.evaluate(() => window.__m10Frames)) > resumed, 'living view must resume rendering');
  }
  await page.setViewportSize({ width: 393, height: 852 });
  await page.getByRole('button', { name: '지구 관찰', exact: true }).click();
  let bound = page.getByRole('dialog', { name: '지구 자연지리 관찰 미리보기' });
  await bound.locator('canvas').waitFor();
  const area = await bound.locator('canvas').boundingBox(); let started = false;
  for (const [x,y] of [[.5,.5],[.4,.4],[.55,.45],[.35,.6],[.6,.3],[.35,.5],[.6,.5]]) {
   await bound.locator('canvas').click({ position: { x: area.width*x, y: area.height*y } });
   const start = bound.getByRole('button', { name: '선택한 지역 A에서 새로운 4인 생활 시작', exact: true });
   await start.waitFor(); page.once('dialog', d => d.accept()); await start.click();
   await page.waitForTimeout(120);
   if (!await bound.count()) { started = true; break; }
  }
  assert(started, 'a selected valid region must start a real Core world');
  await page.getByRole('button', { name: '일시정지', exact: true }).click();
  await page.getByRole('button', { name: '지구 관찰', exact: true }).click();
  bound = page.getByRole('dialog', { name: '지구 자연지리 관찰 미리보기' });
  await bound.locator('canvas').waitFor(); await bound.locator('.earth-life-region').waitFor();
  const binding = await bound.locator('.earth-life-region > span').innerText();
  for (let n=0; n<5 && await bound.count(); n++) await bound.getByRole('button', { name: '지구본 확대', exact: true }).click();
  await bound.waitFor({ state: 'hidden' });
  await page.getByRole('button', { name: '지구 관찰', exact: true }).click();
  bound = page.getByRole('dialog', { name: '지구 자연지리 관찰 미리보기' });
  await bound.locator('.earth-life-region').waitFor();
  assert.equal(await bound.locator('.earth-life-region > span').innerText(), binding, 'globe-to-life zoom must preserve Core region and seed');
  await bound.getByRole('button', { name: '이 지역의 주민 생활로 확대', exact: true }).click();
  await nav.getByRole('button', { name: '메뉴', exact: true }).click();
  await page.getByRole('button', { name: '월드 설정' }).click();
  panel = page.getByRole('dialog'); await panel.getByText('화면 성능 확인', { exact: true }).click();
  await panel.locator('.render-performance-readout').waitFor();
  const pendingDownload = page.waitForEvent('download'); await panel.getByRole('button', { name: '성능 기록 저장', exact: true }).click();
  await (await pendingDownload).saveAs(output+'/mobile-software-webgl-cost.json');
  const report = JSON.parse(fs.readFileSync(output+'/mobile-software-webgl-cost.json'));
  assert(report.sampledFrames > 0 && report.latest.calls > 0 && report.latest.triangles > 0);
  assert.equal(report.presentationProfile, 'mobile'); assert.equal(report.targetFrameBudgetMs, 1000/30);
  assert(report.surface.viewportWidth > 0 && report.surface.viewportHeight > 0);
  results.push({ path: 'actual Core app', viewports: [320,393,915], personalityFields: 14, goalPreview: true, idleGlobeFrames: 0, coreRegionHandoff: true, boundZoomReturn: true, mobileFrameCost: report });
  await page.close(); await context.close();
  }

  if (!process.env.LIFELENS_M10_UI_ONLY) {
  const visual = await browser.newPage({ viewport: { width: 700, height: 620 } });
  visual.on('pageerror', e => errors.push(e.message));
  await visual.goto(base+'tests/m10/visual-fixture.html');
  await visual.waitForFunction(() => !!window.m10Visual);
  for (const [stage, age] of [['Baby',1],['Toddler',3],['Child',8],['Teen',15],['Adult',30],['Elderly',80]]) for (const sex of ['Male','Female']) {
   const result = await visual.evaluate(([sex,stage,age]) => window.m10Visual.life(sex,stage,age), [sex,stage,age]);
   assert(result.native && result.bounds.every(Number.isFinite)); assert(result.minClearance > -.08);
   await visual.screenshot({ path: output+`/life-${stage}-${sex}.png` }); results.push(result);
  }
  for (const name of await visual.evaluate(() => window.m10Visual.cases)) {
   const result = await visual.evaluate(name => window.m10Visual.human(name), name);
   assert.equal(result.active, result.expected, name+' real production motion');
   assert(result.minClearance > -.08, name+' body penetrated ground');
   await visual.screenshot({ path: output+`/motion-${name}.png` }); results.push(result);
  }
  for (const species of ['Tiger','Wolf','Deer','Hare']) for (const behavior of ['Rest','Forage','Drink','Roam','Flee','Hunt']) {
   const result = await visual.evaluate(([s,b]) => window.m10Visual.animal(s,b), [species,behavior]);
   assert(result.native, species+' must use an actual native skin, not the fallback');
   await visual.screenshot({ path: output+`/animal-${species}-${behavior}.png` }); results.push(result);
  }
  await visual.evaluate(() => window.m10Visual.dispose()); await visual.close();
  const lifecycle = await browser.newPage(); lifecycle.on('pageerror', e => errors.push(e.message));
  await lifecycle.goto(base+'tests/m10/lifecycle-fixture.html');
  await lifecycle.waitForFunction(() => typeof window.m10Lifecycle === 'function');
  results.push({ legacyLifecycle: await lifecycle.evaluate(() => window.m10Lifecycle()) });
  await lifecycle.close();
  }
  assert.deepEqual(errors, []);
  fs.writeFileSync(output+(process.env.LIFELENS_M10_UI_ONLY ? '/ui-acceptance.json' : process.env.LIFELENS_M10_VISUAL_ONLY ? '/visual-acceptance.json' : '/full-acceptance.json'), JSON.stringify({ scope: 'actual Core app and explicitly separate production-rig display fixtures; software WebGL, no thermal or physical Android GPU claim', results, errors }, null, 2));
  console.log(process.env.LIFELENS_M10_UI_ONLY ? 'PASS actual Core mobile UI, personality/goals, globe region handoff/zoom return and mobile frame-cost export' : process.env.LIFELENS_M10_VISUAL_ONLY ? 'PASS actual native life-stage skins, human/animal motion captures and legacy renderer lifecycle' : 'PASS full M10 actual Core UI and separate production rig/lifecycle matrix');
 } finally { await browser.close(); }
})().catch(e => { console.error(e); process.exit(1); });
