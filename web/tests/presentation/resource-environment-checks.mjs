import assert from 'node:assert/strict';

export function resourceProjectionChecks({ test, projection, grid, profile }) {
  const { NaturalResourceProjection: Projection, NaturalResourceProjectionCache: Cache, resourceQuantityRatio } = projection;
  const span = grid.gridCellsPerChunk;
  const terrain = { available: true, worldSeed: 'resource-seed', centerChunkX: 0, centerChunkY: 0,
    chunks: [{ x: 0, y: 0 }, { x: 1, y: 0 }, { x: -1, y: 0 }] };
  const node = (quantity = 10, material = 'Wood', extra = {}) => ({
    id: 'resource-1', gridX: span / 2, gridY: span / 2, material,
    quantity, maxQuantity: 10, renewable: true, regenerationPerDay: 1, ...extra,
  });
  const build = resources => new Projection({ available: true, resources }, terrain);
  const candidates = (p, kind = 'trees', baseline = [], budget = 6) => p.candidates(kind, 0, 0, terrain.worldSeed, baseline, budget);
  test('full Wood uses ordinary forest candidates; harvest, zero and recovery follow actual quantity', () => {
    const full = candidates(build([node()]));
    const half = candidates(build([node(5)]));
    const empty = candidates(build([node(0)]), 'trees', [{ x: 0, z: 0, scale: 1 }]);
    assert.equal(full.length, profile.slotsPerNode);
    assert.ok(half.length < full.length);
    assert.equal(empty.length, 0);
    assert.deepEqual(candidates(build([node(10)])), full);
    const young = candidates(build([node(1)]));
    assert.ok(young.length > 0 && young[0].scale < full[0].scale);
    assert.equal(young[0].x, full[0].x); assert.equal(young[0].z, full[0].z);
  });
  test('same seed/state and reordered resource snapshots produce identical dressing', () => {
    const resources = [node(), node(4, 'Wood', { id: 'second', gridX: span + span / 2 })];
    assert.deepEqual(candidates(build(resources)), candidates(build([...resources].reverse())));
    assert.equal(build(resources).signature, build([...resources].reverse()).signature);
  });
  test('forestCoverage baseline outside authoritative patches remains unchanged', () => {
    const baseline = [{ x: -2, z: 1, scale: 1 }, { x: 2, z: -1, scale: 1 }];
    assert.deepEqual(candidates(build([]), 'trees', baseline), baseline);
    assert.deepEqual(build([node(0)]).candidates('trees', 1, 0, 'resource-seed', baseline, 6), baseline);
  });
  test('PlantFood/Fiber clear both nearby herbaceous layers and restore only from stock', () => {
    for (const [material, kind] of [['Fiber', 'grass'], ['PlantFood', 'shrubs']]) {
      assert.equal(candidates(build([node(10, material)]), kind).length, profile.slotsPerNode);
      for (const layer of ['grass', 'shrubs']) {
        assert.equal(candidates(build([node(0, material)]), layer, [{ x: 0.4, z: 0.4, scale: 1 }]).length, 0);
      }
      assert.ok(candidates(build([node(1, material)]), kind).length > 0);
    }
  });
  test('Stone/Flint/CopperOre/TinOre/Clay density decreases and zero leaves no resource clump', () => {
    for (const material of ['Stone', 'Flint', 'CopperOre', 'TinOre', 'Clay']) {
      const full = candidates(build([node(10, material)]), 'rocks');
      assert.equal(full.length, profile.slotsPerNode);
      assert.ok(candidates(build([node(4, material)]), 'rocks').length < full.length);
      assert.equal(candidates(build([node(0, material)]), 'rocks', [{ x: 0.4, z: 0.4, scale: 1 }]).length, 0);
      assert.deepEqual(candidates(build([node(10, material)]), 'rocks'), full);
    }
  });
  test('resource ground position remains immutable when accessGrid changes and corridor stays clear', () => {
    const full = candidates(build([node()]));
    const changed = candidates(build([node(10, 'Wood', { hasAccessGrid: true, accessGridX: 0, accessGridY: 0 })]));
    assert.deepEqual(changed, full);
    const blocked = node(10, 'Wood', { hasAccessGrid: true,
      accessGridX: (full[0].x + grid.worldUnitsPerChunk / 2) / grid.worldUnitsPerChunk * span,
      accessGridY: (full[0].z + grid.worldUnitsPerChunk / 2) / grid.worldUnitsPerChunk * span });
    const kept = candidates(build([blocked]));
    assert.ok(kept.every(p => Math.hypot(p.x - full[0].x, p.z - full[0].z) >= profile.treeAccessClearanceWorldUnits));
  });
  test('depleted patch masks adjacent chunk baseline and handles negative grid positions', () => {
    const p = build([node(0, 'Wood', { gridX: span - 1 })]);
    assert.equal(p.candidates('trees', 1, 0, 'seed', [{ x: -3.8, z: 0, scale: 1 }], 6).length, 0);
    const negative = build([node(10, 'Wood', { gridX: -span / 2 })]);
    assert.equal(negative.candidates('trees', -1, 0, 'seed', [], 6).length, profile.slotsPerNode);
  });
  test('mobile instance budgets remain bounded even with many overlapping resource nodes', () => {
    const p = build(Array.from({ length: 100 }, (_, i) => node(10, 'Wood', { id: String(i) })));
    assert.ok(candidates(p).length <= 6);
    for (const [material, kind, budget] of [['Fiber', 'grass', 14], ['PlantFood', 'shrubs', 4], ['Clay', 'rocks', 5]]) {
      assert.ok(candidates(build(Array.from({ length: 100 }, (_, i) => node(10, material, { id: String(i) }))), kind, [], budget).length <= budget);
    }
  });
  test('mobile rock budget shares visible slots with Stone/Flint/Clay/CopperOre/TinOre', () => {
    const materials = ['Stone', 'Flint', 'Clay', 'CopperOre', 'TinOre'];
    const resources = materials.map(material => node(10, material, { id: material }));
    const full = candidates(build(resources), 'rocks', [], 5);
    assert.deepEqual(full.map(p => p.material).sort(), [...materials].sort());
    const clayDepleted = candidates(build(resources.map(r => r.material === 'Clay' ? { ...r, quantity: 0 } : r)), 'rocks', [], 5);
    assert.equal(clayDepleted.some(p => p.material === 'Clay'), false);
    assert.equal(clayDepleted.length, 4);
  });
  test('invalid resource capacity fails closed and non-natural inventory cannot create scenery', () => {
    assert.equal(resourceQuantityRatio(node(10, 'Wood', { maxQuantity: 0 })), 0);
    assert.equal(resourceQuantityRatio(node(NaN)), 0);
    assert.equal(candidates(build([node(10, 'Bronze')])).length, 0);
    assert.equal(candidates(new Projection({ available: false, resources: [node()] }, terrain)).length, 0);
  });
  test('stock-only updates invalidate only the relevant forest or ground mesh signature', () => {
    const before = build([node(10), node(10, 'Stone', { id: 'stone' })]);
    const stone = build([node(10), node(0, 'Stone', { id: 'stone' })]);
    assert.equal(before.treeSignature, stone.treeSignature);
    assert.notEqual(before.groundSignature, stone.groundSignature);
    const wood = build([node(0), node(10, 'Stone', { id: 'stone' })]);
    assert.equal(before.groundSignature, wood.groundSignature);
    assert.notEqual(before.treeSignature, wood.treeSignature);
  });
  test('500ms refresh reuses projection; seed, stock, viewport and availability changes invalidate it', () => {
    const cache = new Cache(), civ = { available: true, resources: [node()] };
    const initial = cache.get(civ, terrain);
    assert.equal(cache.get({ ...civ }, { ...terrain }), initial);
    const copy = cache.get({ ...civ, resources: [...civ.resources] }, terrain);
    assert.notEqual(copy, initial); assert.equal(copy.signature, initial.signature);
    assert.notEqual(cache.get({ ...civ, resources: [node(0)] }, terrain).signature, initial.signature);
    assert.notEqual(cache.get(civ, { ...terrain, worldSeed: 'new' }), copy);
    assert.notEqual(cache.get(civ, { ...terrain, centerChunkX: 1 }), initial);
    assert.equal(cache.get({ ...civ, available: false }, terrain).signature, '[]');
  });
}

