import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import ts from 'typescript';
function load(path) {
  const module = { exports: {} };
  const code = ts.transpileModule(readFileSync(path, 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
  }).outputText;
  new Function('require','module','exports',code)(id => load(resolve(dirname(path),id+'.ts')),module,module.exports);
  return module.exports;
}
const { CameraInput, cameraPanDelta } = load(resolve(import.meta.dirname,'../../src/input/camera-input.ts'));
class Canvas {
  style = {}; handlers = new Map();
  addEventListener(name, fn) { this.handlers.set(name, fn); }
  removeEventListener(name) { this.handlers.delete(name); }
  setPointerCapture() {}
  fire(type, id, x, y, pointerType='touch', button=0) {
    this.handlers.get(type)?.({type,pointerId:id,clientX:x,clientY:y,pointerType,button,preventDefault(){}});
  }
}
let passed=0;
function test(name,run) {
  const canvas=new Canvas(), changes=[], taps=[], pans=[];
  const input=new CameraInput(canvas,{initialAngle:0,initialElevation:0.6,initialZoom:1,onChange:s=>changes.push(s),onTap:(...v)=>taps.push(v),onPan:(...v)=>pans.push(v)});
  try {run({canvas,input,changes,taps,pans});passed++;console.log(`PASS ${name}`);} finally {input.dispose();}
}
test('small touch jitter selects without moving the camera',({canvas,changes,taps})=>{
  canvas.fire('pointerdown',1,100,100);canvas.fire('pointermove',1,103,102);canvas.fire('pointerup',1,103,102);
  assert.equal(changes.length,0);assert.deepEqual(taps,[[103,102]]);
});
test('downward orbit increases elevation and cannot select',({canvas,input,taps})=>{
  canvas.fire('pointerdown',1,100,100);canvas.fire('pointermove',1,120,120);canvas.fire('pointerup',1,120,120);
  assert.ok(input.snapshot().angle>0);assert.ok(input.snapshot().elevation>0.6);assert.equal(taps.length,0);
});
test('two-finger gesture cannot turn into orbit when one finger lifts',({canvas,input,changes,taps,pans})=>{
  canvas.fire('pointerdown',1,100,100);canvas.fire('pointerdown',2,200,100);
  canvas.fire('pointermove',2,220,110);assert.ok(pans.length>0);assert.ok(input.snapshot().zoom>1);
  canvas.fire('pointerup',2,220,110);const count=changes.length;
  canvas.fire('pointermove',1,130,140);canvas.fire('pointerup',1,130,140);
  assert.equal(changes.length,count);assert.equal(taps.length,0);
  canvas.fire('pointerdown',3,100,100);canvas.fire('pointermove',3,120,110);assert.ok(changes.length>count);
});
test('cancel and lost capture clear input without selecting',({canvas,changes,taps})=>{
  for(const stop of ['pointercancel','lostpointercapture']){
    canvas.fire('pointerdown',1,100,100);canvas.fire(stop,1,100,100);canvas.fire('pointermove',1,140,140);canvas.fire('pointerup',1,100,100);
  }
  assert.equal(changes.length,0);assert.equal(taps.length,0);
});
test('release beyond tap slop is not selection even without a move event',({canvas,taps})=>{
  canvas.fire('pointerdown',1,100,100);canvas.fire('pointerup',1,140,140);assert.equal(taps.length,0);
});
test('mouse right orbit and middle pan preserve desktop controls',({canvas,changes,pans,taps})=>{
  canvas.fire('pointerdown',1,100,100,'mouse',2);canvas.fire('pointermove',1,120,110,'mouse',2);canvas.fire('pointerup',1,120,110,'mouse',2);
  assert.equal(changes.length,1);
  canvas.fire('pointerdown',1,100,100,'mouse',1);canvas.fire('pointermove',1,120,110,'mouse',1);canvas.fire('pointerup',1,120,110,'mouse',1);
  assert.deepEqual(pans,[[20,10]]);assert.equal(taps.length,0);
});
test('pan follows the fingers in all camera quadrants',()=>{
  for(const angle of [0,Math.PI/2,Math.PI,Math.PI*1.5]){
    const p=cameraPanDelta(angle,10,20),sin=Math.sin(angle),cos=Math.cos(angle);
    // Screen displacement is negative camera displacement projected on right/up.
    assert.ok(Math.abs(-(p.x*sin-p.z*cos)-10)<1e-8);
    assert.ok(Math.abs(p.x*cos+p.z*sin-20)<1e-8);
  }
});
console.log(`${passed} input regression checks passed`);
