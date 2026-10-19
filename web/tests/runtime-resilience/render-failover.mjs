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
class TestGPU {
  constructor(options) {
    this.domElement = options.canvas;
    this.contextLost = false;
    this.failNextRender = false;
    this.disposed = false;
    lastRenderer = this;
  }
  setPixelRatio() {}
  setSize() {}
  dispose() { this.disposed = true; }
  getContext() { return { isContextLost: () => this.contextLost }; }
  render() {
    if (this.failNextRender) throw Error('GPU render failed');
  }
}
class TestWorld {
  scene = {};
  camera = {};
  update() {}
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
function frame() {
  const pending = [...scheduled.values()];
  scheduled.clear();
  for (const cb of pending) cb(16);
}
const oldWindow = globalThis.window;
globalThis.window = { devicePixelRatio: 1 };
try {
  const exports = {};
  const mockRequire = id => {
    if (id === 'three') return { WebGLRenderer: TestGPU, SRGBColorSpace: 'srgb' };
    if (id === './world-scene') return { WorldScene: TestWorld };
    throw Error('Unmocked render dependency: ' + id);
  };
  new Function(
    'require', 'exports', 'requestAnimationFrame', 'cancelAnimationFrame',
    rendererCode,
  )(mockRequire, exports, raf, cancel);
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
}
