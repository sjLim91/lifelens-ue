import assert from 'node:assert/strict';

export function observedWorkReachP3cChecks({ test, source, THREE }) {
  const { boundedVisualWorkReach } = source('render/resident-work-reach.ts');

  test('P3c reach limits the gesture to resident height, never alters the Core resource point', () => {
    const shoulder = new THREE.Vector3(0, 1, 0);
    const observed = new THREE.Vector3(3, 0, -4);
    const original = observed.clone();
    const out = new THREE.Vector3();
    const result = boundedVisualWorkReach(shoulder, observed, 2, out);
    assert.equal(result, out, 'one reusable target vector per world');
    assert.ok(Math.abs(out.distanceTo(shoulder) - 2 * .58) < 1e-8);
    assert.deepEqual(observed.toArray(), original.toArray());
    assert.ok(new THREE.Vector3().subVectors(out, shoulder)
      .cross(new THREE.Vector3().subVectors(observed, shoulder)).length() < 1e-8,
      'hand does not point toward Core resource');
    assert.deepEqual(boundedVisualWorkReach(shoulder, observed, 2, out), out,
      'repeated animation frame is deterministic');
  });

  test('P3c near actual work targets remain exact; invalid targets never generate arm motion', () => {
    const shoulder = new THREE.Vector3(.2, 1.4, -.3);
    const exact = new THREE.Vector3(.31, 1.24, -.25);
    const out = new THREE.Vector3();
    assert.equal(boundedVisualWorkReach(shoulder, exact, 1.8, out), out);
    assert.deepEqual(out.toArray(), exact.toArray());
    assert.equal(boundedVisualWorkReach(shoulder, new THREE.Vector3(Infinity, 0, 0), 1.8, out), null);
    assert.equal(boundedVisualWorkReach(shoulder, exact, 0, out), null);
    assert.equal(boundedVisualWorkReach(new THREE.Vector3(NaN, 0, 0), exact, 1.8, out), null);
    assert.deepEqual(exact.toArray(), [.31, 1.24, -.25]);
  });
}
