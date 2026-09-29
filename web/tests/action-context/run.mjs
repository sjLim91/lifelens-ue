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

const { residentActionCue } = load(resolve(
  import.meta.dirname,
  '../../src/render/resident-action-context.ts',
));
const { resolveResidentSemanticMotion } = load(resolve(
  import.meta.dirname,
  '../../src/render/resident-semantic-motion.ts',
));
const { residentSocialCuePairs } = load(resolve(
  import.meta.dirname,
  '../../src/render/resident-social-cues.ts',
));

const resident = (extra = {}) => ({
  id: 'a',
  name: '민재',
  presentation: {
    active: true,
    kind: 'Physical',
    phase: 'Moving',
    physicalGoal: 'Drink',
  },
  ...extra,
});

const residents = [
  resident(),
  {
    id: 'b',
    name: '하린',
  },
];

let passed = 0;
function testCase(name, run) {
  run();
  passed += 1;
  console.log('PASS ' + name);
}

testCase('inactive or idle presentation stays visually silent', () => {
  assert.equal(
    residentActionCue(
      resident({ presentation: { active: false } }),
      residents,
    ),
    null,
  );
  assert.equal(
    residentActionCue(
      resident({
        presentation: {
          active: true,
          kind: 'Physical',
          phase: 'Idle',
          physicalGoal: 'Drink',
        },
      }),
      residents,
    ),
    null,
  );
});

testCase('physical movement uses only authoritative goal and phase', () => {
  const cue = residentActionCue(resident(), residents);
  assert.ok(cue);
  assert.equal(cue.phase, 'Moving');
  assert.equal(cue.text, '물 마시기 · 이동 중');
});

testCase('washing visibly states that carried water is being used', () => {
  const moving = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Moving',
        physicalGoal: 'Wash',
      },
    }),
    residents,
  );
  assert.equal(moving?.text, '소지한 물로 씻으러 이동 중');

  const washing = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Interacting',
        physicalGoal: 'Wash',
      },
    }),
    residents,
  );
  assert.equal(washing?.text, '소지한 물로 씻는 중');
});

testCase('known resident target is named without inventing a relationship', () => {
  const cue = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Moving',
        socialIntent: 'Approach',
        targetResidentId: 'b',
      },
    }),
    residents,
  );
  assert.ok(cue);
  assert.equal(cue.text, '다가가기 · 하린 · 이동 중');
});

testCase('unknown resident id is not shown as a fabricated target label', () => {
  const cue = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Comfort',
        targetResidentId: 'missing',
      },
    }),
    residents,
  );
  assert.ok(cue);
  assert.equal(cue.text, '위로하기 · 진행 중');
  assert.ok(!cue.text.includes('missing'));
});

testCase('validated object target can describe the object context', () => {
  const cue = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Interacting',
        physicalGoal: 'Sleep',
        hasObjectTarget: true,
        objectKind: 'Bed',
      },
    }),
    residents,
  );
  assert.ok(cue);
  assert.equal(cue.text, '잠자기 · 잠자리 · 진행 중');
});

testCase('civilization and parenting cues remain factual and compact', () => {
  const gather = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Civilization',
        phase: 'Moving',
        civilizationIntent: 'Gather',
        civilizationMaterial: 'Water',
        hasTargetGrid: true,
      },
    }),
    residents,
  );
  assert.equal(gather?.text, '물 있는 곳으로 이동 중');

  const gathering = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Civilization',
        phase: 'Interacting',
        civilizationIntent: 'Gather',
        civilizationMaterial: 'Water',
        hasTargetGrid: true,
      },
    }),
    residents,
  );
  assert.equal(gathering?.text, '물 채집 중');

  const drink = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Interacting',
        physicalGoal: 'Drink',
      },
    }),
    residents,
  );
  assert.equal(drink?.text, '물 마시기 · 진행 중');

  const exploration = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Civilization',
        phase: 'Moving',
        civilizationIntent: 'Explore',
        civilizationMaterial: 'Water',
        hasTargetGrid: true,
      },
    }),
    residents,
  );
  assert.equal(exploration?.text, '물 탐색 지역으로 이동 중');

  const parenting = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Parenting',
        phase: 'Interacting',
        parentingAction: 'Comfort',
        targetResidentId: 'b',
      },
    }),
    residents,
  );
  assert.equal(parenting?.text, '달래기 · 하린 · 진행 중');
});

testCase('sanitation wording distinguishes real sites from outdoor fallback', () => {
  const designated = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Moving',
        physicalGoal: 'UseToilet',
        designatedSanitationSite: true,
        sanitationSiteId: '7',
      },
    }),
    residents,
  );
  assert.equal(designated?.text, '위생 장소로 이동 중');

  const designatedUse = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Interacting',
        physicalGoal: 'UseToilet',
        designatedSanitationSite: true,
        sanitationSiteId: '7',
      },
    }),
    residents,
  );
  assert.equal(designatedUse?.text, '위생 장소 이용 중');

  const outdoors = residentActionCue(
    resident({
      presentation: {
        active: true,
        kind: 'Physical',
        phase: 'Moving',
        physicalGoal: 'UseToilet',
        emergencyFallback: true,
      },
    }),
    residents,
  );
  assert.equal(outdoors?.text, '야외 용변 장소로 이동 중');
});

