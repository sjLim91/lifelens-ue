import assert from 'node:assert/strict';

export function worldPresentationChecks({ test, source, flatWindow }) {
  const { ObservedFootTraffic } = source('render/observed-foot-traffic.ts');
  const { WORLD_PRESENTATION: config } = source('render/world-presentation-config.ts');
  const resident = (x, y = 0) => ({ id: 'walker', alive: true, hasPosition: true, gridX: x, gridY: y });
  test('관측 보행은 반복 통행만 누적하고 정지·시간 역행·새 월드를 구분한다', () => {
    const history = new ObservedFootTraffic();
    history.observe([resident(0)], 'world', 0);
    history.observe([resident(1)], 'world', 1);
    const first = history.marks.get('1:0').strength;
    history.observe([resident(1)], 'world', 1);
    assert.equal(history.marks.get('1:0').strength, first);
    history.observe([resident(0)], 'world', 2);
    history.observe([resident(1)], 'world', 3);
    assert.ok(history.marks.get('1:0').strength > first);
    history.observe([resident(1)], 'world', 2);
    assert.equal(history.marks.size, 0);
    history.observe([resident(2)], 'other', 3);
    assert.equal(history.marks.size, 0);
  });
  test('보행 흔적은 관측 공백을 연결하지 않고 시뮬레이션 시간으로 사라진다', () => {
    const history = new ObservedFootTraffic();
    history.observe([resident(0)], 'world', 0);
    history.observe([resident(1)], 'world', 1);
    const mark = history.marks.get('1:0');
    assert.ok(history.opacity(mark, 1 + config.paths.fadeMinutes / 2) < mark.strength);
    history.observe([resident(30)], 'world', 2);
    assert.equal(history.marks.size, 1);
    history.observe([resident(31)], 'world', 20);
    assert.equal(history.marks.size, 1);
    history.observe([], 'world', 1 + config.paths.fadeMinutes);
    assert.equal(history.marks.size, 0);
    for (let i = 0; i < 1200; i++) history.observe([resident(i)], 'world', 2000 + i);
    assert.ok(history.marks.size <= config.paths.maxMarks);
  });
  const { FacilityLayer } = source('render/facility-layer.ts');
  const trace = {
    id: 'facility:42', kind: 'Facility', gridX: 0, gridY: 0,
    facilityKind: 'PrimitiveStorage', state: 'Operational', progress01: 1,
    deliveredMaterialUnits: 10, requiredMaterialUnits: 10, active: true, lit: false,
  };
  const terrain = () => ({ ...flatWindow(), humanTraces: { total: 1, entries: [trace] } });
  test('시설은 실제 저장량·내구도를 읽고 수리 시 원형을 회복한다', () => {
    const layer = new FacilityLayer();
    const window = terrain();
    const detail = { id: '42', kind: 'PrimitiveStorage', gridX: 0, gridY: 0, durability: 1, linkedStorage: 'stock' };
    const data = { available: true, facilities: [detail], storages: [{ id: 'stock', totalUnits: 12 }] };
    try {
      layer.setCivilization(data, window);
      const healthy = layer.group.children[0];
      assert.equal(healthy.children.filter(child => /-95[0-9]$/.test(child.name)).length, 3);
      layer.setCivilization(data, window);
      assert.equal(layer.group.children[0], healthy);
      const healthyColor = healthy.children[0].material.color.getHex();
      layer.setCivilization({ ...data, facilities: [{ ...detail, durability: 0.25 }] }, window);
      assert.notEqual(layer.group.children[0].children[0].material.color.getHex(), healthyColor);
      layer.setCivilization({ ...data, storages: [{ id: 'stock', totalUnits: 0 }] }, window);
      assert.equal(layer.group.children[0].children[0].material.color.getHex(), healthyColor);
      assert.equal(layer.group.children[0].children.filter(child => /-95[0-9]$/.test(child.name)).length, 0);
      layer.setCivilization({ available: false }, window);
      assert.equal(layer.group.children.length, 1);
    } finally { layer.dispose(); }
  });
  test('불빛은 점화된 시설에만 생기고 실시간 광원을 추가하지 않는다', () => {
    const layer = new FacilityLayer();
    try {
      const window = terrain();
      window.humanTraces.entries = [{ ...trace, facilityKind: 'FirePit', lit: true }];
      layer.setTerrain(window);
      let lights = 0, sprites = 0;
      layer.group.traverse(object => { if (object.isLight) lights++; if (object.isSprite) sprites++; });
      assert.equal(lights, 0); assert.equal(sprites, 1);
      window.humanTraces.entries = [{ ...trace, facilityKind: 'FirePit', lit: false }];
      layer.setTerrain(window);
      assert.equal(layer.group.children[0].children.some(child => child.isSprite), false);
    } finally { layer.dispose(); }
  });
}
