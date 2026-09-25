import assert from 'node:assert/strict';
import { mkdtempSync, readFileSync, writeFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { createRequire } from 'node:module';
import ts from 'typescript';

const temporary = mkdtempSync(join(tmpdir(), 'lifelens-continuity-'));
let passed = 0;
function test(name, run) { run(); passed++; console.log(`PASS ${name}`); }
const resident = (id, extra = {}) => ({id, name:id, hasPosition:true, gridX:2, gridY:3, ...extra});
const snapshot = residents => ({available:true,residents});
try {
  const source = readFileSync(resolve(import.meta.dirname,'../../src/runtime/resident-continuity.ts'),'utf8');
  const compiled = ts.transpileModule(source,{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText;
  const output = join(temporary,'continuity.cjs'); writeFileSync(output,compiled);
  const { ResidentContinuity } = createRequire(import.meta.url)(output);
  const fixture = () => {
    let now = 0;
    return {cache:new ResidentContinuity(100,()=>now),time:value=>{now=value;}};
  };
  test('brief transport gap preserves last known residents',()=>{
    const {cache,time}=fixture();cache.stabilize(snapshot([resident('a'),resident('b')]),2);
    time(50);assert.equal(cache.stabilize({available:false},2).length,2);
  });
  test('complete survivor snapshot prevents resurrection during later gap',()=>{
    const {cache,time}=fixture();cache.stabilize(snapshot([resident('a'),resident('b')]),2);
    time(10);cache.stabilize(snapshot([resident('b')]),1);
    time(20);assert.deepEqual(cache.stabilize({available:false},1).map(r=>r.id),['b']);
    assert.equal(cache.positionFor(resident('a',{hasPosition:false})),undefined);
  });
  test('new identities replace old identities even at identical population',()=>{
    const {cache}=fixture();cache.stabilize(snapshot([resident('old')]),1);
    cache.stabilize(snapshot([resident('new')]),1);
    assert.deepEqual(cache.stabilize({available:false},1).map(r=>r.id),['new']);
    assert.equal(cache.positionFor(resident('old',{hasPosition:false})),undefined);
  });
  test('expired resident also loses cached coordinates',()=>{
    const {cache,time}=fixture();cache.stabilize(snapshot([resident('a')]),1);time(101);
    assert.deepEqual(cache.stabilize({available:false},1),[]);
    assert.equal(cache.positionFor(resident('a',{hasPosition:false})),undefined);
  });
  test('position expires while non-position resident observations continue',()=>{
    const {cache,time}=fixture();cache.stabilize(snapshot([resident('a')]),1);
    time(90);cache.stabilize(snapshot([resident('a',{hasPosition:false})]),1);
    assert.deepEqual(cache.positionFor(resident('a',{hasPosition:false})),{x:2,y:3});
    time(101);assert.equal(cache.positionFor(resident('a',{hasPosition:false})),undefined);
  });
  test('repeated rendering cannot keep an unavailable resident alive',()=>{
    const {cache,time}=fixture();const a=resident('a');cache.stabilize(snapshot([a]),1);
    for(let n=1;n<100;n++){time(n);cache.positionFor(a);}
    time(101);assert.deepEqual(cache.stabilize({available:false},1),[]);
    assert.equal(cache.positionFor(resident('a',{hasPosition:false})),undefined);
  });
  test('unavailable payload contents cannot renew cache or insert identities',()=>{
    const {cache,time}=fixture();cache.stabilize(snapshot([resident('a')]),1);time(90);
    assert.deepEqual(cache.stabilize({available:false,residents:[resident('bogus')]},1).map(r=>r.id),['a']);
    time(101);assert.deepEqual(cache.stabilize({available:false,residents:[resident('a')]},1),[]);
  });
  test('explicit Core death invalidates cache even in a partial list',()=>{
    const {cache}=fixture();cache.stabilize(snapshot([resident('a'),resident('b')]),2);
    assert.deepEqual(cache.stabilize(snapshot([resident('a',{alive:false})]),1).map(r=>r.id),['b']);
    assert.equal(cache.positionFor(resident('a',{hasPosition:false})),undefined);
  });
  test('zero living count and reset both release the old world',()=>{
    const {cache}=fixture();cache.stabilize(snapshot([resident('a')]),1);
    cache.stabilize({available:false},0);assert.equal(cache.positionFor(resident('a',{hasPosition:false})),undefined);
    cache.stabilize(snapshot([resident('a')]),1);cache.reset();
    assert.deepEqual(cache.stabilize({available:false},1),[]);
  });
  test('invalid count fails before mutating the last valid cache',()=>{
    const {cache}=fixture();cache.stabilize(snapshot([resident('a')]),1);
    for(const count of [NaN,Infinity,-1,0.5])assert.throws(()=>cache.stabilize(snapshot([]),count));
    assert.equal(cache.stabilize({available:false},1)[0].id,'a');
  });
  test('nonfinite coordinates cannot poison last known position',()=>{
    const {cache}=fixture();cache.stabilize(snapshot([resident('a')]),1);
    const invalid=resident('a',{gridX:NaN,gridY:Infinity});cache.stabilize(snapshot([invalid]),1);
    assert.deepEqual(cache.positionFor(invalid),{x:2,y:3});
  });
  test('caller cannot mutate cached coordinates through returned object',()=>{
    const {cache}=fixture();cache.stabilize(snapshot([resident('a')]),1);
    const missing=resident('a',{hasPosition:false});cache.positionFor(missing).x=999;
    assert.deepEqual(cache.positionFor(missing),{x:2,y:3});
  });
  test('long generation churn cannot retain coordinates of old identities',()=>{
    const {cache}=fixture();
    for(let n=0;n<1000;n++) cache.stabilize(snapshot([resident(String(n))]),1);
    for(let n=0;n<999;n++)assert.equal(cache.positionFor(resident(String(n),{hasPosition:false})),undefined);
    assert.equal(cache.stabilize({available:false},1)[0].id,'999');
  });
  console.log(`${passed} resident continuity checks passed`);
} finally {rmSync(temporary,{recursive:true,force:true});}