export function resourceEnvironmentChecks({ test, source, THREE, flatWindow }) {
  const projection = source('render/natural-resource-projection.ts');
  const { WORLD_GRID_CONTRACT: grid } = source('runtime/lifelens-contract.ts');
  const { WORLD_PRESENTATION } = source('render/world-presentation-config.ts');
  resourceProjectionChecks({ test, projection, grid, profile: WORLD_PRESENTATION.naturalResources });
  const { VegetationLayer } = source('render/vegetation-layer.ts');
  const { GroundDetailLayer } = source('render/ground-detail-layer.ts');
  const { AuthoritativeSpatialTargetLayer } = source('render/authoritative-spatial-target-layer.ts');
  const { outsideFacilityFootprints, facilityPresentationFootprints } = source('render/facility-layer.ts');
  const { createVisibleWaterFootprintTester } = source('render/water-geometry.ts');
  const span = grid.gridCellsPerChunk;
  const resource = (quantity, material = 'Wood') => ({ id: 'node', gridX: span / 2, gridY: span / 2,
    material, quantity, maxQuantity: 10, renewable: true, regenerationPerDay: 1 });
  const project = (terrain, quantity, material) => new projection.NaturalResourceProjection({ available: true, resources: [resource(quantity, material)] }, terrain);
  test('real tree GLTF instance matrices and fallback canopy deplete and restore together', () => {
    const layer = new VegetationLayer(), terrain = { ...flatWindow(), chunks: [flatWindow().chunks.find(c => c.x === 0 && c.y === 0)] };
    try {
      layer.setTerrain(terrain, project(terrain, 10, 'Wood'));
      const full = layer.trunks.count;
      const matrices = layer.actualTrees.map(a => [...a.matrices]);
      assert.equal(full, 3);
      layer.setTerrain(terrain, project(terrain, 4, 'Wood')); assert.ok(layer.trunks.count < full);
      layer.setTerrain(terrain, project(terrain, 0, 'Wood')); assert.equal(layer.trunks.count, 0);
      assert.equal(layer.crowns.count, 0); assert.ok(layer.actualTrees.every(a => a.count === 0));
      layer.setTerrain(terrain, project(terrain, 10, 'Wood')); assert.equal(layer.trunks.count, full);
      assert.deepEqual(layer.actualTrees.map(a => [...a.matrices]), matrices);
      layer.trunks.instanceMatrix.needsUpdate = false;
      const version = layer.trunks.instanceMatrix.version;
      layer.setTerrain(terrain, project(terrain, 10, 'Wood'));
      assert.equal(layer.trunks.instanceMatrix.version, version);
    } finally { layer.dispose(); }
  });
  test('real ground meshes use quantity; clay lies flat and depleted resources have no proxy mesh', () => {
    const terrain = { ...flatWindow(), chunks: [flatWindow().chunks.find(c => c.x === 0 && c.y === 0)] };
    for (const [material, key] of [['Fiber', 'grass'], ['PlantFood', 'shrubs'], ['Stone', 'rocks'], ['CopperOre', 'rocks'], ['TinOre', 'rocks'], ['Clay', 'rocks']]) {
      const layer = new GroundDetailLayer();
      try {
        layer.setTerrain(terrain, project(terrain, 10, material)); assert.equal(layer[key].count, 3);
        if (material === 'Clay') {
          const m = new THREE.Matrix4(), pos = new THREE.Vector3(), rot = new THREE.Quaternion(), scale = new THREE.Vector3();
          layer.rocks.getMatrixAt(0, m); m.decompose(pos, rot, scale); assert.ok(scale.y < scale.x * 0.1);
        }
        layer.setTerrain(terrain, project(terrain, 0, material)); assert.equal(layer[key].count, 0);
      } finally { layer.dispose(); }
    }
    const proxy = new AuthoritativeSpatialTargetLayer();
    try {
      proxy.setTargets({ available: true, resources: [resource(10)] }, { available: true }, terrain);
      assert.equal(proxy.group.children.length, 2); assert.ok(proxy.group.children.every(m => m.count === 0));
    } finally { proxy.dispose(); }
  });
  test('resource dressing respects shared water/facility guard with the mobile profile', () => {
    const oldWindow = globalThis.window;
    globalThis.window = { innerWidth: 390, innerHeight: 844, matchMedia: () => ({ matches: true }) };
    try {
      for (const waterKind of ['None', 'River', 'Lake', 'Ocean']) {
        const terrain = flatWindow();
        terrain.chunks = terrain.chunks.map(c => ({ ...c, waterKind, forestCoverage01: 1, grassCoverage01: 1, shrubCoverage01: 1, rockCoverage01: 1 }));
        terrain.humanTraces = { available: true, entries: [{ id: 'shelter', kind: 'Facility', facilityKind: 'Shelter', gridX: span / 2, gridY: span / 2 }] };
        const footprint = facilityPresentationFootprints(terrain), water = createVisibleWaterFootprintTester(terrain);
        for (const [Layer, material, meshName, budget] of [[VegetationLayer, 'Wood', 'trunks', 6], [GroundDetailLayer, 'PlantFood', 'shrubs', 4], [GroundDetailLayer, 'Stone', 'rocks', 5]]) {
          const layer = new Layer();
          try {
            layer.setTerrain(terrain, project(terrain, 10, material));
            const mesh = layer[meshName]; assert.ok(mesh.count <= budget * terrain.chunks.length);
            for (let i = 0; i < mesh.count; i++) {
              const matrix = new THREE.Matrix4(); mesh.getMatrixAt(i, matrix);
              const pos = new THREE.Vector3().setFromMatrixPosition(matrix);
              assert.equal(water(pos.x, pos.z), false);
              assert.ok(outsideFacilityFootprints(pos.x, pos.z, footprint, Layer === VegetationLayer ? 0.55 : undefined));
            }
            if (waterKind === 'Ocean') assert.equal(mesh.count, 0);
          } finally { layer.dispose(); }
        }
      }
    } finally { if (oldWindow === undefined) delete globalThis.window; else globalThis.window = oldWindow; }
  });
}
