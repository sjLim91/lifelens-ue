import assert from 'node:assert/strict';
import { readFileSync, existsSync } from 'node:fs';
import { resolve, extname } from 'node:path';
import { chromium } from 'playwright';
import * as THREE from 'three';
import { source } from './compile.mjs';
for (const file of ['procedural-wildlife', 'procedural-asset-kit', 'plant-reproductive-geometry']) await source(`render/${file}.ts`);
const root = resolve(import.meta.dirname, '../..');
const browser = await chromium.launch({ headless: true,
  ...(process.env.LIFELENS_CHROMIUM_PATH ? { executablePath: process.env.LIFELENS_CHROMIUM_PATH } : {}),
  args: ['--no-sandbox', '--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
try {
  const page = await browser.newPage({ viewport: { width: 1100, height: 700 } });
  const errors = [];
  page.on('pageerror', e => { errors.push(e.message); console.error('PAGE', e.message); });
  page.on('console', message => {
    if (message.type() === 'error') { errors.push(message.text()); console.error('BROWSER', message.text()); }
  });
  await page.route('http://lifelens.local/**', async route => {
    const file = resolve(root, '.' + new URL(route.request().url()).pathname);
    if (!file.startsWith(root + '/')) {
      await route.fulfill({ status: 403, body: '' }); return;
    }
    // Match the existing passing browser review: some npm distributions expose
    // Three's CJS bundle without the module path used by the import map.
    if (!existsSync(file) && file.endsWith('/three/build/three.module.js')) {
      const cjs = readFileSync(resolve(root, 'node_modules/three/build/three.cjs'), 'utf8');
      const body = 'const exports={};\n' + cjs + '\n'
        + Object.keys(THREE).filter(key => key !== 'default').map(key => `export const ${key}=exports.${key};`).join('\n');
      await route.fulfill({ status: 200, contentType: 'text/javascript', body }); return;
    }
    if (!existsSync(file)) {
      console.error('MISSING_MODULE', new URL(route.request().url()).pathname);
      await route.fulfill({ status: 404, body: '' }); return;
    }
    await route.fulfill({ status: 200, contentType: extname(file) === '.html' ? 'text/html' : 'text/javascript', body: readFileSync(file) });
  });
  await page.goto('http://lifelens.local/tests/procedural-environment/wildlife-review.html');
  try {
    await page.waitForFunction(() => window.wildlifeReview?.ready);
  } catch (error) {
    throw new Error(`Wildlife gallery did not finish: ${errors.join('; ') || String(error)}`);
  }
  assert.deepEqual(errors, []);
  await page.screenshot({ path: resolve(import.meta.dirname, 'review/wildlife-webgl.png') });
  console.log('PASS wildlife WebGL gallery', await page.evaluate(() => window.wildlifeReview.info));
} finally { await browser.close(); }
