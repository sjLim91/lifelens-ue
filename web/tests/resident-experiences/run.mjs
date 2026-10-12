import assert from 'node:assert/strict';
import fs from 'node:fs';
import { Buffer } from 'node:buffer';
import ts from 'typescript';

const source=fs.readFileSync(
  new URL('../../src/ui/resident-experiences.ts',import.meta.url),'utf8');
const js=ts.transpileModule(source,{
  compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022},
}).outputText;
const {deriveResidentExperienceHighlights}=await import(
  `data:text/javascript;base64,${Buffer.from(js).toString('base64')}`);

const direct=(minute,tags,where='personal experience')=>({
  minute,tags,where,source:'DirectWitness',witnessed:true,
});
const records=[
  direct(200,['action_experience','Sleep','failure']),
  direct(300,['action_experience','Sleep','failure']),
  direct(310,['action_experience','Drink','success']),
  direct(350,['environment','contamination','avoidance'],'grid:10,20'),
  {...direct(400,['environment','contamination'],'grid:20,30'),
    source:'Inferred'},
  direct(500,['unrelated']),
];
const highlights=deriveResidentExperienceHighlights(records,3);
assert.equal(highlights.length,3);
assert.deepEqual(highlights.map(v=>v.minute),[350,310,300]);
assert.equal(highlights[0].location,'grid:10,20');
assert.equal(highlights[2].title,'수면 경험');
assert.equal(highlights[2].detail,'직접 시도에 실패함');
assert.deepEqual(deriveResidentExperienceHighlights(records,1).map(v=>v.minute),[350]);
assert.deepEqual(deriveResidentExperienceHighlights(undefined,3),[]);
assert.deepEqual(deriveResidentExperienceHighlights(records,0),[]);
assert.deepEqual(deriveResidentExperienceHighlights([
  {...direct(900,['action_experience','Eat','success']),witnessed:false},
  {...direct(910,['action_experience','Wash','success']),source:'ToldByOther'},
  direct(930,['action_experience','unknown','success']),
],3),[]);
console.log('resident firsthand experience readout passed');
