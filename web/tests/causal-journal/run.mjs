import assert from 'node:assert/strict';
import { source } from '../procedural-environment/compile.mjs';
const { parseCognitiveCausalJournal: parse } = await source('cognition/cognitive-causal-journal.ts');
const { LifeLensCoreBridge } = await source('runtime/core-bridge.ts');
const sample = { schema:1, actor:'1', minute:510, worldSeed:'18446744073709551615',populationSeed:'9',generationVersion:8,olderRecords:0,
 records:[{id:'18446744073709551615',accepted:true,selectionState:1,rejection:0,minute:480,observedMinute:479,contextFingerprint:'9007199254740993',observedFingerprint:'9007199254740995',intent:'CraftUsefulTools',target:'0',omittedSteps:0,steps:[{
 token:'9007199254740997',previousToken:'0',exchangeFact:'0',issuedMinute:490,resultMinute:500,kind:2,source:0,result:1,attempted:true,target:'0',action:'Gather',material:'Wood',technique:'None',reason:'civilization-executed',
 learning:{changed:true,subject:'0',proposition:'experienced-action-outcome:Civilization:Gather:Wood',beforeCount:'0',afterCount:'1',beforeStance:0,afterStance:.2}}]}]};
assert.deepEqual(parse(sample,'1'),sample);assert.equal(parse(sample,'2'),null);
const mutate=fn=>{const v=structuredClone(sample);fn(v);assert.equal(parse(v,'1'),null);};
for(const bad of [1, '01','1e5','18446744073709551616','__proto__'])mutate(v=>v.records[0].steps[0].token=bad);
mutate(v=>v.records[0].contextFingerprint=9007199254740993);mutate(v=>v.records[0].accepted=false);
mutate(v=>v.records[0].minute=511);mutate(v=>v.records[0].observedMinute=481);
mutate(v=>v.records[0].steps[0].issuedMinute=470);mutate(v=>v.records[0].steps[0].resultMinute=511);
mutate(v=>v.records[0].steps[0].attempted=false);mutate(v=>v.records[0].steps[0].learning.afterCount='0');
mutate(v=>v.records[0].steps[0].learning.afterStance=Infinity);mutate(v=>v.records[0].steps[0].learning.proposition='');
mutate(v=>v.records[0].steps[0].source=2);mutate(v=>v.records[0].steps.push(structuredClone(v.records[0].steps[0])));
mutate(v=>v.records[0].steps[0].kind=99);mutate(v=>v.records[0].steps[0].result=99);
mutate(v=>v.records.push(structuredClone(v.records[0])));
const rejected=structuredClone(sample);Object.assign(rejected.records[0],{accepted:false,selectionState:0,rejection:1,steps:[]});assert(parse(rejected,'1'));
const pending=structuredClone(sample);Object.assign(pending.records[0].steps[0],{result:0,resultMinute:-1,attempted:false,learning:{changed:false,subject:'0',proposition:'',beforeCount:'0',afterCount:'0',beforeStance:0,afterStance:0}});assert(parse(pending,'1'));
const exchange=structuredClone(sample);Object.assign(exchange.records[0].steps[0],{token:'0',exchangeFact:'99',kind:5,source:1,action:'Exchange'});assert(parse(exchange,'1'));
const bridge=new LifeLensCoreBridge({cognitiveCausalJournalJson:()=>JSON.stringify(sample)});assert.deepEqual(bridge.cognitiveCausalJournal('1'),sample);
assert.equal(new LifeLensCoreBridge({}).cognitiveCausalJournal('1'),null);
assert.equal(new LifeLensCoreBridge({cognitiveCausalJournalJson:()=>'{'}).cognitiveCausalJournal('1'),null);
assert.equal(new LifeLensCoreBridge({cognitiveCausalJournalJson:()=>' '.repeat(262145)}).cognitiveCausalJournal('1'),null);
console.log('PASS read-only Core causal DTO preserves uint64 provenance, rejects cross-actor/future/false success/false learning/duplicate edges and handles old WASM');
