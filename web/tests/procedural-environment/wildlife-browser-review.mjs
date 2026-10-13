import assert from 'node:assert/strict';
import { readFileSync, existsSync } from 'node:fs';
import { resolve, extname } from 'node:path';
import { chromium } from 'playwright';
import { source } from './compile.mjs';
for (const file of ['procedural-wildlife', 'procedural-asset-kit', 'plant-reproductive-geometry']) await source(`render/${file}.ts`);
const root = resolve(import.meta.dirname, '../..');
const browser = await chromium.launch({ headless: true,
  args: ['--no-sandbox', '--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
try {
  const page = await browser.newPage({ viewport: { width: 1100, height: 700 } });
  const errors = [];
  page.on('pageerror', e => errors.push(e.message));
  await page.route('http://lifelens.local/**', async route => {
    const file = resolve(root, '.' + new URL(route.request().url()).pathname);
    if (!file.startsWith(root + '/') || !existsSync(file)) {
      await route.fulfill({ status: 404, body: '' }); return;
    }
    await route.fulfill({ status: 200, contentType: extname(file) === '.html' ? 'text/html' : 'text/javascript', body: readFileSync(file) });
  });
  await page.goto('http://lifelens.local/tests/procedural-environment/wildlife-review.html');
  await page.waitForFunction(() => window.wildlifeReview?.ready);
  assert.deepEqual(errors, []);
  await page.screenshot({ path: resolve(import.meta.dirname, 'review/wildlife-webgl.png') });
  console.log('PASS wildlife WebGL gallery', await page.evaluate(() => window.wildlifeReview.info));
} finally { await browser.close(); }
