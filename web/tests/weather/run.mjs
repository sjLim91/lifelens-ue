import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import ts from 'typescript';

const require = createRequire(import.meta.url);
const source = readFileSync(new URL('../../src/render/weather-layer.ts', import.meta.url), 'utf8');
const compiled = ts.transpileModule(source, {
  compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
}).outputText;
const module = { exports: {} };
new Function('require', 'module', 'exports', compiled)(require, module, module.exports);
const { WeatherLayer } = module.exports;
let passed = 0;
function test(name, run) {
  const layer = new WeatherLayer();
  try { run(layer); passed++; console.log(`PASS ${name}`); }
  finally { layer.dispose(); }
}
const wet = { available: true, summary: 'Rain', precipitationType: 'Rain', precipitationIntensity01: 0.5, windIntensity01: 0.25 };
function points(layer) { return layer.group.children[0].geometry.attributes.position.array; }
function distribution(array, count) {
  const cells = new Set();
  let close = 0;
  for (let n = 0; n < count; n++) {
    const x = array[n * 3], z = array[n * 3 + 2];
    cells.add(`${Math.floor((x + 48) / 12)},${Math.floor((z + 48) / 12)}`);
    if (Math.abs(x - z) < 1) close++;
  }
  assert.ok(cells.size >= 60, `occupied only ${cells.size}/64 ground cells`);
  assert.ok(close / count < 0.08, `${close}/${count} drops collapsed onto x=z`);
}
test('rain fills a volume rather than a diagonal sheet, including light rain', layer => {
  layer.setEnvironment({ ...wet, precipitationIntensity01: 0.01 });
  distribution(points(layer), layer.group.children[0].geometry.drawRange.count);
});
test('falling rain remains distributed after many wrap cycles', layer => {
  layer.setEnvironment(wet);
  for (let i = 0; i < 1200; i++) layer.update(1 / 60);
  distribution(points(layer), layer.group.children[0].geometry.drawRange.count);
  const a = points(layer), bins = new Set();
  for (let i = 0; i < 800; i++) {
    assert.ok(a[i * 3 + 1] >= -2 && a[i * 3 + 1] <= 68);
    bins.add(Math.floor((a[i * 3 + 1] + 2) / 7));
  }
  assert.equal(bins.size, 10);
});
test('Core dry state overrides an older rainy summary', layer => {
  layer.setEnvironment(wet);
  layer.setEnvironment({ ...wet, precipitationType: 'None' });
  assert.equal(layer.group.visible, false);
  layer.setEnvironment({ ...wet, precipitationIntensity01: 0 });
  assert.equal(layer.group.visible, false);
});
test('unavailable or removed environment stops precipitation', layer => {
  for (const value of [null, { ...wet, available: false }]) {
    layer.setEnvironment(wet); layer.setEnvironment(value);
    assert.equal(layer.group.visible, false);
  }
});
test('legacy summary-only observations still render rain and snow', layer => {
  layer.setEnvironment({ summary: 'Rain' });
  assert.equal(layer.group.children[0].visible, true);
  layer.setEnvironment({ summary: 'Snow' });
  assert.equal(layer.group.children[0].visible, false);
  assert.equal(layer.group.children[1].visible, true);
});
test('pixel ratio and aspect update with the actual renderer', layer => {
  const rain = layer.group.children[0];
  for (const ratio of [1, 2]) {
    rain.onBeforeRender({ getSize: v => v.set(390, 844), getPixelRatio: () => ratio });
    assert.equal(rain.material.uniforms.uPixelRatio.value, ratio);
    assert.deepEqual(rain.material.uniforms.uViewport.value.toArray(), [390, 844]);
  }
});
test('wind streak direction uses the same velocity as particle movement', layer => {
  layer.setEnvironment({ ...wet, windIntensity01: 1 });
  const before = points(layer).slice(0, 3);
  const velocity = layer.group.children[0].material.uniforms.uVelocity.value;
  layer.update(0.001);
  assert.ok(Math.abs((points(layer)[0] - before[0]) / 0.001 - velocity.x) < 0.01);
  assert.ok(Math.abs((points(layer)[1] - before[1]) / 0.001 - velocity.y) < 0.01);
});
console.log(`${passed} weather regression checks passed`);
