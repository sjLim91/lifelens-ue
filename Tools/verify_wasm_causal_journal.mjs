#!/usr/bin/env node
import assert from 'node:assert/strict';
import { readFile, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
const fixture = resolve(process.argv[2] || '/tmp/lifelens-causal-fixture');
const runtime = resolve(process.argv[3] || 'Clients/Web/generated');
const oldFetch=globalThis.fetch,oldWindow=globalThis.window,oldDocument=globalThis.document;
let fetches=0,client,resumed;
const timeout=setTimeout(()=>{console.error('WASM causal journal timed out');process.exit(1);},120000);
globalThis.fetch=async()=>{fetches++;throw Error('Unexpected network fetch');};
globalThis.window=globalThis;globalThis.document={currentScript:null};
try {
  const javascript=await readFile(resolve(runtime,'lifelens_core.js'),'utf8');
  const bytes=await readFile(resolve(runtime,'lifelens_core.wasm'));
  const {default:createModule}=await import('data:text/javascript;base64,'+Buffer.from(javascript).toString('base64'));
  const module=await createModule({wasmBinary:Uint8Array.from(bytes),locateFile:p=>p});
  const start=JSON.parse(await readFile(resolve(fixture,'start.json'),'utf8'));
  const expected=JSON.parse(await readFile(resolve(fixture,'completed.json'),'utf8'));
  const initial=await readFile(resolve(fixture,'start.bin'));
  client=new module.LifeLensWebClient();
  assert.equal(typeof client.cognitiveCausalJournalJson,'function');
  assert.equal(client.restoreSnapshotBase64(initial.toString('base64')),true);
  assert.deepEqual(JSON.parse(client.cognitiveCausalJournalJson(start.actor)),start);
  const gather=expected.records.flatMap(r=>r.steps).find(s=>s.action==='Gather'&&s.result===1);
  assert(gather && gather.learning.changed);
  client.runMinutes(gather.resultMinute-start.minute);
  const checkpoint=client.saveSnapshotBase64();
  resumed=new module.LifeLensWebClient();assert(resumed.restoreSnapshotBase64(checkpoint));
  const tail=expected.minute-gather.resultMinute;
  client.runMinutes(tail);resumed.runMinutes(tail);
  const actual=JSON.parse(client.cognitiveCausalJournalJson(start.actor));
  assert.deepEqual(actual,expected,'native-approved goal -> actual WASM Gather/Craft -> actual learning');
  assert.deepEqual(JSON.parse(resumed.cognitiveCausalJournalJson(start.actor)),expected,'save/reload cannot lose or relink a causal step');
  const unchanged=client.saveSnapshotBase64();client.cognitiveCausalJournalJson(start.actor);assert.equal(client.saveSnapshotBase64(),unchanged,'observation is read-only');
  assert.equal(client.newGame('874213954','',2),true);
  const req=JSON.parse(client.cognitiveRequestJson('1','Reflection'));
  assert.equal(client.submitLocalCognitiveProposal('1',req.minute,req.worldSeed,'CooperateWithResident',.9,'1','wasm-self-target','Reflection',req.contextFingerprint),false);
  const blocked=JSON.parse(client.cognitiveCausalJournalJson('1'));
  assert(blocked.records.some(r=>!r.accepted&&r.rejection===1&&r.steps.length===0));
  assert.equal(fetches,0);
  const result={scope:'controlled actual Core fixture; no model inference or fabricated action results',nativeWasmExactMatch:true,serializedContinuation:true,readOnly:true,rejectedNoSteps:true,fetches};
  await writeFile(resolve(fixture,'wasm-causal-result.json'),JSON.stringify(result,null,2));
  console.log('WASM causal journal: PASS '+JSON.stringify(result));
} catch(error) {
  console.error('WASM causal journal: FAIL '+(error instanceof Error?error.message:String(error)));process.exitCode=1;
} finally {
  client?.delete();resumed?.delete();clearTimeout(timeout);globalThis.fetch=oldFetch;
  if(oldWindow===undefined)delete globalThis.window;else globalThis.window=oldWindow;
  if(oldDocument===undefined)delete globalThis.document;else globalThis.document=oldDocument;
}
