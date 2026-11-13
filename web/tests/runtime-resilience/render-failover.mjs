// Exercise the production 3D failure callback and the observer engine's
// actual Canvas 2D failover. A failing mobile GPU must not reset Core state.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import ts from 'typescript';

const root = resolve(import.meta.dirname, '../../src');
const rendererCode = ts.transpileModule(
  readFileSync(resolve(root, 'render/world-renderer.ts'), 'utf8'),
  { compilerOptions: {
    module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020,
  } },
).outputText;

let lastRenderer = null;
let lastWorld = null;
class TestGPU {
  info = { autoReset: true, reset() {}, render: { calls: 1, triangles: 2 }, memory: { geometries: 1, textures: 1 }, programs: [] };
  constructor(options) {
    this.options = options;
    this.domElement = options.canvas;
    this.contextLost = false;
    this.failNextRender = false;
    this.disposed = false;
    this.renderCalls = 0;
    lastRenderer = this;
  }
  setPixelRatio(value) { this.pixelRatio = value; }
  getPixelRatio() { return this.pixelRatio; }
  setSize() {}
  dispose() { this.disposed = true; }
  getContext() { return { isContextLost: () => this.contextLost }; }
  render() {
    if (this.failNextRender) throw Error('GPU render failed');
    this.renderCalls++;
  }
}
class TestWorld {
  constructor() { lastWorld = this; }
  deltas = [];
  scene = {};
  camera = {};
  update(dt) { this.deltas.push(dt); }
  updateReflections() {}
  dispose() {}
}
let nextFrame = 1;
const scheduled = new Map();
const raf = callback => {
  const id = nextFrame++;
  scheduled.set(id, callback);
  return id;
};
const cancel = id => scheduled.delete(id);
let wall = 0;
function frame(at = wall + 16) {
  wall = at;
  const pending = [...scheduled.values()];
  scheduled.clear();
  for (const cb of pending) cb(at);
}
const oldWindow = globalThis.window;
const oldDocument = globalThis.document;
globalThis.window = { devicePixelRatio: 1 };
globalThis.document = { hidden: false, addEventListener() {}, removeEventListener() {} };
try {
  const exports = {};
  function dependency(path) {
    const result = {};
    new Function('exports', ts.transpileModule(
      readFileSync(resolve(root, path), 'utf8'),
      { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } },
    ).outputText)(result);
    return result;
  }
  const devicePerformance = dependency('runtime/device-performance.ts');
  const framePacer = dependency('render/render-frame-pacer.ts');
  const renderPerformance = dependency('render/render-performance.ts');
  const mockRequire = id => {
    if (id === 'three') return { WebGLRenderer: TestGPU, SRGBColorSpace: 'srgb' };
    if (id === './world-scene') return { WorldScene: TestWorld };
    if (id === '../runtime/device-performance') return devicePerformance;
    if (id === './render-frame-pacer') return framePacer;
    if (id === './render-performance') return renderPerformance;
    throw Error('Unmocked render dependency: ' + id);
  };
  new Function(
    'require', 'exports', 'requestAnimationFrame', 'cancelAnimationFrame',
    'performance',
    rendererCode,
  )(mockRequire, exports, raf, cancel, { now: () => wall });
  const { WorldRenderer } = exports;
  function canvas() {
    const handlers = new Map();
    return {
      handlers,
      addEventListener(name, callback) { handlers.set(name, callback); },
      removeEventListener(name) { handlers.delete(name); },
    };
  }

  {
    const c = canvas(), failures = [];
    const renderer = new WorldRenderer(c, (...args) => failures.push(args));
    renderer.start();
    assert.equal(scheduled.size, 1);
    lastRenderer.contextLost = true;
    frame();
    assert.equal(failures.length, 1, 'Silent GPU context loss must trigger failover');
    assert.match(failures[0][0], /그래픽 장치/);
    assert.equal(scheduled.size, 0, 'Dead GPU must not keep scheduling frames');
    renderer.dispose();
    assert(!c.handlers.has('webglcontextlost'));
  }
  {
    const c = canvas(), failures = [];
    const renderer = new WorldRenderer(c, (...args) => failures.push(args));
    renderer.start();
    lastRenderer.failNextRender = true;
    frame();
    assert.equal(failures.length, 1, 'Render exceptions must trigger failover');
    assert.match(failures[0][1].message, /GPU render failed/);
    assert.equal(scheduled.size, 0);
    renderer.dispose();
  }
  {
    const c = canvas(), failures = [];
    const renderer = new WorldRenderer(c, (...args) => failures.push(args));
    renderer.start();
    let defaultPrevented = false;
    c.handlers.get('webglcontextlost')({
      preventDefault() { defaultPrevented = true; },
    });
    assert(defaultPrevented, 'Restore-capable context loss must be acknowledged');
    assert.equal(failures.length, 1);
    c.handlers.get('webglcontextlost')({
      preventDefault() {},
    });
    assert.equal(failures.length, 1, 'Context loss must fail over exactly once');
    renderer.dispose();
  }

  {
    // The target Android/phone configuration must not allocate desktop-size
    // antialiased buffers for every frame, even on a high-DPR display.
    globalThis.window.devicePixelRatio = 3;
    globalThis.window.matchMedia = () => ({ matches: true });
    const c = canvas(), renderer = new WorldRenderer(c);
    assert.equal(lastRenderer.pixelRatio, 1.35);
    assert.equal(lastRenderer.options.antialias, false);
    assert.equal(lastRenderer.options.powerPreference, 'low-power');
    renderer.dispose();
    globalThis.window.devicePixelRatio = 1;
    delete globalThis.window.matchMedia;
  }

  {
    const queries = [];
    globalThis.window.matchMedia = query => {
      queries.push(query);
      return { matches: query === '(pointer: coarse)' };
    };
    assert(devicePerformance.useMobilePerformanceProfile(),
      'landscape phones and large coarse-pointer tablets retain the mobile budget');
    assert.deepEqual(queries, ['(pointer: coarse)']);
    globalThis.window.matchMedia = () => ({ matches: false });
    assert.equal(devicePerformance.useMobilePerformanceProfile(), false,
      'a fine-pointer desktop keeps its existing budget');
    delete globalThis.window.matchMedia;
  }

  for (const hz of [60, 90, 120, 144]) {
    globalThis.window.matchMedia = () => ({ matches: true });
    const renderer = new WorldRenderer(canvas());
    const startedAt = wall;
    renderer.start();
    for (let i = 1; i <= hz * 10; i++) frame(startedAt + i * 1000 / hz);
    assert.equal(lastRenderer.renderCalls, 300,
      `${hz}Hz mobile must perform 30 renders per second`);
    assert.equal(lastWorld.deltas.length, 300,
      'skip scene updates and reflection work along with the GPU draw');
    assert(Math.abs(lastWorld.deltas.reduce((sum, dt) => sum + dt, 0) - 10) < 0.001,
      'presentation receives elapsed time across skipped frames');
    renderer.stop();
    assert.equal(scheduled.size, 0, 'stopping cancels the paced RAF loop');
    renderer.dispose();
    delete globalThis.window.matchMedia;
  }

  {
    const renderer = new WorldRenderer(canvas());
    const startedAt = wall;
    renderer.start();
    frame(startedAt - 1);
    assert.equal(lastRenderer.renderCalls, 1,
      'desktop also renders a queued RAF timestamp just before start()');
    for (let i = 1; i <= 240; i++) frame(startedAt + i * 1000 / 120);
    assert.equal(lastRenderer.renderCalls, 241,
      'desktop rendering retains the display refresh rate');
    renderer.dispose();
  }

  {
    globalThis.window.matchMedia = () => ({ matches: true });
    const renderer = new WorldRenderer(canvas());
    renderer.start();
    frame(wall + 10000);
    assert.equal(lastRenderer.renderCalls, 1, 'a long pause never replays render work');
    assert.equal(lastWorld.deltas[0], 0.05, 'retain the existing animation delta cap');
    frame(wall);
    assert.equal(lastRenderer.renderCalls, 1);
    renderer.dispose();
    delete globalThis.window.matchMedia;
  }

  const engineSource = ts.transpileModule(
    readFileSync(resolve(root, 'observer-engine.ts'), 'utf8'),
    { compilerOptions: {
      module: ts.ModuleKind.ESNext, target: ts.ScriptTarget.ES2020,
    } },
  ).outputText;
  const begin = engineSource.indexOf('function enterCanvasFailover(');
  const end = engineSource.indexOf("if (requestedRenderMode === 'three-world')", begin);
  assert(begin >= 0 && end > begin);
  const failoverCode = engineSource.slice(begin, end);

  class TestCanvas2DWorld {
    constructor(canvas, overlay) {
      assert.equal(overlay.ready, false, 'GPU fallback must avoid a second WebGL renderer');
      this.canvas = canvas;
    }
    draw() {}
  }
  const domNodes = [];
  const fakeDocument = {
    createElement() {
      const element = {
        attrs: {},
        textContent: '',
        setAttribute(name, value) { this.attrs[name] = value; },
      };
      domNodes.push(element);
      return element;
    },
  };
  let disposeCount = 0;
  let resizeCount = 0;
  const restoredCore = { minute: 123, active: true };
  const createFailover = new Function(
    'LegacyCanvasWorldRenderer', 'document', 'console', 'worldSession',
    'incrementDispose', 'incrementResize',
    `
    let stopped = false;
    let effectiveRenderMode = 'three-world';
    const canvas = { style: {}, parentElement: { appendChild() {} } };
    const threeWorldCanvas = { style: {} };
    const characterCanvas = { style: {} };
    const residentContinuity = {};
    let renderFailureBanner = null;
    let legacyRenderer = null;
    let threeWorldRenderer = { dispose() { incrementDispose(); } };
    function resizeCanvas() { incrementResize(); }
    ${failoverCode}
    return {
      enterCanvasFailover,
      state: () => ({
        effectiveRenderMode, renderFailureBanner, legacyRenderer,
        threeWorldRenderer, worldSession,
        canvasVisible: canvas.style.opacity,
        threeVisible: threeWorldCanvas.style.opacity,
        threeVisibility: threeWorldCanvas.style.visibility,
      }),
    };
    `,
  );
  const fixture = createFailover(
    TestCanvas2DWorld, fakeDocument, { warn() {}, error() {} },
    restoredCore, () => disposeCount++, () => resizeCount++,
  );
  fixture.enterCanvasFailover('Test GPU failure');
  assert.equal(fixture.state().effectiveRenderMode, 'legacy-canvas');
  assert.equal(fixture.state().canvasVisible, '1');
  assert.equal(fixture.state().threeVisible, '0');
  assert.equal(fixture.state().threeVisibility, 'hidden');
  assert.equal(fixture.state().worldSession, restoredCore,
    'Rendering fallback must preserve the real world/Core session');
  assert.equal(fixture.state().renderFailureBanner.attrs.role, 'status');
  assert.equal(resizeCount, 1);
  fixture.enterCanvasFailover('Duplicate event');
  assert.equal(disposeCount, 1);
  assert.equal(domNodes.length, 1, 'Failover notice cannot multiply');
  console.log('PASS GPU loss, render exceptions and live Core-preserving 2D failover');
} finally {
  if (oldWindow === undefined) delete globalThis.window;
  else globalThis.window = oldWindow;
  globalThis.document = oldDocument;
}

