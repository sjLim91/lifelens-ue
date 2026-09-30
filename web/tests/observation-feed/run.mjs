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

testCase('need-driven physical milestones expose cause, travel and interaction', () => {
  const moving = deriveObservationEvents(
    world(20),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'Drink',
      physicalGoal: 'Drink',
      needs: { thirst: 0.88 },
      presentation: {
        active: false,
      },
    })],
    world(21),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'Drink',
      physicalGoal: 'Drink',
      needs: { thirst: 0.89 },
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Moving',
        physicalGoal: 'Drink',
        directNaturalWaterSource: true,
        hasTargetGrid: true,
        targetGridX: 12,
        targetGridY: 8,
      },
    })],
  );

  assert.equal(moving.length, 1);
  assert.match(moving[0].summary, /민재.*갈증.*물가.*이동/);
  assert.match(moving[0].detail, /갈증 89%/);
  assert.match(moving[0].detail, /자연수 직접 사용/);
  assert.equal(moving[0].importance, 'high');

  const interacting = deriveObservationEvents(
    world(21),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'Drink',
      physicalGoal: 'Drink',
      needs: { thirst: 0.89 },
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Moving',
        physicalGoal: 'Drink',
        directNaturalWaterSource: true,
        hasTargetGrid: true,
        targetGridX: 12,
        targetGridY: 8,
      },
    })],
    world(22),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'Drink',
      physicalGoal: 'Drink',
      needs: { thirst: 0.89 },
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Interacting',
        physicalGoal: 'Drink',
        directNaturalWaterSource: true,
        hasTargetGrid: true,
        targetGridX: 12,
        targetGridY: 8,
      },
    })],
  );

  assert.equal(interacting.length, 1);
  assert.match(interacting[0].summary, /물가에 도착.*마시기 시작/);
});

testCase('sleep and sanitation milestones stay factual to authoritative target context', () => {
  const sleep = deriveObservationEvents(
    world(30),
    [resident({ presentation: { active: false } })],
    world(31),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'Sleep',
      physicalGoal: 'Sleep',
      needs: { sleep: 0.78 },
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Moving',
        physicalGoal: 'Sleep',
        hasTargetGrid: true,
        targetGridX: 4,
        targetGridY: 5,
      },
    })],
  );
  assert.ok(sleep.some(event => /잠자리로 이동/.test(event.summary)));
  assert.ok(sleep.some(event => /피로 78%/.test(event.detail ?? '')));

  const toilet = deriveObservationEvents(
    world(40),
    [resident({ presentation: { active: false } })],
    world(41),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'UseToilet',
      physicalGoal: 'UseToilet',
      needs: { bladder: 0.71 },
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Interacting',
        physicalGoal: 'UseToilet',
        designatedSanitationSite: true,
        sanitationSiteId: '7',
      },
    })],
  );
  assert.ok(toilet.some(event => /위생 장소 이용/.test(event.summary)));
  assert.ok(toilet.some(event => /배뇨 욕구 71%/.test(event.detail ?? '')));
});

testCase('unchanged physical phase does not spam the observation feed', () => {
  const presentation = {
    active: true,
    kind: 'Physical',
    phase: 'Interacting',
    physicalGoal: 'Wash',
    directNaturalWaterSource: false,
  };
  const events = deriveObservationEvents(
    world(50),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'Wash',
      physicalGoal: 'Wash',
      needs: { hygiene: 0.7 },
      presentation,
    })],
    world(51),
    [resident({
      activityKind: 'Physical',
      activityLabel: 'Wash',
      physicalGoal: 'Wash',
      needs: { hygiene: 0.68 },
      presentation: { ...presentation },
    })],
  );
  assert.equal(events.length, 0);
});

testCase('social action milestones expose approach and interaction phases', () => {
  const target = resident({ id: 'b', name: '하린' });
  const moving = deriveObservationEvents(
    world(20),
    [resident({ presentation: { active: false } }), target],
    world(21),
    [resident({
      activityKind: 'Social',
      activityLabel: 'Comfort',
      socialIntent: 'Comfort',
      activityTargetId: 'b',
      activityTargetName: '하린',
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Moving',
        socialIntent: 'Comfort',
        targetResidentId: 'b',
        contextActionToken: '501',
      },
    }), target],
  );

  assert.equal(moving.length, 1);
  assert.match(moving[0].summary, /민재.*하린.*위로하러 이동/);
  assert.equal(moving[0].targetResidentId, 'b');

  const interacting = deriveObservationEvents(
    world(21),
    [resident({
      activityKind: 'Social',
      activityLabel: 'Comfort',
      socialIntent: 'Comfort',
      activityTargetId: 'b',
      activityTargetName: '하린',
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Moving',
        socialIntent: 'Comfort',
        targetResidentId: 'b',
        contextActionToken: '501',
      },
    }), target],
    world(22),
    [resident({
      activityKind: 'Social',
      activityLabel: 'Comfort',
      socialIntent: 'Comfort',
      activityTargetId: 'b',
      activityTargetName: '하린',
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Comfort',
        targetResidentId: 'b',
        contextActionToken: '501',
      },
    }), target],
  );

  assert.equal(interacting.length, 1);
  assert.match(interacting[0].summary, /민재.*하린.*위로하는 중/);
});

