import assert from 'node:assert/strict';
import { existsSync, readFileSync, writeFileSync, mkdirSync, cpSync } from 'node:fs';
import { resolve, extname } from 'node:path';
import { stripTypeScriptTypes } from 'node:module';
import * as THREE from 'three';
import { chromium } from 'playwright';
import { source } from './compile.mjs';
const root = resolve(import.meta.dirname, '../..'), out = resolve(import.meta.dirname, 'review');
mkdirSync(out, { recursive: true });
for (const p of ['vegetation-layer', 'ground-detail-layer', 'facility-layer', 'tree-asset-layer']) await source(`render/${p}.ts`);
cpSync(resolve(out, 'src'), resolve(out, 'baseline'), { recursive: true });
const base = 'https://raw.githubusercontent.com/sjLim91/lifelens-ue/8ba73745e1bd8d20108f59202c714deae1af2f1d';
const baselinePath = resolve(out, 'baseline-vegetation.ts');
if (!existsSync(baselinePath)) { const r = await fetch(`${base}/web/src/render/vegetation-layer.ts`); assert(r.ok); writeFileSync(baselinePath, await r.text()); }
const oldCode = stripTypeScriptTypes(readFileSync(baselinePath, 'utf8'), { mode: 'transform' })
  .replace(/(from\s+['"])(\.[^'"]+)(['"])/g, '$1$2.mjs$3');
writeFileSync(resolve(out, 'baseline/render/vegetation-layer.mjs'), oldCode);
let profile = readFileSync(resolve(out, 'baseline/render/vegetation-profile.mjs'), 'utf8');
for (let i = 1; i <= 3; i++) {
  const file = resolve(out, `tree${i}.glb`);
  if (!existsSync(file)) {
    const url = `https://raw.githubusercontent.com/anshaneja5/skyline-run/0c41526d8d4130c37c15e219c2c3737fa8cb4ad8/public/assets/models/tree${i}.glb`;
    const response = await fetch(url); assert(response.ok); writeFileSync(file, Buffer.from(await response.arrayBuffer()));
  }
  profile = profile.replace(new RegExp(`https:[^'"\\s]+/tree${i}\\.glb`, 'g'), `/tests/procedural-environment/review/tree${i}.glb`);
}
writeFileSync(resolve(out, 'baseline/render/vegetation-profile.mjs'), profile);
const mime = { '.html': 'text/html', '.mjs': 'text/javascript', '.js': 'text/javascript', '.glb': 'model/gltf-binary' };
async function serve(route) {
  const path = resolve(root, '.' + new URL(route.request().url()).pathname);
  if (!path.startsWith(root + '/')) { await route.fulfill({ status: 403, body: '' }); return; }
  // Playwright request routing avoids sockets, including in offline sandboxes.
  if (!existsSync(path) && path.endsWith('/three/build/three.module.js')) {
    const cjs = readFileSync(resolve(root, 'node_modules/three/build/three.cjs'), 'utf8');
    const code = 'const exports={};\n' + cjs + '\n' + Object.keys(THREE).filter(k => k !== 'default').map(k => `export const ${k}=exports.${k};`).join('\n');
    await route.fulfill({ status: 200, contentType: 'text/javascript', body: code }); return;
  }
  if (!existsSync(path)) { await route.fulfill({ status: 404, body: '' }); return; }
  await route.fulfill({ status: 200, contentType: mime[extname(path)] ?? 'application/octet-stream', body: readFileSync(path) });
}
const browser = await chromium.launch({ headless: true, ...(process.env.LIFELENS_CHROMIUM_PATH ? { executablePath: process.env.LIFELENS_CHROMIUM_PATH } : {}),
  args: ['--no-sandbox', '--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
const errors = [], metrics = [];
try {
  for (const viewport of [{ width: 1280, height: 900 }, { width: 390, height: 844 }]) {
    const page = await browser.newPage({ viewport, deviceScaleFactor: 1 });
    page.on('pageerror', e => { errors.push(e.message); console.error('PAGE', e.message); });
    page.on('console', m => { if (m.type() === 'error') { errors.push(m.text()); console.error('GPU', m.text()); } });
    await page.route('http://lifelens.local/**', serve);
    await page.goto('http://lifelens.local/tests/procedural-environment/fixture.html');
    await page.waitForFunction(() => window.proceduralReview?.ready);
    for (const name of ['gallery', 'baseline', 'forest', 'autumn', 'winter', 'harvest', 'construction', 'operational', 'ruin']) {
      metrics.push({ viewport, ...await page.evaluate(name => window.proceduralReview.renderCase(name), name) });
      await page.screenshot({ path: resolve(out, `${viewport.width}-${name}.png`) });
    }
    await page.close();
  }
  assert.deepEqual(errors, [], 'shader/runtime errors');
  writeFileSync(resolve(out, 'browser-metrics.json'), JSON.stringify({ renderer: 'Chromium SwiftShader, synthetic fixed-state fixture; not device FPS', metrics, errors }, null, 2));
  console.log(JSON.stringify(metrics, null, 2));
} finally { await browser.close(); }
