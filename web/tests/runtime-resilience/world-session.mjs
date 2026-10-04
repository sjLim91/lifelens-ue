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
    const calls = { detail: 0, terrain: 0, civilization: 0, objects: 0 };
    const resident = () => ({ id: '18446744073709551615', name: '주민', alive: true, hasPosition: true, gridX: 0, gridY: 0 });
    const core = {
      worldOverview: () => ({ minute, livingResidents: 1 }),
      residentRuntime: () => ({ available: true, residents: [resident()] }),
      residents: () => {
        calls.detail++;
        return unavailable ? { available: false, residents: [] } : {
          available: true, residents: [{ ...resident(), lifeHistory: structuredClone(history) }],
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
        return { available: true, facilities: [], resources: [], storages: [], recentDiscoveries: [] };
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
    return { session: new WorldSession(core, new ResidentContinuity()), calls, setUnavailable: value => { unavailable = value; } };
  }
  function capture(snapshot) {
    return captureFastForwardState({ ...snapshot, world: snapshot.overview });
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
  console.log('PASS world session: bounded cadence, fresh fast-forward history, terrain reuse, unavailable retry');
} finally {
  rmSync(temporary, { recursive: true, force: true });
}
