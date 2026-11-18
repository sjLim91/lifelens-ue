import assert from 'node:assert/strict';

export function illustratedWorldP2Checks({ test, source, chunk, windowOf, THREE }) {
  const { waterGeometrySignature } = source('render/water-geometry-signature.ts');
  const { WaterLayer } = source('render/water-layer.ts');
  const { ObserverFoliageClarity } = source('render/observer-foliage-clarity.ts');
  const { buildChunkGeometry } = source('render/terrain-geometry.ts');

  const freshFlow = () => windowOf([
    chunk(0, 0, 'River', .5, {
      salinity: 'Fresh', waterAvailability: .8, flowPotential: .7,
      hasDownstream: true, downstreamChunkX: 1, downstreamChunkY: 0,
    }),
    chunk(1, 0, 'Stream', .45, {
      salinity: 'Fresh', waterAvailability: .6,
    }),
    chunk(0, 1, 'Lake', .46, {
      salinity: 'Fresh', waterAvailability: .6,
    }),
  ]);

  test('P2 water cache tracks actual Core downstream, salinity, availability, and flow width', () => {
    const terrain = freshFlow();
    const baseline = waterGeometrySignature(terrain);
    assert.equal(waterGeometrySignature({
      ...terrain, chunks: [...terrain.chunks].reverse(),
    }), baseline, 'chunk order must not invalidate unchanged water');
    for (const replacement of [
      { hasDownstream: false },
      { downstreamChunkX: 0, downstreamChunkY: 1 },
      { flowPotential: .1 },
      { waterAvailability: .3 },
      { salinity: 'Brackish' },
    ]) {
      const updated = {
        ...terrain, chunks: [{ ...terrain.chunks[0], ...replacement }, ...terrain.chunks.slice(1)],
      };
      assert.notEqual(waterGeometrySignature(updated), baseline,
        'Core water change left stale signature: ' + JSON.stringify(replacement));
    }
    assert.equal(waterGeometrySignature(terrain), baseline,
      'P2 mutated authoritative terrain');
  });

  test('P2 water meshes rebuild and dispose when Core flow changes, but not for reordering', () => {
    const layer = new WaterLayer();
    try {
      const terrain = freshFlow();
      layer.setTerrain(terrain);
      const firstFlow = layer.group.children[1].geometry;
      layer.setTerrain({ ...terrain, chunks: [...terrain.chunks].reverse() });
      assert.equal(layer.group.children[1].geometry, firstFlow);
      let disposed = false;
      firstFlow.addEventListener('dispose', () => { disposed = true; });
      const nextTerrain = {
        ...terrain, chunks: [
          { ...terrain.chunks[0], downstreamChunkX: 0, downstreamChunkY: 1 },
          ...terrain.chunks.slice(1),
        ],
      };
      layer.setTerrain(nextTerrain);
      assert.notEqual(layer.group.children[1].geometry, firstFlow);
      assert.ok(disposed, 'old flow geometry not disposed');
      const rerouted = layer.group.children[1].geometry;
      layer.setTerrain({ ...nextTerrain, chunks: [
        { ...nextTerrain.chunks[0], salinity: 'Brackish' },
        ...nextTerrain.chunks.slice(1),
      ] });
      assert.notEqual(layer.group.children[1].geometry, rerouted,
        'changed Core source salinity left water stale');
      assert.ok(layer.group.children[1].geometry.getAttribute('position').count
        < rerouted.getAttribute('position').count || rerouted !== layer.group.children[1].geometry);
    } finally {
      layer.dispose();
    }
  });

  test('P2 selected resident has priority among four foliage sightlines without fake actors', () => {
    const clarity = new ObserverFoliageClarity();
    const targets = [1, 2, 3, 4].map(x => new THREE.Vector3(x, 2, 0));
    const selected = new THREE.Vector3(12, 3, -2);
    clarity.setResidents(targets, selected);
    assert.equal(clarity.uniforms.llResidentFocusCount.value, 4);
    assert.deepEqual(clarity.uniforms.llResidentFocus.value[0].toArray(), selected.toArray());
    assert.deepEqual(clarity.uniforms.llResidentFocus.value[3].toArray(), targets[2].toArray());
    clarity.setResidents([selected, selected.clone(), new THREE.Vector3(Infinity, 0, 0)], selected);
    assert.equal(clarity.uniforms.llResidentFocusCount.value, 1);
    clarity.setResidents([]);
    assert.equal(clarity.uniforms.llResidentFocusCount.value, 0);
  });

  test('P2 slope shading only tints real Core heightfield vertices; classic geometry unaffected', () => {
    const terrain = windowOf([
      chunk(-1, 0, 'None', .18, { forestCoverage01: .3, rockCoverage01: .35 }),
      chunk(0, 0, 'None', .49, { forestCoverage01: .3, rockCoverage01: .35 }),
      chunk(1, 0, 'None', .91, { forestCoverage01: .3, rockCoverage01: .35 }),
      chunk(0, 1, 'None', .62, { forestCoverage01: .3, rockCoverage01: .35 }),
    ]);
    const original = JSON.stringify(terrain);
    const classic = buildChunkGeometry(terrain.chunks[1], terrain);
    const illustrated = buildChunkGeometry(terrain.chunks[1], terrain, { visualStyle: 'illustrated' });
    try {
      assert.deepEqual(Array.from(classic.attributes.position.array), Array.from(illustrated.attributes.position.array));
      assert.deepEqual(Array.from(classic.attributes.normal.array), Array.from(illustrated.attributes.normal.array));
      assert.deepEqual(Array.from(classic.index.array), Array.from(illustrated.index.array));
      assert.notDeepEqual(Array.from(classic.attributes.color.array), Array.from(illustrated.attributes.color.array));
      assert.equal(JSON.stringify(terrain), original);
    } finally { classic.dispose(); illustrated.dispose(); }
  });
}
