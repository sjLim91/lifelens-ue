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
      target: ts.ScriptTarget.ES2022,
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
  buildFastForwardSummary,
  captureFastForwardState,
  FAST_FORWARD_CHUNK_MINUTES,
  MAX_FAST_FORWARD_DAYS,
  MINUTES_PER_DAY,
} = load(resolve(
  import.meta.dirname,
  '../../src/state/fast-forward.ts',
));

assert.equal(MINUTES_PER_DAY, 1440);
assert.equal(FAST_FORWARD_CHUNK_MINUTES, 360);
assert.equal(MAX_FAST_FORWARD_DAYS, 3650);

const before = captureFastForwardState({
  world: {
    minute: 1440,
    totalResidents: 4,
    livingResidents: 4,
    deceasedResidents: 0,
    households: 1,
    activeCouples: 0,
    activePregnancies: 0,
    majorLifeEvents: 2,
  },
  residents: [
    {
      id: 'a',
      name: '서윤',
      alive: true,
      lifeHistory: [{ type: 'Birth', minute: 0 }],
    },
    {
      id: 'b',
      name: '민재',
      alive: true,
      lifeHistory: [{ type: 'Birth', minute: 0 }],
    },
  ],
  civilization: {
    available: true,
    facilityCount: 1,
    techniqueFactCount: 1,
    totalStoredUnits: 5,
    facilities: [{
      id: 'f1',
      kind: 'Shelter',
      state: 'UnderConstruction',
      gridX: 1,
      gridY: 1,
      initiatedBy: 'a',
      lastWorkedBy: 'a',
      startedMinute: 800,
      completedMinute: 0,
      constructionWork: 2,
      requiredWork: 5,
      workProgress: 0.4,
      durability: 1,
      active: true,
      linkedStorage: '',
      requiredMaterialUnits: 4,
      deliveredMaterialUnits: 4,
      fuelUnits: 0,
      charcoalUnits: 0,
      oreUnits: 0,
      metalUnits: 0,
      heatLevel: 0,
      lit: false,
      burnMinutesRemaining: 0,
      lastFireMinute: 0,
      requirements: [],
    }],
    resources: [
      {
        id: 'wood',
        gridX: 0,
        gridY: 0,
        material: 'Wood',
        quantity: 100,
        maxQuantity: 100,
        renewable: true,
        regenerationPerDay: 1,
      },
      {
        id: 'stone',
        gridX: 0,
        gridY: 1,
        material: 'Stone',
        quantity: 20,
        maxQuantity: 50,
        renewable: false,
        regenerationPerDay: 0,
      },
    ],
    recentDiscoveries: [{
      factId: 'd1',
      technique: 'SharpFlake',
      discovererId: 'a',
      discovererName: '서윤',
      minute: 900,
      recipientCount: 0,
      livingKnowerCount: 1,
    }],
  },
  worldObjects: {
    available: true,
    sanitationSites: [{
      id: 's1',
      kind: 'DesignatedArea',
      gridX: 2,
      gridY: 2,
      establishedBy: 'a',
      establishedMinute: 1000,
      active: true,
      useCount: 1,
      improvementWork: 0,
      improvedBy: '',
      improvedMinute: 0,
    }],
  },
});