testCase('semantic motion maps only authoritative safe interactions', () => {
  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Civilization',
    phase: 'Interacting',
    civilizationIntent: 'Craft',
    hasTargetGrid: true,
  }, { moving: false, nearbyResident: false }), 'work');

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Physical',
    phase: 'Interacting',
    physicalGoal: 'UseToilet',
    designatedSanitationSite: true,
  }, { moving: false, nearbyResident: false }), 'crouch');

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Physical',
    phase: 'Interacting',
    physicalGoal: 'Wash',
    hasObjectTarget: true,
    objectKind: 'Sink',
  }, { moving: false, nearbyResident: false }), 'interact');

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Social',
    phase: 'Interacting',
  }, { moving: false, nearbyResident: true }), 'talk');
});

testCase('UAL2 consumption is used only for real eat and drink actions', () => {
  for (const physicalGoal of ['Eat', 'Drink']) {
    assert.equal(resolveResidentSemanticMotion({
      active: true,
      kind: 'Physical',
      phase: 'Interacting',
      physicalGoal,
      hasTargetGrid: true,
    }, { moving: false, nearbyResident: false }), 'consume');
  }

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Physical',
    phase: 'Interacting',
    physicalGoal: 'Sleep',
    hasTargetGrid: true,
  }, { moving: false, nearbyResident: false }), 'idle');
});

testCase('UAL2 harvest and carry require authoritative civilization facts', () => {
  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Civilization',
    phase: 'Interacting',
    civilizationIntent: 'Gather',
    civilizationMaterial: 'PlantFood',
    hasTargetGrid: true,
  }, { moving: false, nearbyResident: false }), 'harvest');

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Civilization',
    phase: 'Moving',
    civilizationIntent: 'Craft',
    facilityAction: 'DeliverMaterial',
    hasTargetGrid: true,
  }, { moving: true, nearbyResident: false }), 'carry');

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Civilization',
    phase: 'Moving',
    civilizationIntent: 'Gather',
    civilizationMaterial: 'Wood',
    hasTargetGrid: true,
  }, { moving: true, nearbyResident: false }), 'walk');
});

testCase('unsupported or unvalidated actions still fail closed', () => {
  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Physical',
    phase: 'Interacting',
    physicalGoal: 'UseToilet',
  }, { moving: false, nearbyResident: false }), 'idle');

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Civilization',
    phase: 'Interacting',
    civilizationIntent: 'Craft',
    hasTargetGrid: false,
  }, { moving: false, nearbyResident: false }), 'idle');

  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Civilization',
    phase: 'Interacting',
    civilizationIntent: 'Gather',
    civilizationMaterial: 'Wood',
    hasTargetGrid: true,
  }, { moving: false, nearbyResident: false }), 'interact');
});

testCase('movement always keeps locomotion ownership', () => {
  assert.equal(resolveResidentSemanticMotion({
    active: true,
    kind: 'Physical',
    phase: 'Moving',
    physicalGoal: 'UseToilet',
    designatedSanitationSite: true,
  }, { moving: true, nearbyResident: false }), 'walk');
});



testCase('social cues require an authoritative interacting resident target', () => {
  const pairs = residentSocialCuePairs([
    resident({
      id: 'a',
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Comfort',
        targetResidentId: 'b',
      },
    }),
    { id: 'b', name: '하린' },
  ]);

  assert.equal(pairs.length, 1);
  assert.deepEqual(pairs[0], {
    key: 'a<>b',
    sourceId: 'a',
    targetId: 'b',
    kind: 'Social',
  });
});

testCase('moving, avoiding and missing-target residents do not draw social links', () => {
  const baseTarget = { id: 'b', name: '하린' };

  assert.equal(residentSocialCuePairs([
    resident({
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Moving',
        socialIntent: 'Approach',
        targetResidentId: 'b',
      },
    }),
    baseTarget,
  ]).length, 0);

  assert.equal(residentSocialCuePairs([
    resident({
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Avoid',
        targetResidentId: 'b',
      },
    }),
    baseTarget,
  ]).length, 0);

  assert.equal(residentSocialCuePairs([
    resident({
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Comfort',
        targetResidentId: 'missing',
      },
    }),
    baseTarget,
  ]).length, 0);
});

testCase('teaching and parenting interactions are factual social cue sources', () => {
  const pairs = residentSocialCuePairs([
    resident({
      id: 'a',
      presentation: {
        active: true,
        kind: 'KnowledgeTeaching',
        phase: 'Interacting',
        targetResidentId: 'b',
      },
    }),
    {
      id: 'b',
      name: '하린',
      presentation: {
        active: true,
        kind: 'Parenting',
        phase: 'Interacting',
        parentingAction: 'Educate',
        targetResidentId: 'c',
      },
    },
    { id: 'c', name: '서윤' },
  ]);

  assert.deepEqual(
    pairs.map((pair) => pair.kind),
    ['KnowledgeTeaching', 'Parenting'],
  );
});

testCase('reciprocal interactions render one bounded connector per pair', () => {
  const pairs = residentSocialCuePairs([
    resident({
      id: 'a',
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Repair',
        targetResidentId: 'b',
      },
    }),
    resident({
      id: 'b',
      name: '하린',
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Comfort',
        targetResidentId: 'a',
      },
    }),
  ], 12);

  assert.equal(pairs.length, 1);
  assert.equal(pairs[0].key, 'a<>b');
});

testCase('social cue count is capped for future larger populations', () => {
  const many = [];
  for (let index = 0; index < 20; index += 1) {
    many.push({
      id: 's' + index,
      name: 'S' + index,
      presentation: {
        active: true,
        kind: 'Social',
        phase: 'Interacting',
        socialIntent: 'Approach',
        targetResidentId: 't' + index,
      },
    });
    many.push({ id: 't' + index, name: 'T' + index });
  }

  assert.equal(residentSocialCuePairs(many, 5).length, 5);
});

console.log(passed + ' action-context regression checks passed');
