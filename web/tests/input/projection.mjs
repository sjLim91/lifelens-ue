import assert from 'node:assert/strict';
import * as THREE from 'three';
import ts from 'typescript';
import { readFileSync } from 'node:fs';
const exports = {};
new Function('exports', 'require', ts.transpileModule(readFileSync(new URL('../../src/render/camera-ground-projection.ts', import.meta.url), 'utf8'), {
  compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 },
}).outputText)(exports, () => THREE);
const projection = new exports.CameraGroundProjection();
let cases = 0;
for (const [width, height] of [[393, 792], [915, 352], [1440, 900]]) {
  for (const angle of [0, Math.PI / 2, Math.PI, -0.68]) for (const pitch of [.28, .67, 1.18]) for (const zoom of [.55, 2.3, 6.4]) {
    const camera = new THREE.PerspectiveCamera(42, width / height, .1, 5000);
    const distance = 40 / zoom;
    const target = new THREE.Vector3(0, 5, 0);
    camera.position.set(Math.cos(angle) * Math.cos(pitch) * distance, 5 + Math.sin(pitch) * distance, Math.sin(angle) * Math.cos(pitch) * distance);
    camera.lookAt(target); camera.updateMatrixWorld();
    const x = .15, y = -.1, dx = 18 / width * 2, dy = -12 / height * 2;
    const before = projection.point(camera, x, y, 5);
    const after = projection.point(camera, x + dx, y + dy, 5);
    assert(before && after);
    const pan = new THREE.Vector3(before.x - after.x, 0, before.z - after.z);
    camera.position.add(pan); target.add(pan); camera.lookAt(target); camera.updateMatrixWorld();
    const screen = new THREE.Vector3(before.x, 5, before.z).project(camera);
    assert(Math.abs(screen.x - x - dx) < 1e-6 && Math.abs(screen.y - y - dy) < 1e-6,
      'ground must follow an 18px/12px drag at every viewport, heading, pitch and zoom');
    // A new zoom uses the same point under the pinch midpoint, not world center.
    const anchor = projection.point(camera, x, y, 5);
    camera.position.sub(target).multiplyScalar(.7).add(target); camera.updateMatrixWorld();
    const shifted = projection.point(camera, x, y, 5);
    assert(anchor && shifted);
    const compensation = new THREE.Vector3(anchor.x - shifted.x, 0, anchor.z - shifted.z);
    camera.position.add(compensation); target.add(compensation); camera.lookAt(target); camera.updateMatrixWorld();
    const anchored = new THREE.Vector3(anchor.x, 5, anchor.z).project(camera);
    assert(Math.abs(anchored.x - x) < 1e-6 && Math.abs(anchored.y - y) < 1e-6, 'pinch midpoint stays anchored');
    cases++;
  }
}
console.log('PASS '+cases+' camera projection/zoom anchor combinations');
