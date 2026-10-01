import assert from 'node:assert/strict';

export function mapSurfaceChecks({ test, source, THREE }) {
  const { terrainColor } = source('render/terrain-presentation.ts');
  const { createWaterSurfaceMaterial, configureWaterSurfaceEnvironment } =
    source('render/water-surface-material.ts');
  const land = elevation01 => ({ waterKind: 'None', elevation01 });
  const channels = color => [1, 3, 5].map(i => parseInt(color.slice(i, i + 2), 16));

  test('terrain colors are continuous at former elevation bands and new stops', () => {
    for (const elevation of [.27, .34, .41, .48, .55, .62, .69, .76, .83]) {
      const before = channels(terrainColor(land(elevation - .00001)));
      const after = channels(terrainColor(land(elevation + .00001)));
      assert.ok(before.every((value, i) => Math.abs(value - after[i]) <= 1),
        `visible color step at ${elevation}`);
    }
    assert.notEqual(terrainColor(land(.32)), terrainColor(land(.37)),
      'the transition must retain elevation variation');
  });

  test('terrain palette preserves water identity and never mutates Core observations', () => {
    for (const [waterKind, expected] of [['Ocean', '#1b4b63'], ['Coast', '#276878'], ['Wetland', '#496e58']]) {
      assert.equal(terrainColor(Object.freeze({ ...land(.6), waterKind })), expected);
    }
    const observation = Object.freeze({ ...land(.52), forestCoverage01: .8, grassCoverage01: .3, rockCoverage01: .2 });
    assert.equal(terrainColor(observation), terrainColor(observation));
    assert.equal(observation.elevation01, .52);
    for (const elevation of [-1, 0, 1, 2, NaN, Infinity]) {
      assert.match(terrainColor(land(elevation)), /^#[0-9a-f]{6}$/);
    }
  });

  test('water materials use independent scene fog uniforms and display transforms', () => {
    const first = createWaterSurfaceMaterial('River');
    const second = createWaterSurfaceMaterial('Lake');
    try {
      assert.equal(first.fog, true);
      assert.notEqual(first.uniforms.fogColor.value, second.uniforms.fogColor.value);
      assert.ok(first.uniforms.fogColor.value instanceof THREE.Color);
      for (const name of ['fogDensity', 'fogNear', 'fogFar']) assert.ok(name in first.uniforms);
      assert.match(first.vertexShader, /vec4 mvPosition = viewMatrix \* world/);
      assert.match(first.vertexShader, /#include <fog_vertex>/);
      for (const name of ['fog_pars_fragment', 'tonemapping_fragment', 'colorspace_fragment', 'fog_fragment']) {
        assert.ok(THREE.ShaderChunk[name], `missing Three.js shader chunk ${name}`);
        assert.ok(first.fragmentShader.includes(`#include <${name}>`));
      }
      assert.equal(first.depthWrite, false);
      assert.equal(first.depthTest, true);
      assert.equal(first.side, THREE.DoubleSide);
      assert.equal(first.uniforms.uFlow.value.length(), 0);
    } finally { first.dispose(); second.dispose(); }
  });

  test('daylight and weather remain per-material inputs without changing water shape or flow', () => {
    const first = createWaterSurfaceMaterial('River');
    const second = createWaterSurfaceMaterial('River');
    try {
      const flow = first.uniforms.uFlow.value.clone();
      const opacity = first.uniforms.uOpacity.value;
      configureWaterSurfaceEnvironment(first, { wind01: .5, rain01: .7, daylight01: 0 });
      assert.equal(first.uniforms.uDaylight.value, 0);
      assert.equal(second.uniforms.uDaylight.value, 1);
      assert.equal(first.uniforms.uOpacity.value, opacity);
      assert.ok(first.uniforms.uFlow.value.equals(flow));
      assert.match(first.fragmentShader, /water \*= mix\(0\.12, 1\.0, clamp\(uDaylight/);
    } finally { first.dispose(); second.dispose(); }
  });
}
