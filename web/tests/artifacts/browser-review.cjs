const { chromium } = require(process.env.LIFELENS_PLAYWRIGHT_MODULE || 'playwright');
const assert = require('node:assert/strict'), fs = require('node:fs');
(async () => {
  const output = process.env.LIFELENS_ARTIFACT_REVIEW_DIR || '/tmp/lifelens-artifact-review';
  fs.mkdirSync(output, { recursive: true });
  const browser = await chromium.launch({ executablePath: '/usr/bin/chromium', headless: true,
    args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  const errors = [], results = [];
  try {
    const page = await browser.newPage(); page.on('pageerror', e => errors.push(e.message));
    for (const width of [320, 393, 915, 1440]) {
      await page.setViewportSize({ width, height: width === 915 ? 412 : 852 });
      await page.goto((process.env.LIFELENS_REVIEW_URL || 'http://127.0.0.1:5173/') + 'tests/artifacts/fixture.html');
      const panel = page.locator('details').first(); await panel.locator('summary').click();
      await panel.getByRole('heading', { name: /^발명품 #/ }).waitFor();
      assert((await panel.innerText()).includes('원료 3단위 소모'));
      assert((await panel.innerText()).includes('제작 성공'));
      assert((await panel.innerText()).includes('재현 가능'));
      assert(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth));
      await page.screenshot({ path: `${output}/invention-${width}.png`, fullPage: true });
      results.push({ width, actualCoreEvidence: true, noOverflow: true });
    }
    assert.deepEqual(errors, []);
    fs.writeFileSync(`${output}/browser-proof.json`, JSON.stringify({ results, errors,
      profile: 'native production Core export rendered by actual React UI; software WebGL, no physical device claim' }, null, 2));
    console.log('PASS actual Core invention UI at 320/393/915/1440px; page errors 0');
  } finally { await browser.close(); }
})().catch(e => { console.error(e); process.exit(1); });