testCase('teaching and parenting milestones name the real target and action', () => {
  const target = resident({ id: 'b', name: '하린' });
  const teaching = deriveObservationEvents(
    world(30),
    [resident({ presentation: { active: false } }), target],
    world(31),
    [resident({
      presentation: {
        active: true,
        kind: 'KnowledgeTeaching',
        phase: 'Interacting',
        targetResidentId: 'b',
        knowledgeTeachingTechnique: 'FireMaking',
        contextActionToken: '601',
      },
    }), target],
  );
  assert.ok(teaching.some(event => /하린.*불 피우기.*가르치는 중/.test(event.summary)));

  const parenting = deriveObservationEvents(
    world(40),
    [resident({ presentation: { active: false } }), target],
    world(41),
    [resident({
      presentation: {
        active: true,
        kind: 'Parenting',
        phase: 'Interacting',
        targetResidentId: 'b',
        parentingAction: 'Comfort',
        contextActionToken: '602',
      },
    }), target],
  );
  assert.ok(parenting.some(event => /하린.*달래기.*진행 중/.test(event.summary)));
});

testCase('unchanged social phase does not spam and exact social outcome wins', () => {
  const target = resident({ id: 'b', name: '하린' });
  const presentation = {
    active: true,
    kind: 'Social',
    phase: 'Interacting',
    socialIntent: 'Repair',
    targetResidentId: 'b',
    contextActionToken: '701',
  };
  const unchanged = deriveObservationEvents(
    world(50),
    [resident({
      activityKind: 'Social',
      activityLabel: 'Repair',
      presentation,
    }), target],
    world(51),
    [resident({
      activityKind: 'Social',
      activityLabel: 'Repair',
      presentation: { ...presentation },
    }), target],
  );
  assert.equal(unchanged.length, 0);

  const exact = deriveObservationEvents(
    world(51),
    [resident({ presentation: { active: false } }), target],
    world(52),
    [resident({
      activityKind: 'Social',
      activityLabel: 'Comfort',
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Comfort',
        targetResidentId: 'b',
        contextActionToken: '702',
      },
    }), target],
    { available: true, events: [] },
    {
      available: true,
      events: [{
        sequence: '100',
        actorId: 'a',
        targetId: 'b',
        type: 'Comfort',
        intensity: 0.8,
        importance: 0.7,
        minute: 52,
        where: '',
        presentationLevel: 'Meaningful',
        successful: true,
      }],
    },
  );
  assert.equal(exact.filter(event => event.kind === 'social').length, 1);
  assert.equal(exact.filter(event => event.id.startsWith('social-action:')).length, 0);
});

testCase('authoritative resource exploration start becomes an observation', () => {
  const events = deriveObservationEvents(
    world(11),
    [resident({
      presentation: {
        active: false,
      },
    })],
    world(12),
    [resident({
      presentation: {
        active: true,
        kind: 'Civilization',
        phase: 'Moving',
        civilizationIntent: 'Explore',
        civilizationMaterial: 'Water',
        issuedMinute: 12,
        contextActionToken: '91',
        hasTargetGrid: true,
        targetGridX: 32,
        targetGridY: -16,
      },
    })],
  );

  const exploration = events.find(event => event.id === 'civilization:explore:a:91');
  assert.ok(exploration);
  assert.equal(exploration.kind, 'civilization');
  assert.equal(exploration.summary, '민재: 물 자원 탐색 시작');
  assert.equal(exploration.detail, '탐색 목표 좌표 32, -16');
  assert.equal(exploration.importance, 'medium');
});

