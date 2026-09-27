import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, resolve } from 'node:path';
import ts from 'typescript';

const require = createRequire(import.meta.url);
const modules = new Map();

function load(path) {
  if (modules.has(path)) return modules.get(path).exports;
  const module = { exports: {} };
  modules.set(path, module);
  const code = ts.transpileModule(readFileSync(path, 'utf8'), {
    compilerOptions: {
      module: ts.ModuleKind.CommonJS,
      target: ts.ScriptTarget.ES2020,
    },
  }).outputText;
  new Function('require', 'module', 'exports', code)(
    id => id.startsWith('.')
      ? load(resolve(dirname(path), id + '.ts'))
      : require(id),
    module,
    module.exports,
  );
  return module.exports;
}

const {
  deriveObservationEvents,
  mergeObservationEvents,
} = load(resolve(
  import.meta.dirname,
  '../../src/state/observation-feed.ts',
));

const world = (minute, majorLifeEvents = 0, worldSeed = '42') => ({
  worldSeed,
  minute,
  majorLifeEvents,
});

const resident = (extra = {}) => ({
  id: 'a',
  name: '민재',
  activityKind: 'Idle',
  activityLabel: '쉬는 중',
  relationships: [],
  memories: [],
  family: {
    hasActivePartner: false,
    expectingChild: false,
    children: [],
  },
  ...extra,
});

let passed = 0;
function testCase(name, run) {
  run();
  passed += 1;
  console.log('PASS ' + name);
}

testCase('first frame and world switch never fabricate observations', () => {
  assert.deepEqual(
    deriveObservationEvents({}, [], world(10), [resident()]),
    [],
  );
  assert.deepEqual(
    deriveObservationEvents(
      world(10, 0, 'one'),
      [resident()],
      world(11, 0, 'two'),
      [resident({ activityKind: 'Social', activityLabel: '대화' })],
    ),
    [],
  );
});

testCase('real activity transition with a target becomes an observation', () => {
  const events = deriveObservationEvents(
    world(10),
    [resident()],
    world(11),
    [resident({
      activityKind: 'Social',
      activityLabel: '대화',
      activityTargetId: 'b',
      activityTargetName: '하린',
    })],
  );
  assert.equal(events.length, 1);
  assert.equal(events[0].kind, 'activity');
  assert.match(events[0].summary, /민재.*대화.*하린/);
});

testCase('important newly reported memory becomes an observation', () => {
  const events = deriveObservationEvents(
    world(10),
    [resident()],
    world(12),
    [resident({
      memories: [{
        minute: 12,
        what: '하린이 물을 찾는 모습을 봄',
        where: '강가',
        importance: 0.8,
        source: 'DirectWitness',
        emotionIntensity: 0.5,
      }],
    })],
  );
  assert.equal(events[0].kind, 'memory');
  assert.match(events[0].detail, /강가/);
});

testCase('relationship feed ignores noise and reports meaningful deltas', () => {
  const baseRelation = {
    targetId: 'b',
    targetName: '하린',
    trust: 0.4,
    socialBond: 0.3,
    conflict: 0.1,
    romancePotential: 0.1,
  };
  const quiet = deriveObservationEvents(
    world(10),
    [resident({ relationships: [baseRelation] })],
    world(11),
    [resident({ relationships: [{ ...baseRelation, trust: 0.47 }] })],
  );
  assert.equal(
    quiet.filter(event => event.kind === 'relationship').length,
    0,
  );

  const meaningful = deriveObservationEvents(
    world(10),
    [resident({ relationships: [baseRelation] })],
    world(12),
    [resident({ relationships: [{ ...baseRelation, trust: 0.61 }] })],
  );
  const relation = meaningful.find(event => event.kind === 'relationship');
  assert.ok(relation);
  assert.match(relation.summary, /신뢰/);
  assert.equal(relation.detail, '40% → 61%');
});

testCase('family and major life changes are high-priority factual cues', () => {
  const next = resident({
    family: {
      hasActivePartner: false,
      expectingChild: true,
      pregnancyPartnerName: '하린',
      children: [],
    },
  });
  const events = deriveObservationEvents(
    world(20, 2),
    [resident()],
    world(21, 3),
    [next],
  );
  assert.ok(events.some(event => event.kind === 'life'));
  assert.ok(events.some(event => event.kind === 'family'));
  assert.ok(events.every(event => event.importance === 'high'));
});

