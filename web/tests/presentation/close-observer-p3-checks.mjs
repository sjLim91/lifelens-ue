import assert from 'node:assert/strict';

export function closeObserverP3Checks({ test, source, THREE, chunk, windowOf, withScene }) {
  const { FacilityLayer } = source('render/facility-layer.ts');
  const { WORLD_GRID_CONTRACT } = source('runtime/lifelens-contract.ts');
  const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const elevationScale = WORLD_GRID_CONTRACT.elevationScale;
  const trace = {
    id: 'facility:storage-one', kind: 'Facility',
    gridX: span / 2, gridY: span / 2,
    facilityKind: 'PrimitiveStorage', sourceResidentId: 'resident-1',
    state: 'Operational', active: true, lit: false,
    progress01: 1, deliveredMaterialUnits: 10, requiredMaterialUnits: 10,
  };
  const terrainWithSamples = height => ({
    ...windowOf([chunk(0, 0, 'None', .5, {
      elevationSamples: Array(17 * 17).fill(height),
    })]),
    humanTraces: { total: 1, entries: [trace] },
  });
  const civilizationWithStorage = linkedStorage => ({
    available: true,
    facilities: [{
      id: 'storage-one', kind: 'PrimitiveStorage', gridX: trace.gridX,
      gridY: trace.gridY, state: 'Operational', active: true,
      linkedStorage, durability: 1, workProgress: 1,
      deliveredMaterialUnits: 10, requiredMaterialUnits: 10,
    }],
    storages: linkedStorage ? [{ id: linkedStorage, gridX: trace.gridX, gridY: trace.gridY, items: [] }] : [],
  });

  test('P3 constructed facility follows detailed Core elevation even when chunk elevation01 is unchanged', () => {
    const layer = new FacilityLayer();
    try {
      const first = terrainWithSamples(.25);
      layer.setTerrain(first);
      const entry = layer.structures.get(trace.id);
      assert.ok(entry, 'real Core facility trace missing from scene');
      const baseline = entry.group.position.y;
      layer.setTerrain(terrainWithSamples(.75));
      const next = layer.structures.get(trace.id);
      assert.ok(next);
      assert.equal(next.group, entry.group, 'unchanged facility object should be reused');
      assert.ok(Math.abs(next.group.position.y - baseline - .5 * elevationScale) < .00001,
        'facility remained at the old ground height');
      assert.equal(first.chunks[0].elevationSamples[0], .25,
        'facility presentation changed Core samples');
    } finally {
      layer.dispose();
    }
  });

  test('P3 selected operational storage opens only its existing observed roof/walls', () => {
    const layer = new FacilityLayer();
    try {
      layer.setCivilization(civilizationWithStorage('real-storage-id'), terrainWithSamples(.5));
      const entry = layer.structures.get(trace.id);
      assert.ok(entry, 'linked facility not rendered');
      const panels = entry.group.children.filter(part => part.userData.storageCutaway === true);
      assert.ok(panels.length > 0, 'no physical panels to cut away');
      assert.ok(panels.every(part => part.visible), 'unselected facility should remain whole');
      layer.setSelectedTrace(trace.id);
      assert.ok(panels.every(part => !part.visible), 'selected storage remains closed');
      layer.setSelectedTrace(null);
      assert.ok(panels.every(part => part.visible), 'selection clearing did not restore original building');
      // The pre-existing working-resident cutaway remains authoritative.
      layer.storageInteractions.add('real-storage-id');
      layer.updateStorageCutaway();
      assert.ok(panels.every(part => !part.visible));
      layer.storageInteractions.clear();
      layer.updateStorageCutaway();
      assert.ok(panels.every(part => part.visible));
    } finally {
      layer.dispose();
    }
  });

  test('P3 selection cannot open incomplete or unlinked storage or an unrelated structure', () => {
    const layer = new FacilityLayer();
    try {
      layer.setCivilization(civilizationWithStorage(null), terrainWithSamples(.5));
      const panels = layer.structures.get(trace.id)?.group.children
        .filter(part => part.userData.storageCutaway === true) ?? [];
      assert.ok(panels.length > 0);
      layer.setSelectedTrace(trace.id);
      assert.ok(panels.every(part => part.visible),
        'invented linked storage inventory through focus-only selection');
      layer.setSelectedTrace('facility:other');
      assert.ok(panels.every(part => part.visible));
    } finally {
      layer.dispose();
    }
  });

  test('P3 WorldScene trace selection forwards to the facility model and clears it', () => {
    withScene(scene => {
      scene.setSelectedHumanTrace(trace.id);
      assert.equal(scene.facilityLayer.selectedTraceId, trace.id);
      scene.setSelectedHumanTrace(null);
      assert.equal(scene.facilityLayer.selectedTraceId, null);
    });
  });

  test('P3 illustrated facility palette remains opt-in without new scene meshes', () => {
    const oldWindow = globalThis.window;
    try {
      delete globalThis.window;
      const standard = new FacilityLayer();
      const defaultColor = standard.woodMaterial.color.getHex();
      const defaultChildren = standard.group.children.length;
      standard.dispose();
      globalThis.window = { location: { search: '?world-style=illustrated' } };
      const illustrated = new FacilityLayer();
      try {
        assert.notEqual(illustrated.woodMaterial.color.getHex(), defaultColor);
        assert.equal(illustrated.woodMaterial.flatShading, true);
        assert.equal(illustrated.group.children.length, defaultChildren);
      } finally {
        illustrated.dispose();
      }
    } finally {
      if (oldWindow === undefined) delete globalThis.window;
      else globalThis.window = oldWindow;
    }
  });
}
