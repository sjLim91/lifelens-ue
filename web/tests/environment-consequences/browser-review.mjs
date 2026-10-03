import assert from 'node:assert/strict';
import { mkdirSync, writeFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { createServer } from 'vite';
import { chromium } from 'playwright';
const out = resolve(import.meta.dirname, 'review'); mkdirSync(out, { recursive: true });
const server = await createServer({ server: { host: '127.0.0.1', port: 4179 }, logLevel: 'error' });
await server.listen();
const browser = await chromium.launch({ headless: true, args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
const errors = [], metrics = [];
try {
  for (const viewport of [{ width: 390, height: 844 }, { width: 1280, height: 900 }]) {
    const page = await browser.newPage({ viewport, deviceScaleFactor: 1 });
    page.on('pageerror', error => errors.push(error.message));
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.goto('http://127.0.0.1:4179/tests/environment-consequences/fixture.html');
    await page.waitForFunction(() => window.environmentReview?.ready, { timeout: 30000 });
    for (const name of ['dry', 'rain', 'mud', 'snow', 'residue', 'fire']) {
      const result = await page.evaluate(name => window.environmentReview.renderCase(name), name);
      assert(result.stableChildren); assert(result.stableGeometry); assert(result.puddles <= 128);
      assert(result.additionalPoolDrawCalls <= 3);
      metrics.push({ viewport, ...result });
      await page.screenshot({ path: resolve(out, `${viewport.width}-${name}.png`) });
    }
    const combined = await page.evaluate(() => window.environmentReview.renderCase('rain-fire'));
    assert.equal(combined.additionalPoolDrawCalls, 3);
    assert(combined.stableChildren && combined.stableGeometry);
    metrics.push({ viewport, ...combined });
    await page.close();
  }
  assert.deepEqual(errors, [], 'browser/GPU shader errors');
  writeFileSync(resolve(out, 'browser-metrics.json'), JSON.stringify({ metrics, errors }, null, 2));
  console.log(JSON.stringify(metrics, null, 2));
} finally { await browser.close(); await server.close(); }
