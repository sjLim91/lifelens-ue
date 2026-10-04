// Node 22.13+; exercise the production session and summary without a browser.
import assert from 'node:assert/strict';
import { mkdtempSync, readFileSync, writeFileSync, rmSync } from 'node:fs';
import { stripTypeScriptTypes } from 'node:module';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const root = resolve(import.meta.dirname, '../..');
const temporary = mkdtempSync(join(tmpdir(), 'lifelens-session-'));
try {
  writeFileSync(join(temporary, 'package.json'), '{"type":"module"}');
  for (const name of ['world-session', 'resident-continuity', 'lifelens-contract', 'generated-core-contract', 'fast-forward']) {
    const folder = name === 'fast-forward' ? 'state' : 'runtime';
    const source = readFileSync(join(root, `src/${folder}/${name}.ts`), 'utf8');
    const output = stripTypeScriptTypes(source, { mode: 'transform' })
      .replace(/from '(\.\/[\w-]+)'/g, "from '$1.js'");
    writeFileSync(join(temporary, `${name}.js`), output);
  }
  const { WorldSession } = await import(pathToFileURL(join(temporary, 'world-session.js')));
  const { ResidentContinuity } = await import(pathToFileURL(join(temporary, 'resident-continuity.js')));
  const { OBSERVER_RUNTIME_CONTRACT } = await import(pathToFileURL(join(temporary, 'lifelens-contract.js')));
  const { captureFastForwardState, buildFastForwardSummary } = await import(pathToFileURL(join(temporary, 'fast-forward.js')));

  function fixture() {
    let minute = 480;
    let history = [];
    let unavailable = false;
    let dead = false;
    let detailsEmpty = false;
    let activityUnavailable = false;
    const calls = { detail: 0, terrain: 0, civilization: 0, objects: 0 };
    const resident = () => ({ id: '18446744073709551615', name: '주민', alive: !dead, hasPosition: !dead, gridX: 0, gridY: 0 });
    const core = {
      worldOverview: () => ({ minute, totalResidents: 1, livingResidents: dead ? 0 : 1 }),
      residentRuntime: () => ({ available: true, residents: [resident()] }),
      residents: () => {
        calls.detail++;
        return unavailable ? { available: false, residents: [] } : {
          available: true, residents: detailsEmpty ? [] : [{ ...resident(), lifeHistory: structuredClone(history) }],
        };
      },
      terrainWindow: () => {
        calls.terrain++;
        return { available: true, chunks: [{}], humanTraces: { total: 0, entries: [] } };
      },
      humanTracesWindow: () => ({ available: true, humanTraces: { total: 0, entries: [] } }),
      dynamicEnvironment: () => ({ available: false }),
      recentSocialEvents: () => ({ available: true, events: [] }),
      civilizationWorldWindow: () => {
        calls.civilization++;
        return { available: !activityUnavailable, facilities: [], resources: [], storages: [], recentDiscoveries: [] };
      },
      worldObjects: () => {
        calls.objects++;
        return { available: true, smartObjects: [], sanitationSites: [] };
      },
      runMinutes: minutes => {
        minute += minutes;
        history = [{ type: 'Married', minute: minute - 60, relatedCharacterIds: ['2'] }];
      },
    };
    return {
      session: new WorldSession(core, new ResidentContinuity()), calls,
      setUnavailable: value => { unavailable = value; },
      setDetailsEmpty: value => { detailsEmpty = value; },
      setActivityUnavailable: value => { activityUnavailable = value; },
      die: () => { dead = true; history = [{ type: 'Death', minute }]; },
    };
  }
  function capture(snapshot) {
    return captureFastForwardState({ ...snapshot, world: snapshot.overview, residents: snapshot.residentDetails ?? snapshot.residents });
  }

  {
    const { session, calls } = fixture();
    session.refresh();
    for (let i = 1; i < OBSERVER_RUNTIME_CONTRACT.residentDetailRefreshEverySnapshots; i++) session.refresh();
    assert.equal(calls.detail, 1, 'ordinary observation keeps the bounded detail cadence');
    session.refresh();
    assert.equal(calls.detail, 2);
    assert.equal(calls.terrain, 1, 'ordinary refresh reuses static terrain');
  }
  {
    const { session, calls } = fixture();
    session.refresh();
    session.forceWorldActivityRefresh();
    const before = capture(session.refresh());
    session.runMinutes(30 * 1440);
    session.forceWorldActivityRefresh();
    const after = capture(session.refresh());
    const summary = buildFastForwardSummary(30, before, after);
    assert.deepEqual(summary.lifeEvents, [{ residentId: '18446744073709551615', residentName: '주민', type: 'Married', minute: after.world.minute - 60 }],
      'same identities after fast-forward must expose new Core life history immediately');
    assert.deepEqual(before.residents[0].lifeHistory, [], 'the before capture remains detached');
    assert.equal(calls.detail, 3, 'each explicit capture reads fresh detail');
    assert.equal(calls.civilization, 3);
    assert.equal(calls.objects, 3);
    assert.equal(calls.terrain, 1, 'forced activity refresh does not rebuild terrain');
    session.refresh();
    assert.equal(calls.detail, 3, 'normal throttling resumes after the forced read');
  }
  {
    const { session, calls, setUnavailable } = fixture();
    session.refresh();
    session.runMinutes(1440);
    setUnavailable(true);
    session.forceWorldActivityRefresh();
    const unavailable = session.refresh();
    assert.deepEqual(unavailable.residents[0].lifeHistory, [], 'unavailable details never fabricate events');
    assert.equal(calls.detail, 2);
    setUnavailable(false);
    const recovered = session.refresh();
    assert.equal(calls.detail, 3, 'an unavailable forced read retries on the next snapshot');
    assert.equal(recovered.residents[0].lifeHistory[0].type, 'Married');
  }
  {
    const { session, setUnavailable } = fixture();
    session.refresh();
    session.runMinutes(1440);
    setUnavailable(true);
    assert.throws(() => session.refresh(true), /주민 상세/,
      'a required capture must reject cached history when fresh details are unavailable');
    setUnavailable(false);
    const recovered = session.refresh(true);
    assert.equal(recovered.residentDetails[0].lifeHistory[0].type, 'Married');
  }
  {
    const { session, setDetailsEmpty } = fixture();
    session.refresh();
    setDetailsEmpty(true);
    assert.throws(() => session.refresh(true), /현재 인구/,
      'an available but incomplete detail response cannot replace a required capture');
    setDetailsEmpty(false);
    assert.equal(session.refresh(true).residentDetails.length, 1);
  }
  {
    const { session, setActivityUnavailable } = fixture();
    session.refresh();
    setActivityUnavailable(true);
    assert.throws(() => session.refresh(true), /문명·시설/,
      'unavailable world activity cannot masquerade as empty authoritative state');
    setActivityUnavailable(false);
    assert.equal(session.refresh(true).civilization.available, true);
  }
  {
    const { session, die, calls } = fixture();
    const before = capture(session.refresh(true));
    session.runMinutes(1440);
    die();
    const snapshot = session.refresh(true);
    assert.deepEqual(snapshot.residents, [], 'dead residents never return to the rendering list');
    const after = capture(snapshot);
    const summary = buildFastForwardSummary(1, before, after);
    assert.deepEqual(summary.newlyDeceased, [{ id: '18446744073709551615', name: '주민' }]);
    assert.equal(summary.lifeEvents[0].type, 'Death', 'extinction must retain exact Core death history');
    assert.equal(calls.terrain, 1);
    const unchanged = buildFastForwardSummary(1, after, capture(session.refresh(true)));
    assert.deepEqual(unchanged.newlyDeceased, [], 'existing deaths must not be reported as new again');
    assert.deepEqual(unchanged.lifeEvents, []);
  }
  {
    const { session, die, setDetailsEmpty } = fixture();
    session.refresh(true);
    die();
    setDetailsEmpty(true);
    assert.throws(() => session.refresh(true), /현재 인구/,
      'zero living residents does not permit missing deceased records');
  }
  console.log('PASS world session: bounded cadence, fresh history, death summary, terrain reuse, unavailable rejection/retry');
} finally {
  rmSync(temporary, { recursive: true, force: true });
}
