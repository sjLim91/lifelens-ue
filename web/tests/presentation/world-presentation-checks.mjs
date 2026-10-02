import assert from 'node:assert/strict';

export function worldPresentationChecks({ test, source, flatWindow }) {
  const { ObservedFootTraffic } = source('render/observed-foot-traffic.ts');
  const { WORLD_PRESENTATION: config } = source('render/world-presentation-config.ts');
  const { SimulationClock } = source('runtime/simulation-clock.ts');
  const { SIMULATION_TIME_CONTRACT: time, REAL_MS_PER_SIMULATION_MINUTE_AT_1X: msPerMinute } = source('runtime/lifelens-contract.ts');
  const resident = (x, y = 0) => ({ id: 'walker', alive: true, hasPosition: true, gridX: x, gridY: y });
  function actualClock(speed, check) {
    const previousWindow = globalThis.window, previousDocument = globalThis.document, previousNow = Date.now;
    let now = 0, id = 0, minute = 0, x = 0;
    const timers = new Map();
    const history = new ObservedFootTraffic(); history.setSpeed(speed);
    history.observe([resident(x)], 'clock-world', minute, now);
    const snapshots = [];
    Date.now = () => now;
    globalThis.window = {
      setInterval(callback, interval) { const key = ++id; timers.set(key, { callback, interval, due: now + interval }); return key; },
      clearInterval(key) { timers.delete(key); },
    };
    globalThis.document = { addEventListener() {}, removeEventListener() {} };
    const clock = new SimulationClock({ initialSpeed: speed,
      onAdvance(minutes) { minute += minutes; x += minutes; },
      onRefresh() { snapshots.push(minute); history.observe([resident(x)], 'clock-world', minute, now); },
    });
    const run = duration => {
      const end = now + duration;
      while (true) {
        const next = [...timers.values()].sort((a, b) => a.due - b.due)[0];
        if (!next || next.due > end) break;
        now = next.due; next.due += next.interval; next.callback();
      }
      now = end;
    };
    try { clock.start(); check({ clock, history, snapshots, run,
      jump(minutes, dx) { history.breakContinuity(); minute += minutes; x += dx; clock.resetAccumulator(); },
    }); } finally { clock.stop(); Date.now = previousNow; globalThis.window = previousWindow; globalThis.document = previousDocument; }
  }
  for (const speed of [1, 4]) test(`실제 SimulationClock ${speed}× 500ms 관측은 이동 끝점만 기록한다`, () => actualClock(speed, ({ history, snapshots, run }) => {
    run(time.refreshIntervalMs * 4);
    const expected = Math.floor(time.refreshIntervalMs / msPerMinute * speed);
    assert.ok(expected > 2, '회귀는 기존 maxGapMinutes=2가 실패하는 실제 계약을 사용해야 함');
    assert.equal(snapshots[0], expected);
    assert.equal(history.marks.size, snapshots.length);
    assert.deepEqual([...history.marks.values()].map(mark => mark.x), snapshots);
    assert.equal(history.marks.has('1:0'), false, '관측하지 않은 중간 경로를 만들면 안 됨');
  }));
  test('실제 clock 일시정지·동일 snapshot은 보행을 누적하지 않는다', () => actualClock(1, ({ clock, history, run }) => {
    run(time.refreshIntervalMs * 2);
    const before = [...history.marks.entries()];
    clock.setSpeed(0); history.setSpeed(0);
    run(time.refreshIntervalMs * 4);
    assert.deepEqual([...history.marks.entries()], before);
    clock.setSpeed(4); history.setSpeed(4);
    run(time.refreshIntervalMs * 2);
    assert.ok(history.marks.size > before.length);
  }));
  test('실제 clock 시간 점프 이후에는 가까운 이동도 이전 표본과 연결하지 않는다', () => actualClock(4, ({ history, run, jump }) => {
    run(time.refreshIntervalMs);
    jump(time.simulationMinutesPerDay, 1);
    run(time.refreshIntervalMs);
    assert.equal(history.marks.size, 0, '기존 자국은 하루 감쇠, 점프 도착점은 누적 금지');
    run(time.refreshIntervalMs);
    assert.equal(history.marks.size, 1, '정상 관측 재개 후 이동은 다시 기록');
  }));
  test('cadence·Core cardinal 이동량을 벗어난 teleport와 관측 공백은 거부한다', () => {
    const history = new ObservedFootTraffic(); history.setSpeed(4);
    const delta = Math.floor(time.refreshIntervalMs / msPerMinute * 4);
    history.observe([resident(0)], 'w', 0, 0);
    history.observe([resident(delta + 1)], 'w', delta, time.refreshIntervalMs);
    assert.equal(history.marks.size, 0);
    history.observe([resident(delta + 2)], 'w', delta * 2, time.refreshIntervalMs * 4);
    assert.equal(history.marks.size, 0);
    history.observe([resident(delta + 3)], 'w', delta * 3, time.refreshIntervalMs * 5);
    assert.equal(history.marks.size, 1);
    history.observe([resident(delta + 4)], 'w', delta * 3 + time.simulationMinutesPerDay, time.refreshIntervalMs * 6);
    assert.equal(history.marks.size, 0);
  });
  test('관측 보행은 반복 통행만 누적하고 정지·시간 역행·새 월드를 구분한다', () => {
    const history = new ObservedFootTraffic();
    history.observe([resident(0)], 'world', 0, 0);
    history.observe([resident(1)], 'world', 1, msPerMinute);
    const first = history.marks.get('1:0').strength;
    history.observe([resident(1)], 'world', 1);
    assert.equal(history.marks.get('1:0').strength, first);
    history.observe([resident(0)], 'world', 2, msPerMinute * 2);
    history.observe([resident(1)], 'world', 3, msPerMinute * 3);
    assert.ok(history.marks.get('1:0').strength > first);
    history.observe([resident(1)], 'world', 2);
    assert.equal(history.marks.size, 0);
    history.observe([resident(2)], 'other', 3);
    assert.equal(history.marks.size, 0);
  });
  test('보행 흔적은 관측 공백을 연결하지 않고 시뮬레이션 시간으로 사라진다', () => {
    const history = new ObservedFootTraffic();
    history.observe([resident(0)], 'world', 0, 0);
    history.observe([resident(1)], 'world', 1, msPerMinute);
    const mark = history.marks.get('1:0');
    assert.ok(history.opacity(mark, 1 + config.paths.fadeMinutes / 2) < mark.strength);
    history.observe([resident(30)], 'world', 2, msPerMinute * 2);
    assert.equal(history.marks.size, 1);
    history.observe([resident(31)], 'world', 20, msPerMinute * 20);
    assert.equal(history.marks.size, 1);
    history.observe([], 'world', 1 + config.paths.fadeMinutes);
    assert.equal(history.marks.size, 0);
    for (let i = 0; i < 1200; i++) history.observe([resident(i)], 'world', 2000 + i, (2000 + i) * msPerMinute);
    assert.ok(history.marks.size <= config.paths.maxMarks);
  });
  const { FacilityLayer } = source('render/facility-layer.ts');
  const { storedGoodsPiles } = source('render/stored-goods-presentation.ts');
  const { representativeSettlementForTrace } = source('state/settlement-observation.ts');
  const { observedSettlementEvents } = source('state/observation-feed.ts');
  test('적재물은 실제 재고 종류·수량·소진을 따르고 묶음 예산을 넘지 않는다', () => {
    const stock = { totalUnits: 12, inventory: [{ material: 'Wood', quantity: 6 }, { material: 'PlantFood', quantity: 6 }] };
    const piles = storedGoodsPiles(stock);
    assert.equal(piles.reduce((sum, pile) => sum + pile.quantity, 0), stock.totalUnits);
    assert.ok(piles.some(pile => pile.material === 'Wood'));
    assert.ok(piles.some(pile => pile.material === 'PlantFood'));
    assert.equal(piles[1].material, undefined, '혼합 묶음에 한 종류만 있다고 추정하지 않음');
    assert.deepEqual(storedGoodsPiles({ ...stock, totalUnits: 0 }), []);
    assert.equal(storedGoodsPiles({ totalUnits: 1 })[0].fill, 1 / config.storage.unitsPerPile);
    assert.equal(storedGoodsPiles({ totalUnits: 1000 }).length, config.storage.maxPiles);
  });
  test('대표시설 연결은 Core 최소 노드 ID를 무손실로 사용하고 인접 시설을 추정하지 않는다', () => {
    const id = '9007199254740993', key = (BigInt(id) << 1n).toString();
    const facility = { id, kind: 'Shelter', gridX: 4, gridY: 5, state: 'Operational', linkedStorage: '0' };
    const data = { available: true, facilities: [facility], settlements: [{ id: key, gridX: 10, gridY: 11 }] };
    const selected = { kind: 'Facility', id: `facility:${id}`, facilityKind: 'Shelter', gridX: 4, gridY: 5 };
    assert.equal(representativeSettlementForTrace(selected, data).id, key);
    assert.equal(representativeSettlementForTrace(selected, { ...data, settlements: [{ id: '2', gridX: 4, gridY: 5 }] }), undefined);
    assert.equal(representativeSettlementForTrace(selected, { ...data, available: false }), undefined);
    assert.equal(representativeSettlementForTrace(selected, { ...data, facilities: [{ ...facility, state: 'Ruined' }] }), undefined);
  });
  test('정착지 형성·교역 활성 로그는 전환에서만 발생하고 초기 로드·캐시·누락 DTO는 침묵한다', () => {
    const before = { available: true, minute: 0, settlements: [], tradeRoutes: [] };
    const settlement = { id: '2', gridX: 0, gridY: 0, residentCount: 4, facilityCount: 1, storageSiteCount: 1, established: true };
    const next = { ...before, minute: 24, settlements: [settlement], tradeRoutes: [{ id: 'route', firstSettlement: '2', secondSettlement: '4', firstGridX: 0, firstGridY: 0, exchangeCount: 5, partnerCount: 2, active: true }] };
    assert.equal(observedSettlementEvents(before, next).length, 2);
    assert.equal(observedSettlementEvents(next, { ...next, minute: 48 }).length, 0);
    assert.equal(observedSettlementEvents(next, next).length, 0);
    assert.equal(observedSettlementEvents({ available: false }, next).length, 0);
    assert.equal(observedSettlementEvents({ ...before, settlements: undefined, tradeRoutes: undefined }, next).length, 0);
    assert.equal(observedSettlementEvents(next, { ...next, minute: 48, settlements: [{ ...settlement, lifecycle: 'Abandoned', dominantKind: 'Food' }], relations: [{ state: 'Hostile' }] }).length, 0);
  });
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
  test('binding 없는 전문화·lifecycle은 시설·재고·폐허를 생성하거나 변경하지 않는다', () => {
    const layer = new FacilityLayer();
    try {
      const window = terrain();
      const data = { available: true, facilities: [{ id: '42', kind: 'PrimitiveStorage', gridX: 0, gridY: 0, durability: 1, linkedStorage: 'stock' }], storages: [{ id: 'stock', totalUnits: 4, inventory: [{ material: 'Wood', quantity: 4 }] }] };
      layer.setCivilization(data, window);
      const actual = layer.group.children[0];
      layer.setCivilization({ ...data, settlements: [{ id: '84', lifecycle: 'Abandoned', dominantKind: 'Metallurgy', specialization01: 1 }] }, window);
      assert.equal(layer.group.children[0], actual);
      assert.equal(actual.children.filter(child => child.userData.storedGoods).length, 1);
      const empty = { ...window, humanTraces: { total: 0, entries: [] } };
      layer.setCivilization({ available: true, settlements: [{ id: '84', dominantKind: 'Food', specialization01: 1 }] }, empty);
      assert.equal(layer.group.children.length, 0);
    } finally { layer.dispose(); }
  });
  test('같은 총량에서 재고 종류 변경은 cache를 갱신하고 실제 폐허 상태가 권위다', () => {
    const layer = new FacilityLayer();
    try {
      const window = terrain();
      const facility = { id: '42', kind: 'PrimitiveStorage', gridX: 0, gridY: 0, durability: 1, linkedStorage: 'stock' };
      const data = { available: true, facilities: [facility], storages: [{ id: 'stock', totalUnits: 4, inventory: [{ material: 'Wood', quantity: 4 }] }] };
      layer.setCivilization(data, window);
      const old = layer.group.children[0];
      layer.setCivilization({ ...data, storages: [{ id: 'stock', totalUnits: 4, inventory: [{ material: 'Clay', quantity: 4 }] }] }, window);
      assert.notEqual(layer.group.children[0], old);
      assert.equal(layer.group.children[0].children.find(child => child.userData.storedGoods).userData.storedGoods.material, 'Clay');
      window.humanTraces.entries = [{ ...trace, state: 'Ruined' }];
      layer.setCivilization(data, window);
      assert.equal(layer.group.children[0].children.some(child => child.userData.storedGoods), false);
    } finally { layer.dispose(); }
  });
  test('정착지 중심 강조는 선택 시 한 개만 표시하고 geometry·material을 재사용한다', () => {
    const { SettlementFocusLayer } = source('render/settlement-focus-layer.ts');
    const layer = new SettlementFocusLayer();
    try {
      const window = flatWindow();
      const data = { available: true, settlements: [{ id: '2', gridX: 0, gridY: 0 }, { id: '4', gridX: 8, gridY: 8 }] };
      layer.setTargets(data, window);
      const mesh = layer.group.children[0], geometry = mesh.geometry, material = mesh.material;
      assert.equal(mesh.visible, false);
      layer.select('2'); assert.equal(mesh.visible, true);
      layer.select('4'); assert.equal(layer.group.children.length, 1);
      assert.equal(mesh.geometry, geometry); assert.equal(mesh.material, material);
      layer.select(null); assert.equal(mesh.visible, false);
      layer.select('2'); layer.setTargets({ available: false }, window); assert.equal(mesh.visible, false);
    } finally { layer.dispose(); }
  });
}
