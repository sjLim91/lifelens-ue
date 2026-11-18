import assert from 'node:assert/strict';

export function isometricM0Checks({ test, source, chunk, windowOf }) {
  const { createIsometricProjection } = source('render/isometric-projection.ts');
  const { paintIsometricWorld, IsometricCanvasWorldRenderer } =
    source('render/isometric-canvas-world-renderer.ts');
  const { readRenderMode, writeRenderMode, urlForRenderMode } = source('render/render-mode.ts');
  const { ResidentContinuity } = source('runtime/resident-continuity.ts');
  const { WORLD_GRID_CONTRACT } = source('runtime/lifelens-contract.ts');

  const view = {
    width: 600, height: 360, extent: 3, zoom: 1.8,
    centerChunkX: 0, centerChunkY: 0, panX: 0, panZ: 0,
  };
  const makeContext = () => {
    const ops = [];
    const ctx = {};
    for (const op of ['clearRect','fillRect','fillText','beginPath','moveTo','lineTo',
      'closePath','fill','stroke','arc','ellipse','strokeRect','quadraticCurveTo']) {
      ctx[op] = (...args) => { ops.push([op, ...args]); };
    }
    return { ctx, ops };
  };
  const resident = {
    id: 'real-resident', alive: true, hasPosition: true,
    gridX: WORLD_GRID_CONTRACT.gridCellsPerChunk / 2,
    gridY: WORLD_GRID_CONTRACT.gridCellsPerChunk / 2, sex: 'Female',
  };
  const base = () => windowOf([chunk(0, 0, 'None', .42)], 0, 0, 'isometric-seed');
  const options = terrain => ({
    terrain, residents: [resident], civilization: { available: false },
    centerX: 0, centerY: 0, zoom: view.zoom, panX: 0, panZ: 0,
    selectedResidentId: resident.id,
  });

  test('M0 45-degree orthographic projection roundtrips Core coordinates with relief and pan', () => {
    const iso = createIsometricProjection({ ...view, panX: 1.25, panZ: -2.5 });
    const point = iso.project(.12, -.3, .75);
    const inverse = iso.inverse(point.x, point.y, .75);
    assert.ok(Math.abs(inverse.x - (.12 * 8)) < 1e-8);
    assert.ok(Math.abs(inverse.z - (-.3 * 8)) < 1e-8);
    assert.ok(iso.tile > 0 && iso.relief > 0);
    const diagonal = iso.project(1, 1, .5), origin = iso.project(0, 0, .5);
    assert.ok(Math.abs(diagonal.x - origin.x) < 1e-9);
    assert.ok(diagonal.y > origin.y);
  });

  test('M0 2.5D paint follows Core detailed 17x17 elevation, never modifying Core inputs', () => {
    const flat = base();
    const samples = Array.from({ length: 289 }, (_, i) => .2 + (i % 17) / 16 * .5);
    const detailed = windowOf([chunk(0, 0, 'None', .42, { elevationSamples: samples })],
      0, 0, 'isometric-seed');
    const before = JSON.stringify(detailed);
    const a = makeContext(), b = makeContext();
    const continuity = new ResidentContinuity();
    const first = paintIsometricWorld(a.ctx, view, options(flat), continuity);
    const second = paintIsometricWorld(b.ctx, view, options(detailed), continuity);
    assert.equal(first.drawnChunks, 1);
    assert.equal(second.drawnChunks, 1);
    assert.equal(second.residentMarkers, 1);
    assert.equal(JSON.stringify(detailed), before);
    const flatPoints = a.ops.filter(x => x[0] === 'moveTo');
    const detailedPoints = b.ops.filter(x => x[0] === 'moveTo');
    assert.notDeepEqual(flatPoints, detailedPoints, 'true Core 17x17 relief was ignored');
    assert.ok(b.ops.filter(x => x[0] === 'fill').length > 0);
  });

  test('M0 water only draws a flow line for an explicit downstream Core edge', () => {
    const river = chunk(0, 0, 'River', .5, {
      hasDownstream: false, downstreamChunkX: 1, downstreamChunkY: 0,
    });
    const downstream = chunk(1, 0, 'River', .49);
    const disconnected = windowOf([river, downstream]);
    const connected = windowOf([{ ...river, hasDownstream: true }, downstream]);
    const a = makeContext(), b = makeContext();
    const continuity = new ResidentContinuity();
    const localView = { ...view, extent: 2, zoom: 1 };
    paintIsometricWorld(a.ctx, localView, { ...options(disconnected), residents: [] }, continuity);
    paintIsometricWorld(b.ctx, localView, { ...options(connected), residents: [] }, continuity);
    assert.ok(b.ops.filter(x => x[0] === 'stroke').length >
      a.ops.filter(x => x[0] === 'stroke').length,
      'explicit Core downstream edge did not change the river presentation');
    assert.equal(river.hasDownstream, false);
  });

  test('M0 marker picks are derived from actual resident and facility observations', () => {
    const trace = {
      id: 'facility:real-one', kind: 'Facility', gridX: 0, gridY: 0,
      facilityKind: 'FirePit', state: 'Operational', progress01: 1,
      deliveredMaterialUnits: 2, requiredMaterialUnits: 2,
      active: true, lit: false, sourceResidentId: resident.id,
      cropPlanted: false, cropGrowth01: 0, cropMoisture01: 0,
      cropCare01: 0, cropHarvestUnits: 0,
    };
    const terrain = { ...base(), humanTraces: { total: 1, entries: [trace] } };
    const { ctx } = makeContext();
    const canvas = {
      width: 600, height: 360,
      getContext: kind => kind === '2d' ? ctx : null,
      getBoundingClientRect: () => ({ left: 10, top: 20, width: 600, height: 360 }),
    };
    const renderer = new IsometricCanvasWorldRenderer(canvas, new ResidentContinuity());
    renderer.draw({ ...options(terrain), civilization: {
      available: true, settlements: [{
        id: 'settlement-one', active: true, gridX: 32, gridY: 32,
      }],
    } });
    const projection = createIsometricProjection({ ...view, extent: 1 });
    const residentPoint = projection.gridPoint(resident.gridX, resident.gridY, .42);
    assert.equal(renderer.pickResident(10 + residentPoint.x, 20 + residentPoint.y), resident.id);
    const tracePoint = projection.gridPoint(trace.gridX, trace.gridY, .42);
    assert.equal(renderer.pickHumanTrace(10 + tracePoint.x, 20 + tracePoint.y), trace.id);
    assert.equal(renderer.pickResident(-999, -999), null);
    assert.equal(renderer.pickHumanTrace(-999, -999), null);
    assert.equal(renderer.pickSettlement(-999, -999), null);
    const ground = renderer.groundPointAtClient(10 + residentPoint.x, 20 + residentPoint.y);
    assert.ok(ground && Number.isFinite(ground.x) && Number.isFinite(ground.z));
  });

  test('M0 render selection is URL-only and does not change world seed, safe mode or hash', () => {
    const oldWindow = globalThis.window;
    const oldStorage = globalThis.localStorage;
    const data = new Map();
    globalThis.localStorage = {
      getItem: key => data.get(key) ?? null, setItem: (key, value) => data.set(key, value),
    };
    try {
      globalThis.window = { location: { search: '' } };
      assert.equal(readRenderMode(), 'three-world');
      writeRenderMode('isometric-canvas');
      assert.equal(readRenderMode(), 'isometric-canvas');
      const url = urlForRenderMode('https://example.com/lifelens-ue/?safe=1&seed=4242#world',
        'isometric-canvas');
      assert.equal(new URL(url).searchParams.get('view'), '2.5d');
      assert.equal(new URL(url).searchParams.get('seed'), '4242');
      assert.equal(new URL(url).searchParams.get('safe'), '1');
      assert.equal(new URL(url).hash, '#world');
      globalThis.window.location.search = new URL(url).search;
      assert.equal(readRenderMode(), 'isometric-canvas');
      globalThis.window.location.search = '?view=3d';
      assert.equal(readRenderMode(), 'three-world');
    } finally {
      if (oldWindow === undefined) delete globalThis.window;
      else globalThis.window = oldWindow;
      if (oldStorage === undefined) delete globalThis.localStorage;
      else globalThis.localStorage = oldStorage;
    }
  });
}
