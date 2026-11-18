import assert from 'node:assert/strict';

export function residentWorkP3bChecks({ test, source, THREE }) {
  const { ResidentInventoryProps, ResidentPropResources } = source('render/resident-props.ts');
  const { ResidentProductionTargets } = source('render/resident-production-context.ts');
  const { WORLD_GRID_CONTRACT } = source('runtime/lifelens-contract.ts');
  const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const inventory = [{ item: 'RawMaterial', material: 'Wood', quantity: 3 }];
  const storage = { id: 'store-one', gridX: span / 2, gridY: span / 2 };
  const storeTask = {
    active: true, kind: 'Civilization', phase: 'Moving',
    civilizationIntent: 'Store', facilityAction: 'None',
    civilizationMaterial: 'Wood', civilizationStorage: storage.id,
    hasTargetGrid: true, targetGridX: storage.gridX, targetGridY: storage.gridY,
  };

  function visibleProps() {
    const visual = new THREE.Group();
    const model = new THREE.Group();
    for (const name of ['pelvis', 'hand_r', 'hand_l']) {
      const joint = new THREE.Group(); joint.name = name; model.add(joint);
    }
    visual.add(model);
    const resources = new ResidentPropResources();
    const props = new ResidentInventoryProps(visual, model, resources);
    return { visual, model, resources, props };
  }

  test('P3b only Core-confirmed storage target permits a moving cargo prop', () => {
    const targets = new ResidentProductionTargets();
    const { props, resources } = visibleProps();
    try {
      targets.setSnapshot({ available: true, storages: [storage] });
      assert.equal(targets.resolve(storeTask)?.id, storage.id);
      props.setTargetVerified(targets.resolve(storeTask) !== null);
      props.setInventory(inventory, storeTask);
      assert.equal(props.hasCarriedLoad, true);
      assert.equal(props.loadRoot.visible, true);
      const bogus = { ...storeTask, civilizationStorage: 'missing-node' };
      assert.equal(targets.resolve(bogus), null);
      props.setTargetVerified(targets.resolve(bogus) !== null);
      props.setInventory(inventory, bogus);
      assert.equal(props.hasCarriedLoad, false);
      assert.equal(props.loadRoot.visible, false, 'missing Core target produced phantom cargo');
      // Same inventory, now from a new authoritative snapshot, restores the
      // very same geometry without allocating a new fictional object.
      props.setTargetVerified(true);
      props.setInventory(inventory, storeTask);
      assert.equal(props.hasCarriedLoad, true);
      props.setInventory([], storeTask);
      assert.equal(props.hasCarriedLoad, false, 'consumed inventory still displayed as cargo');
    } finally { props.dispose(); resources.dispose(); }
  });

  test('P3b no phantom cargo for mismatched Core coordinates or unavailable snapshots', () => {
    const targets = new ResidentProductionTargets();
    const { props, resources } = visibleProps();
    try {
      targets.setSnapshot({ available: true, storages: [storage] });
      const incorrectGrid = { ...storeTask, targetGridX: storage.gridX + span * 2 };
      assert.equal(targets.resolve(incorrectGrid), null);
      props.setInventory(inventory, incorrectGrid);
      props.setTargetVerified(targets.resolve(incorrectGrid) !== null);
      assert.equal(props.loadRoot.visible, false);
      targets.setSnapshot({ available: false, storages: [storage] });
      assert.equal(targets.resolve(storeTask), null);
      props.setTargetVerified(false);
      props.setInventory(inventory, storeTask);
      assert.equal(props.hasCarriedLoad, false);
      targets.setSnapshot({ available: true, storages: [storage] });
      props.setTargetVerified(targets.resolve(storeTask) !== null);
      props.setInventory(inventory, storeTask);
      assert.equal(props.hasCarriedLoad, true);
      props.setInventory(undefined, storeTask);
      assert.equal(props.loadRoot.visible, false);
    } finally { props.dispose(); resources.dispose(); }
  });

  test('P3b visually interpolating residents keep actual cargo but never fake work', () => {
    const { props, resources } = visibleProps();
    try {
      const task = { ...storeTask, phase: 'Interacting' };
      props.setInventory(inventory, task);
      props.setTargetVerified(true);
      props.setInteractionAllowed(false);
      assert.equal(props.loadRoot.visible, false, 'body has not reached the Core site');
      props.setVisuallyMoving(true);
      assert.equal(props.loadRoot.visible, true, 'valid cargo vanished while body caught up');
      props.setTargetVerified(false);
      assert.equal(props.loadRoot.visible, false, 'invalid Core site still carries goods');
      props.setTargetVerified(true);
      props.setVisuallyMoving(false);
      props.setInteractionAllowed(true);
      assert.equal(props.hasCarriedLoad, true);
    } finally { props.dispose(); resources.dispose(); }
  });

  test('P3b illustrated close-up props keep the original geometry and owner-budget', () => {
    const oldWindow = globalThis.window;
    try {
      delete globalThis.window;
      const classic = new ResidentPropResources();
      const classicCount = classic.geometryCount;
      const classicPositions = Array.from(classic.get('Wood', 2).getAttribute('position').array);
      assert.equal(classic.material.flatShading, false);
      classic.dispose();
      globalThis.window = { location: { search: '?world-style=illustrated' } };
      const illustrated = new ResidentPropResources();
      try {
        assert.equal(illustrated.material.flatShading, true);
        assert.equal(illustrated.geometryCount, classicCount);
        assert.deepEqual(Array.from(illustrated.get('Wood', 2).getAttribute('position').array), classicPositions);
      } finally { illustrated.dispose(); }
    } finally {
      if (oldWindow === undefined) delete globalThis.window;
      else globalThis.window = oldWindow;
    }
  });
}
