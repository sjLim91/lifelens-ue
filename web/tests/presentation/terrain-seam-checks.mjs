import assert from 'node:assert/strict';

export function terrainSeamChecks({ test, source, chunk, windowOf, withScene, near }) {
  const { createTerrainGeometryBuilder, createTerrainElevationSampler } = source('render/terrain-geometry.ts');
  const { createTerrainSurfaceSignature } = source('render/terrain-surface.ts');
  const { WORLD_GRID_CONTRACT: grid } = source('runtime/lifelens-contract.ts');
  const chunks = [];
  for (let y = -3; y <= 3; y++) for (let x = -3; x <= 3; x++) {
    chunks.push(chunk(x, y, 'None', .5 + x * .04 + y * .02 + (x * y) * .015, {
      forestCoverage01: (x + 3) / 6, grassCoverage01: (y + 3) / 6,
      moisture01: (x + y + 6) / 12, rockCoverage01: (3 - x) / 6,
    }));
  }
  const window = windowOf(chunks);
  const get = (x, y) => chunks.find(c => c.x === x && c.y === y);
  const components = (geometry, attribute, index) => {
    const a = geometry.attributes[attribute];
    return [a.getX(index), a.getY(index), a.getZ(index)];
  };

  test('neighbor terrain edges share colors and lighting normals in both axes and negative coordinates', () => {
    const build = createTerrainGeometryBuilder(window);
    for (const [x, y] of [[-1, -1], [0, 0], [1, 1]]) {
      const center = build(get(x, y)), east = build(get(x + 1, y)), south = build(get(x, y + 1));
      try {
        for (let i = 0; i <= 10; i++) {
          for (const name of ['color', 'normal']) {
            components(center, name, i * 11 + 10).forEach((v, j) => near(v, components(east, name, i * 11)[j], `${name} east seam`));
            components(center, name, 110 + i).forEach((v, j) => near(v, components(south, name, i)[j], `${name} south seam`));
          }
          near(center.attributes.position.getY(i * 11 + 10), east.attributes.position.getY(i * 11), 'east height');
        }
      } finally { center.dispose(); east.dispose(); south.dispose(); }
    }
  });

  test('terrain blending keeps the authoritative elevation sampler, topology and upward unit normals', () => {
    const before = JSON.stringify(window);
    const sample = createTerrainElevationSampler(window);
    const geometry = createTerrainGeometryBuilder(window)(get(0, 0));
    try {
      assert.equal(geometry.attributes.position.count, 121);
      assert.equal(geometry.index.count, 600);
      for (let y = 0; y <= 10; y++) for (let x = 0; x <= 10; x++) {
        const i = y * 11 + x;
        near(geometry.attributes.position.getY(i), sample(0, 0, x / 10, y / 10) * grid.elevationScale, 'height unchanged');
        const n = components(geometry, 'normal', i);
        near(Math.hypot(...n), 1, 'unit normal'); assert.ok(n[1] > 0);
      }
      assert.equal(JSON.stringify(window), before);
    } finally { geometry.dispose(); }
  });

  test('surface attributes are stable under stream ordering and observer origin changes', () => {
    const a = createTerrainGeometryBuilder(window)(get(0, 0));
    const b = createTerrainGeometryBuilder({ ...window, centerChunkX: 900, centerChunkY: -800, chunks: [...chunks].reverse() })(get(0, 0));
    try {
      for (const name of ['position', 'normal', 'color']) assert.deepEqual(a.attributes[name].array, b.attributes[name].array);
    } finally { a.dispose(); b.dispose(); }
  });

  test('missing streamed neighbors keep finite surfaces and matching shared edges', () => {
    const partial = windowOf([get(-1, 0), get(0, 0)]);
    const build = createTerrainGeometryBuilder(partial);
    const a = build(get(-1, 0)), b = build(get(0, 0));
    try {
      for (const name of ['color', 'normal']) {
        assert.ok([...a.attributes[name].array, ...b.attributes[name].array].every(Number.isFinite));
        for (let i = 0; i <= 10; i++) components(a, name, i * 11 + 10).forEach((v, j) => near(v, components(b, name, i * 11)[j], 'partial seam'));
      }
    } finally { a.dispose(); b.dispose(); }
  });

  test('3D surface cache reacts to neighbor palette and normal dependencies without rebuilding unchanged meshes', () => withScene(scene => {
    scene.setTerrain(window);
    const old = scene.terrainMeshes.get('0:0').mesh.geometry;
    const material = scene.terrainMeshes.get('0:0').mesh.material;
    assert.equal(material.color.getHex(), 0xffffff, 'per-chunk material tint would recreate the grid');
    scene.setTerrain({ ...window, chunks: [...chunks].reverse() });
    assert.equal(scene.terrainMeshes.get('0:0').mesh.geometry, old);
    let disposed = false; old.addEventListener('dispose', () => { disposed = true; });
    const changed = { ...window, chunks: chunks.map(c => c.x === 1 && c.y === 0 ? { ...c, forestCoverage01: 0 } : c) };
    scene.setTerrain(changed);
    assert.notEqual(scene.terrainMeshes.get('0:0').mesh.geometry, old);
    assert.ok(disposed);
    const distant = { ...window, chunks: chunks.map(c => c.x === 2 && c.y === 0 ? { ...c, elevation01: .95 } : c) };
    assert.notEqual(createTerrainSurfaceSignature(window)(get(0, 0)), createTerrainSurfaceSignature(distant)(get(0, 0)));
  }));
}
