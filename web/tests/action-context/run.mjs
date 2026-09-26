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
        phase: 'Interacting',
        civilizationIntent: 'Gather',
      },
    }),
    residents,
  );
  assert.equal(gather?.text, '채집 · 진행 중');

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

console.log(passed + ' action-context regression checks passed');