const after = captureFastForwardState({
  world: {
    minute: 44640,
    totalResidents: 5,
    livingResidents: 4,
    deceasedResidents: 1,
    households: 2,
    activeCouples: 1,
    activePregnancies: 1,
    majorLifeEvents: 7,
  },
  residents: [
    {
      id: 'a',
      name: '서윤',
      alive: true,
      lifeHistory: [
        { type: 'Birth', minute: 0 },
        { type: 'Married', minute: 22000 },
      ],
    },
    {
      id: 'b',
      name: '민재',
      alive: false,
      lifeHistory: [
        { type: 'Birth', minute: 0 },
        { type: 'Death', minute: 40000 },
      ],
    },
    {
      id: 'c',
      name: '하린',
      alive: true,
      lifeHistory: [{ type: 'Birth', minute: 30000 }],
    },
  ],
  civilization: {
    available: true,
    facilityCount: 2,
    techniqueFactCount: 3,
    totalStoredUnits: 17,
    facilities: [
      {
        id: 'f1',
        kind: 'Shelter',
        state: 'Operational',
        gridX: 1,
        gridY: 1,
        initiatedBy: 'a',
        lastWorkedBy: 'a',
        startedMinute: 800,
        completedMinute: 3000,
        constructionWork: 5,
        requiredWork: 5,
        workProgress: 1,
        durability: 1,
        active: true,
        linkedStorage: '',
        requiredMaterialUnits: 4,
        deliveredMaterialUnits: 4,
        fuelUnits: 0,
        charcoalUnits: 0,
        oreUnits: 0,
        metalUnits: 0,
        heatLevel: 0,
        lit: false,
        burnMinutesRemaining: 0,
        lastFireMinute: 0,
        requirements: [],
      },
      {
        id: 'f2',
        kind: 'FirePit',
        state: 'Operational',
        gridX: 10,
        gridY: 10,
        initiatedBy: 'a',
        lastWorkedBy: 'a',
        startedMinute: 5000,
        completedMinute: 7000,
        constructionWork: 2,
        requiredWork: 2,
        workProgress: 1,
        durability: 1,
        active: true,
        linkedStorage: '',
        requiredMaterialUnits: 2,
        deliveredMaterialUnits: 2,
        fuelUnits: 3,
        charcoalUnits: 0,
        oreUnits: 0,
        metalUnits: 0,
        heatLevel: 1,
        lit: true,
        burnMinutesRemaining: 30,
        lastFireMinute: 44600,
        requirements: [],
      },
    ],
    resources: [
      {
        id: 'wood',
        gridX: 0,
        gridY: 0,
        material: 'Wood',
        quantity: 70,
        maxQuantity: 100,
        renewable: true,
        regenerationPerDay: 1,
      },
      {
        id: 'stone',
        gridX: 0,
        gridY: 1,
        material: 'Stone',
        quantity: 40,
        maxQuantity: 50,
        renewable: false,
        regenerationPerDay: 0,
      },
    ],
    recentDiscoveries: [
      {
        factId: 'd1',
        technique: 'SharpFlake',
        discovererId: 'a',
        discovererName: '서윤',
        minute: 900,
        recipientCount: 0,
        livingKnowerCount: 1,
      },
      {
        factId: 'd2',
        technique: 'FireMaking',
        discovererId: 'a',
        discovererName: '서윤',
        minute: 6000,
        recipientCount: 1,
        livingKnowerCount: 2,
      },
    ],
  },
  worldObjects: {
    available: true,
    sanitationSites: [
      {
        id: 's1',
        kind: 'DesignatedArea',
        gridX: 2,
        gridY: 2,
        establishedBy: 'a',
        establishedMinute: 1000,
        active: true,
        useCount: 3,
        improvementWork: 0,
        improvedBy: '',
        improvedMinute: 0,
      },
      {
        id: 's2',
        kind: 'DugPit',
        gridX: 8,
        gridY: 8,
        establishedBy: 'a',
        establishedMinute: 5000,
        active: true,
        useCount: 0,
        improvementWork: 1,
        improvedBy: 'a',
        improvedMinute: 5500,
      },
    ],
  },
});

const summary = buildFastForwardSummary(30, before, after);

assert.equal(summary.startMinute, 1440);
assert.equal(summary.endMinute, 44640);
assert.equal(summary.endMinute - summary.startMinute, 30 * 1440);
assert.equal(summary.totalResidentsBefore, 4);
assert.equal(summary.totalResidentsAfter, 5);
assert.equal(summary.livingResidentsBefore, 4);
assert.equal(summary.livingResidentsAfter, 4);
assert.equal(summary.deceasedResidentsAfter, 1);
assert.equal(summary.householdsAfter, 2);
assert.equal(summary.couplesAfter, 1);
assert.equal(summary.pregnanciesAfter, 1);
assert.equal(summary.facilitiesBefore, 1);
assert.equal(summary.facilitiesAfter, 2);
assert.equal(summary.techniqueFactsBefore, 1);
assert.equal(summary.techniqueFactsAfter, 3);
assert.equal(summary.storedUnitsBefore, 5);
assert.equal(summary.storedUnitsAfter, 17);

assert.deepEqual(summary.newResidents, [{ id: 'c', name: '하린' }]);
assert.deepEqual(summary.newlyDeceased, [{ id: 'b', name: '민재' }]);
assert.equal(summary.facilityChanges.length, 2);
assert.ok(summary.facilityChanges.some(change => (
  change.id === 'f1'
  && change.fromState === 'UnderConstruction'
  && change.toState === 'Operational'
)));
assert.ok(summary.facilityChanges.some(change => (
  change.id === 'f2'
  && change.kind === 'FirePit'
  && change.toState === 'Operational'
)));
assert.deepEqual(summary.newDiscoveries.map(item => item.factId), ['d2']);
assert.ok(summary.lifeEvents.some(event => event.type === 'Married' && event.residentName === '서윤'));
assert.ok(summary.lifeEvents.some(event => event.type === 'Death' && event.residentName === '민재'));
assert.ok(summary.lifeEvents.some(event => event.type === 'Birth' && event.residentName === '하린'));

const wood = summary.resourceChanges.find(change => change.material === 'Wood');
const stone = summary.resourceChanges.find(change => change.material === 'Stone');
assert.equal(wood?.delta, -30);
assert.equal(stone?.delta, 20);
assert.equal(summary.sanitationSitesBefore, 1);
assert.equal(summary.sanitationSitesAfter, 2);

before.world.minute = 999999;
assert.equal(summary.startMinute, 1440, 'summary must remain detached from mutable snapshots');

console.log('PASS authoritative day fast-forward summary');
