// Standalone suite: no package.json or the shared tests.json needs to change.
// Run from repository root: node web/tests/runtime-resilience/run.mjs
import assert from 'node:assert/strict';
import { mkdtempSync, readFileSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { createRequire } from 'node:module';
import ts from 'typescript';

const root = resolve(import.meta.dirname, '../..');
const temporary = mkdtempSync(join(tmpdir(), 'lifelens-runtime-'));
const require = createRequire(import.meta.url);
let passed = 0;
async function test(name, action) {
  await action(); passed++; console.log(`PASS ${name}`);
}
function compile(name, source) {
  writeFileSync(join(temporary, `${name}.js`), ts.transpileModule(source, {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
  }).outputText);
  return require(join(temporary, `${name}.js`));
}
try {
  const loading = compile('runtime-loading', readFileSync(join(root, 'src/runtime/runtime-loading.ts'), 'utf8'));
  const responses = compile('core-response', readFileSync(join(root, 'src/runtime/core-response.ts'), 'utf8'));
  const valid = { js: 'export default () => {};', wasmBuffer: Uint8Array.from([0,97,115,109,1,0,0,0]).buffer };
  const text = value => JSON.stringify(value);
  await test('a successful response is read completely', async () => {
    assert.equal(await loading.readRuntimeResponse('/runtime', r => r.text(), 100, async () => new Response('ready')), 'ready');
  });
  await test('HTTP errors retain status and source', async () => {
    await assert.rejects(loading.readRuntimeResponse('/missing', r => r.text(), 100, async () => new Response('missing', {status:404})), /HTTP 404.*missing/);
  });
  await test('deadline aborts a stalled body after headers arrived', async () => {
    let signal;
    await assert.rejects(loading.readRuntimeResponse('/body', r => r.text(), 15, async (_url, options) => {
      signal = options.signal;
      return {ok:true,text:() => new Promise(() => {})};
    }), /timed out/);
    assert.equal(signal.aborted, true);
  });
  await test('deadline still settles if fetch ignores its abort signal', async () => {
    await assert.rejects(loading.readRuntimeResponse('/headers', r=>r.text(), 15, () => new Promise(()=>{})), /timed out/);
  });
  await test('successful load clears its abort deadline', async () => {
    let signal;
    await loading.readRuntimeResponse('/ok', r=>r.text(), 10, async (_url, options) => {
      signal=options.signal; return new Response('ok');
    });
    await new Promise(resolve=>setTimeout(resolve,25));
    assert.equal(signal.aborted,false);
  });
  await test('HTML, empty script and damaged WASM are rejected', () => {
    for (const js of ['', '  <!doctype html><html>']) assert.throws(()=>loading.validateRuntimeBytes({...valid,js}));
    for (const wasmBuffer of [new ArrayBuffer(0),new TextEncoder().encode('<html>').buffer]) {
      assert.throws(()=>loading.validateRuntimeBytes({...valid,wasmBuffer}),/binary header/);
    }
    assert.equal(loading.validateRuntimeBytes(valid),valid);
  });
  await test('initialization failure retries a complete second pair', async () => {
    const seen=[];
    const result=await loading.connectRuntimeSources([
      {name:'first',load:async()=>({...valid,js:'broken'})},
      {name:'second',load:async()=>valid},
      {name:'unused',load:async()=>{throw Error('must not run');}},
    ],async payload=>{seen.push(payload.js);if(payload.js==='broken')throw Error('link failure');return 'connected';});
    assert.equal(result,'connected');assert.deepEqual(seen,['broken',valid.js]);
  });
  await test('all failed sources fail closed with their reasons', async () => {
    await assert.rejects(loading.connectRuntimeSources([
      {name:'same-origin',load:async()=>{throw Error('offline');}},
      {name:'backend',load:async()=>({...valid,js:'<html>'})},
    ],async()=>{throw Error('unexpected');}), /same-origin: offline.*backend: Core JavaScript/);
  });
  await test('initialization deadline bounds a stalled factory', async () => {
    await assert.rejects(loading.withRuntimeDeadline(new Promise(()=>{}),15,'WASM'),/WASM timed out/);
  });
  await test('late initialization rejection is handled after timeout', async () => {
    await assert.rejects(loading.withRuntimeDeadline(new Promise((_,reject)=>setTimeout(()=>reject(Error('late')),30)),10,'WASM'));
    await new Promise(resolve=>setTimeout(resolve,40));
  });
  await test('invalid overview cannot turn into zero living residents', () => {
    for(const value of ['{','null','[]',text({}),text({minute:480,livingResidents:'4'}),text({minute:480,livingResidents:-1}),text({available:false,minute:0,livingResidents:0})]) {
      assert.throws(()=>responses.readWorldOverview(value));
    }
  });
  await test('authoritative extinction and 64-bit seed remain valid', () => {
    const value={available:true,minute:480,livingResidents:0,worldSeed:'18446744073709551615'};
    assert.deepEqual(responses.readWorldOverview(text(value)),value);
  });
  await test('resident IDs are retained as exact strings', () => {
    const value={available:true,residents:[{id:'18446744073709551615',name:'주민',hasPosition:true,gridX:1,gridY:-1}]};
    assert.deepEqual(responses.readResidents(text(value)),value);
  });
  await test('malformed or duplicate identities never enter the renderer', () => {
    for(const residents of [[null],[{id:1,name:'x'}],[{id:'a',name:'x'},{id:'a',name:'y'}],[{id:'a',name:'x',hasPosition:true,gridX:'2',gridY:3}]]) {
      assert.throws(()=>responses.readResidents(text({available:true,residents})));
    }
    assert.throws(()=>responses.readResidents('null'));
    assert.throws(()=>responses.readResidents('{}'));
  });
  await test('unavailable resident response stays unavailable', () => {
    assert.deepEqual(responses.readResidents(text({available:false})),{available:false,residents:[]});
  });
  // Exercise actual bridge methods with a fake ABI, not a second implementation.
  const bridgeSource=readFileSync(join(root,'src/runtime/core-bridge.ts'),'utf8').replaceAll('import.meta.env.BASE_URL',"'./'");
  const {LifeLensCoreBridge}=compile('core-bridge',bridgeSource);
  await test('bridge propagates corrupt overview instead of fabricating empty state', () => {
    const bridge=new LifeLensCoreBridge({worldOverviewJson:()=>'{'});
    assert.throws(()=>bridge.worldOverview(),/invalid JSON/);
  });
  await test('bridge preserves valid data through the actual ABI methods', () => {
    const overview={minute:500,livingResidents:4};
    const residents={available:true,residents:[{id:'1',name:'주민'}]};
    const bridge=new LifeLensCoreBridge({worldOverviewJson:()=>text(overview),residentsJson:()=>text(residents)});
    assert.deepEqual(bridge.worldOverview(),overview);assert.deepEqual(bridge.residents(),residents);
  });
  console.log(`${passed} runtime resilience checks passed`);
} finally { rmSync(temporary,{recursive:true,force:true}); }
