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
  const input=new CameraInput(canvas,{initialAngle:0,initialElevation:0.6,initialZoom:1,onChange:s=>changes.push(s),onTap:(...v)=>taps.push(v),onPan:(x,y)=>pans.push([x,y])});
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
test('pinch reaches close inspection zoom and clamps at the camera maximum',({canvas,input})=>{
  canvas.fire('pointerdown',1,100,100);canvas.fire('pointerdown',2,200,100);
  canvas.fire('pointermove',2,740,100);
  assert.equal(input.snapshot().zoom,6.4);
  canvas.fire('pointermove',2,1200,100);
  assert.equal(input.snapshot().zoom,6.4);
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


// Browser-like RAF: translate both fingers before flushing their shared frame.
{
  const frames = new Map(); let next = 1;
  globalThis.requestAnimationFrame = cb => { const id = next++; frames.set(id, cb); return id; };
  globalThis.cancelAnimationFrame = id => frames.delete(id);
  const flush = () => { const pending = [...frames.values()]; frames.clear(); pending.forEach(cb => cb(16)); };
  const canvas = new Canvas(), changes = [], pans = [], taps = [];
  const input = new CameraInput(canvas, { initialAngle: 0, initialElevation: .6, initialZoom: 6.4,
    onChange: (state, anchor) => changes.push({state,anchor}), onPan: (x,y,anchor) => pans.push({x,y,anchor}), onTap: (...args) => taps.push(args) });
  try {
    canvas.fire('pointerdown', 1, 100, 100); canvas.fire('pointerdown', 2, 200, 100);
    canvas.fire('pointermove', 1, 120, 110); canvas.fire('pointermove', 2, 220, 110);
    assert.equal(frames.size, 1); flush();
    assert.equal(changes.length, 0, 'translation at maximum zoom must not temporarily clamp and shrink the scene');
    assert.deepEqual(pans, [{x:20,y:10,anchor:{x:150,y:100}}]);
    canvas.fire('pointermove', 1, 130, 110); canvas.fire('pointermove', 2, 210, 110); flush();
    assert.equal(input.snapshot().zoom, 5.12);
    assert.deepEqual(changes[0].anchor, {x:170,y:110});
    canvas.fire('pointerup', 2, 210, 110);
    canvas.fire('pointermove', 1, 180, 150); canvas.fire('pointerup', 1, 180, 150);
    assert.equal(taps.length, 0); assert.equal(changes.length, 1, 'one remaining finger cannot start an orbit');
    canvas.fire('pointerdown', 3, 100, 100); canvas.fire('pointermove', 3, 108.1, 100);
    assert(input.snapshot().angle < .001, 'crossing tap slop does not replay the full 8px as a rotation jump');
    canvas.fire('pointerdown', 4, 200, 100); canvas.fire('pointermove', 4, 210, 100);
    assert.equal(frames.size, 1); input.dispose(); assert.equal(frames.size, 0, 'dispose cancels pending touch work');
  } finally { input.dispose(); delete globalThis.requestAnimationFrame; delete globalThis.cancelAnimationFrame; }
}
console.log('PASS coalesced gestures, zoom-limit pan, midpoint anchor, release and disposal');
await import('./projection.mjs');

test('near-coincident fingers rebase their pinch span and recover without a zoom jump',({canvas,input})=>{
  canvas.fire('pointerdown',1,100,100); canvas.fire('pointerdown',2,100,100);
  canvas.fire('pointermove',2,101,100); canvas.fire('pointermove',2,200,100);
  assert.equal(input.snapshot().zoom,1);
  canvas.fire('pointermove',2,250,100);
  assert.equal(input.snapshot().zoom,1.5);
});
{
  const events=new Map();
  globalThis.window={addEventListener:(name,cb)=>events.set(name,cb),removeEventListener:name=>events.delete(name)};
  try {
    test('losing foreground clears captured touch state before a fresh tap',({canvas,taps})=>{
      canvas.fire('pointerdown',1,100,100); canvas.fire('pointerdown',2,200,100);
      events.get('blur')();
      canvas.fire('pointerdown',3,100,100); canvas.fire('pointerup',3,100,100);
      assert.deepEqual(taps,[[100,100]]);
    });
    assert.equal(events.size,0,'disposing input removes its window listener');
  } finally { delete globalThis.window; }
}