testCase('unchanged exploration context does not spam the observation feed', () => {
  const presentation = {
    active: true,
    kind: 'Civilization',
    phase: 'Moving',
    civilizationIntent: 'Explore',
    civilizationMaterial: 'Wood',
    issuedMinute: 14,
    contextActionToken: '92',
    hasTargetGrid: true,
    targetGridX: 64,
    targetGridY: 0,
  };
  const events = deriveObservationEvents(
    world(14),
    [resident({ presentation })],
    world(15),
    [resident({ presentation: { ...presentation } })],
  );
  assert.equal(events.filter(event => event.id.startsWith('civilization:explore:')).length, 0);
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

testCase('exact social outcome shows the Core directional relationship delta', () => {
  const actorBefore = resident({
    id: 'a',
    name: '민재',
    relationships: [{
      targetId: 'b',
      targetName: '하린',
      trust: 0.2,
      affection: 0.25,
      comfort: 0.2,
      socialBond: 0.18,
    }],
  });
  const recipientBefore = resident({
    id: 'b',
    name: '하린',
    relationships: [{
      targetId: 'a',
      targetName: '민재',
      trust: 0.4,
      affection: 0.3,
      comfort: 0.25,
      socialBond: 0.31,
      conflict: 0.05,
    }],
  });
  const actorAfter = structuredClone(actorBefore);
  const recipientAfter = resident({
    id: 'b',
    name: '하린',
    relationships: [{
      targetId: 'a',
      targetName: '민재',
      trust: 0.45,
      affection: 0.36,
      comfort: 0.32,
      socialBond: 0.38,
      conflict: 0.05,
    }],
  });

  const events = deriveObservationEvents(
    world(60),
    [actorBefore, recipientBefore],
    world(61),
    [actorAfter, recipientAfter],
    { available: true, events: [] },
    {
      available: true,
      events: [{
        sequence: '200',
        actorId: 'a',
        targetId: 'b',
        type: 'Comfort',
        intensity: 0.8,
        importance: 0.7,
        minute: 61,
        where: '잠자리 옆',
        presentationLevel: 'Meaningful',
        successful: true,
      }],
    },
  );

  const social = events.find(event => event.id === 'social:sequence:200');
  assert.ok(social);
  assert.match(social.detail, /잠자리 옆/);
  assert.match(social.detail, /하린→민재/);
  assert.match(social.detail, /유대 \+7\.0%p/);
  assert.match(social.detail, /편안함 \+7\.0%p/);
  assert.match(social.detail, /애정 \+6\.0%p/);
  assert.doesNotMatch(social.detail, /민재→하린/);
});

testCase('multiple events for one directional pair do not invent per-event deltas', () => {
  const actor = resident({ id: 'a', name: '민재' });
  const beforeTarget = resident({
    id: 'b',
    name: '하린',
    relationships: [{
      targetId: 'a',
      targetName: '민재',
      trust: 0.4,
      affection: 0.3,
      comfort: 0.3,
      socialBond: 0.3,
    }],
  });
  const afterTarget = resident({
    id: 'b',
    name: '하린',
    relationships: [{
      targetId: 'a',
      targetName: '민재',
      trust: 0.5,
      affection: 0.4,
      comfort: 0.4,
      socialBond: 0.42,
    }],
  });
  const events = deriveObservationEvents(
    world(70),
    [actor, beforeTarget],
    world(72),
    [structuredClone(actor), afterTarget],
    { available: true, events: [] },
    {
      available: true,
      events: [{
        sequence: '201',
        actorId: 'a',
        targetId: 'b',
        type: 'Help',
        intensity: 0.7,
        importance: 0.6,
        minute: 71,
        where: '',
        presentationLevel: 'Meaningful',
        successful: true,
      }, {
        sequence: '202',
        actorId: 'a',
        targetId: 'b',
        type: 'Comfort',
        intensity: 0.7,
        importance: 0.6,
        minute: 72,
        where: '',
        presentationLevel: 'Meaningful',
        successful: true,
      }],
    },
  );

  const social = events.filter(event => event.kind === 'social');
  assert.equal(social.length, 2);
  assert.ok(social.every(event => !/하린→민재/.test(event.detail ?? '')));
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
    [previous, resident({
      id: 'b',
      name: '하린',
      relationships: [{
        targetId: 'a',
        targetName: '민재',
        trust: 0.4,
        socialBond: 0.3,
        conflict: 0.1,
        romancePotential: 0.1,
      }],
    })],
    world(31),
    [next, resident({
      id: 'b',
      name: '하린',
      relationships: [{
        targetId: 'a',
        targetName: '민재',
        trust: 0.37,
        socialBond: 0.24,
        conflict: 0.18,
        romancePotential: 0.08,
      }],
    })],
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
  assert.match(social.detail, /하린→민재/);
  assert.match(social.detail, /갈등 \+8\.0%p/);
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

testCase('exact civilization changes become observation events', () => {
  const previousCivilization = {
    available: true,
    minute: 70,
    recentDiscoveries: [{
      factId: 'known',
      technique: 'FireMaking',
      discovererId: 'a',
      discovererName: '민재',
      minute: 65,
      livingKnowerCount: 1,
    }],
    facilities: [{
      id: 'shelter-1',
      kind: 'Shelter',
      state: 'UnderConstruction',
      initiatedBy: 'a',
      lastWorkedBy: 'a',
      startedMinute: 60,
    }],
    resources: [{
      id: 'wood-1',
      material: 'Wood',
      quantity: 3,
      gridX: 4,
      gridY: 8,
    }],
  };
  const nextCivilization = {
    available: true,
    minute: 71,
    recentDiscoveries: [
      ...previousCivilization.recentDiscoveries,
      {
        factId: 'new-tech',
        technique: 'SimpleContainer',
        discovererId: 'a',
        discovererName: '민재',
        minute: 71,
        livingKnowerCount: 2,
      },
    ],
    facilities: [{
      ...previousCivilization.facilities[0],
      state: 'Operational',
      completedMinute: 71,
    }],
    resources: [{
      ...previousCivilization.resources[0],
      quantity: 0,
    }],
  };

  const events = deriveObservationEvents(
    world(70),
    [resident()],
    world(71),
    [resident()],
    undefined,
    undefined,
    previousCivilization,
    nextCivilization,
  );

  assert.equal(events.filter(event => event.kind === 'civilization').length, 2);
  assert.equal(events.filter(event => event.kind === 'facility').length, 1);
  assert.ok(events.some(event => /단순 용기 만들기/.test(event.summary)));
  assert.ok(events.some(event => /완성/.test(event.summary)));
  assert.ok(events.some(event => /고갈/.test(event.summary)));
});

testCase('exact sanitation changes become observation events', () => {
  const previousObjects = {
    available: true,
    smartObjects: [],
    sanitationSites: [{
      id: 'san-1',
      kind: 'DesignatedArea',
      gridX: 2,
      gridY: 3,
      establishedBy: 'a',
      establishedMinute: 50,
      active: true,
      useCount: 2,
      improvementWork: 0,
      improvedBy: '',
      improvedMinute: 0,
    }],
  };
  const nextObjects = {
    available: true,
    smartObjects: [],
    sanitationSites: [{
      ...previousObjects.sanitationSites[0],
      useCount: 5,
      improvementWork: 1,
      improvedBy: 'a',
      improvedMinute: 72,
    }, {
      id: 'san-2',
      kind: 'DugPit',
      gridX: 6,
      gridY: 7,
      establishedBy: 'a',
      establishedMinute: 72,
      active: true,
      useCount: 0,
      improvementWork: 0,
      improvedBy: '',
      improvedMinute: 0,
    }],
  };

  const events = deriveObservationEvents(
    world(71),
    [resident()],
    world(72),
    [resident()],
    undefined,
    undefined,
    undefined,
    undefined,
    previousObjects,
    nextObjects,
  );

  assert.equal(events.filter(event => event.kind === 'sanitation').length, 2);
  assert.ok(events.some(event => /개선/.test(event.summary)));
  assert.ok(events.some(event => /구덩이/.test(event.summary)));
});

testCase('first heavy authority payload does not replay old world activity', () => {
  const events = deriveObservationEvents(
    world(80),
    [resident()],
    world(81),
    [resident()],
    undefined,
    undefined,
    { available: false, recentDiscoveries: [], facilities: [], resources: [] },
    {
      available: true,
      minute: 81,
      recentDiscoveries: [{
        factId: 'old-tech',
        technique: 'FireMaking',
        discovererId: 'a',
        discovererName: '민재',
        minute: 40,
        livingKnowerCount: 4,
      }],
      facilities: [{
        id: 'old-shelter',
        kind: 'Shelter',
        state: 'Operational',
        initiatedBy: 'a',
        lastWorkedBy: 'a',
        startedMinute: 20,
      }],
      resources: [],
    },
    { available: false, smartObjects: [], sanitationSites: [] },
    {
      available: true,
      smartObjects: [],
      sanitationSites: [{
        id: 'old-san',
        kind: 'DugPit',
        gridX: 1,
        gridY: 1,
        establishedBy: 'a',
        establishedMinute: 30,
        active: true,
        useCount: 3,
        improvementWork: 1,
        improvedBy: 'a',
        improvedMinute: 35,
      }],
    },
  );

  assert.equal(events.filter(event => (
    event.kind === 'civilization'
    || event.kind === 'facility'
    || event.kind === 'sanitation'
  )).length, 0);
});

console.log(passed + ' observation-feed regression checks passed');