testCase('merge de-duplicates repeated snapshots and bounds history', () => {
  const incoming = Array.from({ length: 8 }, (_, index) => ({
    id: 'event-' + index,
    kind: 'activity',
    minute: index,
    summary: 'event ' + index,
    importance: 'low',
  }));
  const existing = [
    incoming[0],
    {
      id: 'old',
      kind: 'life',
      minute: 0,
      summary: 'old',
      importance: 'high',
    },
  ];
  const merged = mergeObservationEvents(incoming, existing, 5);
  assert.equal(merged.length, 5);
  assert.equal(new Set(merged.map(event => event.id)).size, 5);
  assert.equal(merged[0].id, 'event-0');
});

testCase('exact Core social event outranks inferred relationship/activity noise', () => {
  const previous = resident({
    activityKind: 'Idle',
    relationships: [{
      targetId: 'b',
      targetName: '하린',
      trust: 0.4,
      socialBond: 0.3,
      conflict: 0.1,
      romancePotential: 0.1,
    }],
  });
  const next = resident({
    activityKind: 'Social',
    activityLabel: '갈등',
    activityTargetId: 'b',
    activityTargetName: '하린',
    relationships: [{
      targetId: 'b',
      targetName: '하린',
      trust: 0.2,
      socialBond: 0.15,
      conflict: 0.42,
      romancePotential: 0.08,
    }],
  });
  const events = deriveObservationEvents(
    world(30),
    [previous, { ...resident({ id: 'b', name: '하린' }) }],
    world(31),
    [next, { ...resident({ id: 'b', name: '하린' }) }],
    {
      available: true,
      events: [{
        sequence: '4',
        actorId: 'a',
        targetId: 'b',
        type: 'PositiveInteraction',
        intensity: 0.3,
        importance: 0.2,
        minute: 29,
        where: '',
        presentationLevel: 'Everyday',
        successful: true,
      }],
    },
    {
      available: true,
      events: [{
        sequence: '4',
        actorId: 'a',
        targetId: 'b',
        type: 'PositiveInteraction',
        intensity: 0.3,
        importance: 0.2,
        minute: 29,
        where: '',
        presentationLevel: 'Everyday',
        successful: true,
      }, {
        sequence: '5',
        actorId: 'a',
        targetId: 'b',
        type: 'Conflict',
        intensity: 0.8,
        importance: 0.9,
        minute: 31,
        where: '강가',
        presentationLevel: 'Important',
        successful: true,
      }],
    },
  );
  assert.equal(events.filter(event => event.kind === 'social').length, 1);
  assert.equal(events.filter(event => event.kind === 'relationship').length, 0);
  assert.equal(events.filter(event => event.kind === 'activity').length, 0);
  const social = events.find(event => event.kind === 'social');
  assert.match(social.summary, /민재.*하린.*갈등/);
  assert.match(social.detail, /강가/);
  assert.equal(social.importance, 'high');
});

testCase('exact life history replaces generic major-life and family inference', () => {
  const previous = resident({
    lifeHistory: [{
      type: 'Birth',
      minute: 0,
      relatedCharacterIds: [],
      value: 0,
    }],
  });
  const next = resident({
    family: {
      hasActivePartner: true,
      partnerId: 'b',
      partnerName: '하린',
      expectingChild: false,
      children: [],
    },
    lifeHistory: [{
      type: 'Birth',
      minute: 0,
      relatedCharacterIds: [],
      value: 0,
    }, {
      type: 'DatingStarted',
      minute: 44,
      relatedCharacterIds: ['b'],
      value: 0,
    }],
  });
  const events = deriveObservationEvents(
    world(43, 1),
    [previous, { ...resident({ id: 'b', name: '하린', lifeHistory: [] }) }],
    world(44, 2),
    [next, { ...resident({ id: 'b', name: '하린', lifeHistory: [] }) }],
  );
  assert.equal(events.filter(event => event.kind === 'life').length, 1);
  assert.equal(events.filter(event => event.kind === 'family').length, 0);
  assert.match(events[0].summary, /민재.*연애/);
  assert.equal(events[0].targetResidentId, 'b');
});

testCase('first exact social payload does not replay old Core history', () => {
  const events = deriveObservationEvents(
    world(50),
    [resident()],
    world(51),
    [resident()],
    { available: false, events: [] },
    {
      available: true,
      events: [{
        sequence: '9',
        actorId: 'a',
        targetId: 'b',
        type: 'Comfort',
        intensity: 0.7,
        importance: 0.6,
        minute: 49,
        where: '',
        presentationLevel: 'Meaningful',
        successful: true,
      }],
    },
  );
  assert.equal(events.filter(event => event.kind === 'social').length, 0);
});

console.log(passed + ' observation-feed regression checks passed');
