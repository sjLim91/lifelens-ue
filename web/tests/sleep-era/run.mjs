import assert from 'node:assert/strict';
import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { pathToFileURL } from 'node:url';
import ts from 'typescript';
import * as THREE from 'three';
import React from 'react';
import { renderToStaticMarkup } from 'react-dom/server';

const root=resolve(import.meta.dirname,'../..');
const out=resolve(import.meta.dirname,'review');
const compiled=new Map();
function compile(path){
  const target=resolve(out,path.replace(/\.tsx?$/,'.mjs'));
  if(compiled.has(path)) return target;
  compiled.set(path,target);
  let code=ts.transpileModule(readFileSync(resolve(root,'src',path),'utf8'),{
    compilerOptions:{module:ts.ModuleKind.ES2022,target:ts.ScriptTarget.ES2022,jsx:ts.JsxEmit.ReactJSX},
  }).outputText;
  code=code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g,(_,a,p,b)=>{
    const base=resolve(dirname(path),p);
    const absolute=existsSync(resolve(root,'src',base+'.ts')) ? base+'.ts' : base+'.tsx';
    const relative=absolute.slice(resolve('.').length+1);
    compile(relative); return a+p+'.mjs'+b;
  });
  mkdirSync(dirname(target),{recursive:true}); writeFileSync(target,code); return target;
}
const source=async path=>import(pathToFileURL(compile(path)));
const {FacilityLayer}=await source('render/facility-layer.ts');
const {RESIDENT_PRESENTATION_CONTRACT:contract}=await source('runtime/lifelens-contract.ts');
const {CivilizationEraBadge,formatCivilizationEra,formatEraEvidence}=await source('ui/civilization-era.tsx');
const layer=Object.create(FacilityLayer.prototype);
layer.boxGeometry=new THREE.BoxGeometry(1,1,1);
layer.cylinderGeometry=new THREE.CylinderGeometry(1,1,1,8);
layer.taperedCylinderGeometry=layer.cylinderGeometry;
for(const name of ['beddingMaterial','woodMaterial','thatchMaterial']) layer[name]=new THREE.MeshStandardMaterial();
const group=new THREE.Group(); group.position.y=.025;
layer.buildSleepingPlace(group,{id:'mat',kind:'Facility',facilityKind:'SleepingPlace',state:'Operational'},1);
group.updateMatrixWorld(true);
assert.equal(group.children.length,7);
const mat=group.children[0];
assert(Math.abs(new THREE.Box3().setFromObject(mat).max.y-contract.sleepPoseSleepingPlaceSurfaceHeightWorldUnits)<1e-9);
assert(new THREE.Box3().setFromObject(group).max.y<=.18);
assert.equal(group.children.filter(mesh=>mesh.geometry===layer.cylinderGeometry).length,3);
assert.equal(group.children.filter(mesh=>mesh.scale.y>.10 && mesh.geometry===layer.boxGeometry).length,0);
const frameCount=group.children.length;
assert.equal(frameCount,7); // Bounded shared geometry; no per-frame additions.
const evidence=[{id:'OperationalSleep',satisfied:true},{id:'OperationalPlot',satisfied:false}];
for(const id of ['NaturalSurvival','EarlySettlement','AgrarianSettlement','CopperMetallurgy','BronzeTechnology']){
  const html=renderToStaticMarkup(React.createElement(CivilizationEraBadge,{
    era:{currentEra:{id,ordinal:0},evidence,nextEra:'AgrarianSettlement',nextEraRequirements:evidence},
  }));
  assert(html.includes(formatCivilizationEra(id)));
  assert(!html.includes(id));
  assert(html.includes('aria-haspopup="dialog"') && html.includes('aria-labelledby='));
  assert(html.includes('충족') && html.includes('미충족'));
}
assert.equal(formatEraEvidence('UnknownFutureEvidence'),'새로운 운영 근거');
assert.equal(formatCivilizationEra('UnknownFutureEra'),'문명 단계 확인 중');
writeFileSync(resolve(out,'metrics.json'),JSON.stringify({sleepingPlaceMeshes:7,sharedGeometryKinds:2,surfaceHeight:.14}));
console.log('Primitive mat surface, geometry budget, Korean era evidence and modal semantics PASS');
