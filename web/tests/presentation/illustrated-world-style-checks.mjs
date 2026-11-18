import assert from 'node:assert/strict';

export function illustratedWorldStyleChecks({ test, source, chunk, windowOf }) {
  const {
    worldStyleFromSearch, urlForWorldVisualStyle, ILLUSTRATED_GROUND_PALETTE,
  } = source('render/world-visual-style.ts');
  const { terrainSurfaceColor } = source('render/terrain-surface.ts');
  const { buildChunkGeometry } = source('render/terrain-geometry.ts');

  test('P1 illustrated style is opt-in; URL switch preserves seed and other query inputs', () => {
    assert.equal(worldStyleFromSearch(''), 'classic');
    assert.equal(worldStyleFromSearch('?world-style=illustrated'), 'illustrated');
    assert.equal(worldStyleFromSearch('?world-style=unknown'), 'classic');
    const original = 'https://example.com/lifelens-ue/?safe=1&seed=4242#view';
    const changed = urlForWorldVisualStyle(original, 'illustrated');
    assert.equal(worldStyleFromSearch(new URL(changed).search), 'illustrated');
    assert.equal(new URL(changed).searchParams.get('seed'), '4242');
    assert.equal(new URL(changed).searchParams.get('safe'), '1');
    assert.equal(new URL(changed).hash, '#view');
    assert.equal(urlForWorldVisualStyle(changed, 'classic'), original);
    assert.equal(ILLUSTRATED_GROUND_PALETTE.foliage.length, 7);
  });

  test('P1 keeps original terrain colors as default; illustrated ecology is distinct and deterministic', () => {
    const grass = Object.freeze({
      waterKind: 'None', elevation01: .47, moisture01: .7,
      forestCoverage01: .76, grassCoverage01: .9, rockCoverage01: .08,
    });
    const standard = terrainSurfaceColor(grass);
    const illustrated = terrainSurfaceColor(grass, 'illustrated');
    assert.equal(terrainSurfaceColor(grass).getHex(), standard.getHex());
    assert.notEqual(standard.getHex(), illustrated.getHex());
    assert.equal(terrainSurfaceColor(grass, 'illustrated').getHex(), illustrated.getHex());
    assert.equal(grass.forestCoverage01, .76);
  });

  test('P1 illustrated 3D terrain changes only vertex colors, never Core height, indices or normals', () => {
    const terrain = windowOf([
      chunk(0, 0, 'None', .45, {
        forestCoverage01: .72, grassCoverage01: .85, rockCoverage01: .06, moisture01: .68,
      }),
      chunk(1, 0, 'None', .71, {
        forestCoverage01: .12, grassCoverage01: .3, rockCoverage01: .82, moisture01: .21,
      }),
    ]);
    const unchanged = JSON.stringify(terrain);
    const regular = buildChunkGeometry(terrain.chunks[0], terrain);
    const painted = buildChunkGeometry(terrain.chunks[0], terrain, { visualStyle: 'illustrated' });
    try {
      for (const attr of ['position', 'normal', 'uv']) {
        assert.deepEqual(Array.from(regular.attributes[attr].array),
          Array.from(painted.attributes[attr].array), `P1 modified ${attr}`);
      }
      assert.deepEqual(Array.from(regular.index.array), Array.from(painted.index.array));
      assert.notDeepEqual(Array.from(regular.attributes.color.array),
        Array.from(painted.attributes.color.array), 'P1 did not change visual color');
      assert.equal(JSON.stringify(terrain), unchanged, 'P1 altered source Core DTO');
    } finally {
      regular.dispose();
      painted.dispose();
    }
  });
}
