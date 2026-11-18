import assert from 'node:assert/strict';

export function illustratedRegionMapChecks({ test, source, chunk, windowOf }) {
  const {
    illustratedRegionProjection,
    illustratedRegionHit,
    paintIllustratedRegion,
  } = source('render/illustrated-region-map.ts');

  const terrain = windowOf([
    chunk(-1, 0, 'River', .4, { forestCoverage01: .6 }),
    chunk(0, 0, 'None', .5, { forestCoverage01: .8, grassCoverage01: .6 }),
    chunk(1, 0, 'Ocean', .15),
    chunk(0, 1, 'None', .6, { rockCoverage01: .8 }),
  ]);
  const view = { width: 360, height: 240, centerX: 0, centerY: 0, zoom: 1.5 };

  test('illustrated map picks only observed Core chunks, with negative coordinates and zoom', () => {
    const projection = illustratedRegionProjection(terrain, view);
    assert.ok(projection);
    assert.ok(projection.tile > 0);
    assert.deepEqual(illustratedRegionHit(terrain, view, projection.originX, projection.originY), { x: 0, y: 0 });
    assert.deepEqual(illustratedRegionHit(terrain, view,
      projection.originX - projection.tile, projection.originY), { x: -1, y: 0 });
    assert.equal(illustratedRegionHit(terrain, view,
      projection.originX + projection.tile * 3, projection.originY), null);
    assert.equal(illustratedRegionHit(terrain, view, -2, 20), null);
    assert.equal(illustratedRegionHit({ ...terrain, available: false }, view, 180, 120), null);
  });

  test('illustrated map paints only Core-sourced settlement and positioned resident markers', () => {
    const events = [];
    const ctx = new Proxy({}, {
      get(target, prop) {
        return target[prop] ?? ((...args) => { events.push([prop, ...args]); });
      },
      set(target, prop, value) { target[prop] = value; return true; },
    });
    const original = structuredClone(terrain);
    const input = {
      terrain,
      view,
      residents: [
        { id: 'resident-1', hasPosition: true, gridX: 16, gridY: 16 },
        { id: 'resident-2', hasPosition: false, gridX: 16, gridY: 16 },
      ],
      civilization: {
        available: true,
        settlements: [
          { id: 'settlement-1', gridX: 16, gridY: 16, active: true },
          { id: 'settlement-2', gridX: 16, gridY: 16, active: false },
        ],
      },
    };
    const stats = paintIllustratedRegion(ctx, input);
    assert.equal(stats.drawnChunks, 4);
    assert.equal(stats.residentMarkers, 1);
    assert.equal(stats.settlementMarkers, 1);
    assert.ok(events.some(([name]) => name === 'ellipse'), 'stylized terrain decoration missing');
    assert.deepEqual(terrain, original, 'map painter mutated authoritative Core observations');
    const previous = structuredClone(events);
    events.length = 0;
    assert.deepEqual(paintIllustratedRegion(ctx, input), stats);
    assert.deepEqual(events, previous, 'same Core seed/window changes illustrated rendering');
  });

  test('illustrated map unavailable Core window renders no invented terrain or markers', () => {
    const ctx = new Proxy({}, {
      get() { return () => undefined; },
      set() { return true; },
    });
    const stats = paintIllustratedRegion(ctx, {
      terrain: { ...terrain, available: false },
      civilization: { available: true, settlements: [{ id: 'fake', active: true, gridX: 0, gridY: 0 }] },
      residents: [{ id: 'fake', hasPosition: true, gridX: 0, gridY: 0 }],
      view,
    });
    assert.deepEqual(stats, { drawnChunks: 0, residentMarkers: 0, settlementMarkers: 0 });
  });
}
