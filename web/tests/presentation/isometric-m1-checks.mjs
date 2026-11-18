import assert from 'node:assert/strict';

export function isometricM1Checks({ test, source, chunk, windowOf }) {
  const { observedRiverLinks, forestSilhouettes, terrainSlopeLight } =
    source('render/isometric-natural-presentation.ts');
  const { createIsometricProjection } = source('render/isometric-projection.ts');
  const { paintIsometricWorld } = source('render/isometric-canvas-world-renderer.ts');
  const { ResidentContinuity } = source('runtime/resident-continuity.ts');

  const ctxWithOps = () => {
    const ops = [];
    const ctx = {};
    for (const op of ['clearRect','fillRect','fillText','beginPath','moveTo',
      'lineTo','closePath','fill','stroke','arc','ellipse','strokeRect','quadraticCurveTo']) {
      ctx[op] = (...args) => ops.push(op === 'stroke'
        ? [op, ctx.lineWidth, ctx.strokeStyle] : [op, ...args]);
    }
    return {ctx, ops};
  };
  const view = {width:700,height:430,extent:2,zoom:1.8,centerChunkX:0,centerChunkY:0};
  const draw = (terrain, ctx) =>
    paintIsometricWorld(ctx, view, {
      terrain, residents:[], civilization:{available:false},
      centerX:0,centerY:0,zoom:view.zoom,
    }, new ResidentContinuity());

  test('M1 only explicit adjacent, observed Core water nodes authorize river ribbons', () => {
    const a = chunk(0,0,'River',.54,{waterAvailability:.8,
      hasDownstream:false,downstreamChunkX:1,downstreamChunkY:0});
    const b = chunk(1,0,'Stream',.48,{waterAvailability:.45});
    const real = windowOf([{...a,hasDownstream:true},b],0,0,'water-seed');
    assert.equal(observedRiverLinks(windowOf([a,b])).length,0);
    assert.equal(observedRiverLinks(real).length,1);
    assert.equal(observedRiverLinks(windowOf([
      {...a,hasDownstream:true,downstreamChunkX:3},
      b,chunk(3,0,'River',.42),
    ])).length,0,'unobserved intermediate river route must not be faked');
    assert.equal(observedRiverLinks(windowOf([
      {...a,hasDownstream:true}, {...b,waterKind:'None'},
    ])).length,0,'do not paint a river connection into dry Core terrain');
    assert.equal(observedRiverLinks(windowOf([
      {...a,hasDownstream:true},b,
    ],0,0,'water-seed'))[0].to.x,1);
    assert.ok(observedRiverLinks(real)[0].width01 < .09);
    assert.ok(Math.abs(observedRiverLinks(real)[0].bend) <= .085);
    assert.deepEqual(observedRiverLinks(real),observedRiverLinks(real));
    assert.equal(a.hasDownstream,false,'visuals mutated the original hydrology');
  });

  test('M1 river graphic is a muted bounded curve, not a straight neon-wide segment', () => {
    const a = chunk(0,0,'River',.54,{waterAvailability:.9,
      hasDownstream:true,downstreamChunkX:1,downstreamChunkY:0});
    const b = chunk(1,0,'Stream',.51,{waterAvailability:.3});
    const connected=ctxWithOps(), isolated=ctxWithOps();
    draw(windowOf([a,b],0,0,'water-graphic'),connected.ctx);
    draw(windowOf([{...a,hasDownstream:false},b],0,0,'water-graphic'),isolated.ctx);
    assert.equal(connected.ops.filter(x=>x[0]==='quadraticCurveTo').length,3);
    assert.equal(isolated.ops.filter(x=>x[0]==='quadraticCurveTo').length,0);
    const channels = connected.ops.filter(x=>x[0]==='stroke');
    assert.ok(channels.length>=3);
    assert.ok(channels.every(x=>x[1]<=10),'river was allowed to cover the mobile screen');
    assert.ok(!channels.some(x=>String(x[2]).includes('#4fc2dc')),
      'the old neon-blue straight-line treatment remained');
  });

  test('M1 tree silhouettes follow only observed forest coverage, biome and world seed', () => {
    const none=chunk(0,0,'None',.5,{forestCoverage01:0});
    const rich=chunk(0,0,'None',.5,{forestCoverage01:.88,biome:'TemperateForest'});
    const boreal={...rich,biome:'BorealForest'};
    const forest=forestSilhouettes(rich,'seed-one');
    assert.deepEqual(forestSilhouettes(none,'seed-one'),[]);
    assert.ok(forest.length>=4 && forest.length<=5);
    assert.ok(forest.every(t=>t.u>=.12&&t.u<=.88&&t.v>=.12&&t.v<=.88));
    assert.ok(forest.some(t=>t.broadleaf));
    assert.ok(forestSilhouettes(boreal,'seed-one').every(t=>!t.broadleaf));
    assert.deepEqual(forest,forestSilhouettes(rich,'seed-one'));
    assert.notDeepEqual(forest,forestSilhouettes(rich,'seed-two'));
    assert.deepEqual(forestSilhouettes({...rich,waterKind:'Ocean'},'seed-one'),[]);
  });

  test('M1 actual height differences drive stronger terrain shading and relief, with invertible ground', () => {
    assert.equal(terrainSlopeLight(.5,.5,.5,.5),1.015);
    assert.ok(terrainSlopeLight(.7,.3,.3,.7)>1.015);
    assert.ok(terrainSlopeLight(.3,.7,.7,.3)<1.015);
    const projection=createIsometricProjection(view);
    assert.ok(projection.relief > projection.tile*2,'terrain remained visually flat');
    const p=projection.project(.45,-.25,.73);
    const inverse=projection.inverse(p.x,p.y,.73);
    assert.ok(Math.abs(inverse.x-3.6)<1e-9);
    assert.ok(Math.abs(inverse.z+2)<1e-9);
  });

  test('M1 broadleaf forest actually draws rounded crowns while bare land draws none', () => {
    const plain=ctxWithOps(),forest=ctxWithOps();
    const ground=chunk(0,0,'None',.47,{
      forestCoverage01:0,rockCoverage01:0,waterAvailability:0
    });
    draw(windowOf([ground],0,0,'same-seed'),plain.ctx);
    draw(windowOf([{...ground,forestCoverage01:.86}],0,0,'same-seed'),forest.ctx);
    const bare=plain.ops.filter(x=>x[0]==='ellipse').length;
    const crowns=forest.ops.filter(x=>x[0]==='ellipse').length;
    assert.ok(crowns>=bare+4,'rounded forest canopy did not replace repeated triangle pins');
  });
}
