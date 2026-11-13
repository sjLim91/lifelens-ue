// Optional real-browser review; no screenshots are required by merge automation.
const { chromium } = require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const assert = require('node:assert/strict');
const fs = require('node:fs');
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.LIFELENS_CHROMIUM_PATH || '/usr/bin/chromium',
    headless: true, args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  const output = process.env.LIFELENS_M10_REVIEW_DIR || '/tmp/lifelens-m10-review'; fs.mkdirSync(output, { recursive: true });
  const context = await browser.newContext({ viewport: { width: 393, height: 852 }, hasTouch: true, isMobile: true, deviceScaleFactor: 1 });
  // Verify the free Core/3D baseline without downloading the Qwen model in a software-GPU runner.
  await context.addInitScript(() => Object.defineProperty(navigator, 'gpu', { value: undefined }));
  const page = await context.newPage(), errors = [], failedAssets = [];
  page.on('pageerror', e => errors.push(e.message));
  page.on('response', r => { if (r.status() >= 400 && /vendor|runtime|assets/.test(r.url())) failedAssets.push([r.status(), r.url()]); });
  try {
    await page.goto((process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:4173/') + 'index.html?m10-review', { waitUntil: 'domcontentloaded' });
    await page.waitForFunction(() => document.querySelector('#status')?.textContent?.includes('시뮬레이션 코어 연결됨'), null, { timeout: 90000 });
    await page.getByRole('button', { name: '일시정지', exact: true }).click();
    await page.waitForTimeout(2500);
    const nav = page.getByRole('navigation', { name: '주요 관찰 메뉴' });
    await page.screenshot({ path: output + '/mobile-world.png' });
    await nav.getByRole('button', { name: '주민', exact: true }).click();
    const residents = page.getByRole('dialog', { name: '주민', exact: true });
    assert.equal(await residents.locator('.resident-card').count(), 4);
    await residents.locator('.resident-card').first().click();
    const detail = page.getByRole('dialog');
    await detail.getByText('생각과 실제 행동', { exact: true }).click();
    await detail.locator('[aria-label="생각과 실제 행동 기록"]').waitFor();
    assert(!(await detail.locator('.resident-causal-timeline').innerText()).includes('목표에 도달함'), 'fresh Core must not invent success');
    assert(await detail.locator('[role=meter]').count() >= 5);
    await page.screenshot({ path: output + '/mobile-resident.png' });
    await page.keyboard.press('Escape');
    await nav.getByRole('button', { name: '메뉴', exact: true }).click();
    await page.getByRole('button', { name: '월드 설정' }).click();
    const settings = page.getByRole('dialog');
    await settings.getByText('화면 성능 확인', { exact: true }).click();
    await settings.locator('.render-performance-readout').waitFor();
    assert(await settings.getByRole('button', { name: '성능 기록 저장', exact: true }).isVisible());
    const downloadWait = page.waitForEvent('download');
    await settings.getByRole('button', { name: '성능 기록 저장', exact: true }).click();
    const download = await downloadWait; await download.saveAs(output + '/software-gpu-render-performance.json');
    const report = JSON.parse(fs.readFileSync(output + '/software-gpu-render-performance.json'));
    assert.equal(report.schema, 'lifelens-render-performance-v1'); assert(report.sampledFrames > 0);
    assert(report.cpuFrameMs.p50 > 0); assert(report.latest.triangles > 0); assert(report.latest.calls > 0);
    assert.equal(report.surface.mobile, true);
    await page.screenshot({ path: output + '/mobile-performance.png' });
    assert(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth));
    await page.setViewportSize({ width: 915, height: 412 });
    await page.waitForTimeout(2500);
    assert(await settings.locator('.mobile-modal-body').evaluate(el => el.scrollWidth <= el.clientWidth));
    await page.screenshot({ path: output + '/landscape-performance.png' });
    fs.writeFileSync(output + '/browser-result.json', JSON.stringify({ runtime: 'fresh Core WASM', renderer: 'Chromium software WebGL; Qwen unavailable by test setup', errors, failedAssets, report }, null, 2));
    assert.deepEqual(errors, []); assert.deepEqual(failedAssets, []);
    console.log('PASS fresh Core + actual WebGL mobile resident timeline, no fabricated success, 3D performance download, portrait/landscape fit');
  } finally { await browser.close(); }
})().catch(e => { console.error(e); process.exit(1); });
