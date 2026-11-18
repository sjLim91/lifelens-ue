import assert from 'node:assert/strict';

export function isometricM102Checks({test,source,chunk,windowOf}) {
  const {observedShoreEdges,waterTileEdge,shoreStrokeWidths,
    observedGroundBrushes,observedSlopeContrast} =
    source('render/isometric-shoreline-presentation.ts');
  const {paintIsometricWorld} = source('render/isometric-canvas-world-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');
  const ctxWithOps=()=>{
    const ops=[];
    const ctx={};
    for (const name of ['clearRect','fillRect','fillText','beginPath','moveTo',
      'lineTo','closePath','fill','stroke','arc','ellipse','strokeRect','quadraticCurveTo']) {
      ctx[name]=(...args)=>ops.push({
        name,args,fillStyle:ctx.fillStyle,strokeStyle:ctx.strokeStyle,
        lineWidth:ctx.lineWidth,
      });
    }
    return {ctx,ops};
  };
  const draw=(terrain,ctx,zoom=1.8)=>{
    const view={width:680,height:450,extent:3,zoom,centerChunkX:0,centerChunkY:0};
    return paintIsometricWorld(ctx,view,{
      terrain,residents:[],civilization:{available:false},
      centerX:0,centerY:0,zoom,
    },new ResidentContinuity());
  };

  test('M1-02 shore topology requires both known adjacent Core chunks and actual water kind',()=>{
    const sea=chunk(0,0,'Ocean',.38);
    const coast=chunk(1,0,'Coast',.45);
    const dry=chunk(0,1,'None',.5);
    const real=windowOf([sea,coast,dry],0,0,'shore-seed');
    const edges=observedShoreEdges(real);
    assert.equal(edges.length,2);
    assert.equal(edges.find(e=>e.neighbor===coast)?.side,'east');
    assert.equal(edges.find(e=>e.neighbor===dry)?.side,'south');
    assert.ok(edges.every(e=>e.kind==='sea'));
    assert.equal(observedShoreEdges(windowOf([sea])).length,0,
      'an unobserved Core neighbor became an invented coastline');
    assert.equal(observedShoreEdges(windowOf([sea,chunk(1,0,'Ocean',.43)])).length,0,
      'continuous ocean was divided by a fake beach');
    assert.equal(observedShoreEdges(windowOf([chunk(0,0,'Coast',.5),dry])).length,0,
      'a coast zone without ocean became a fictional water edge');
    assert.deepEqual(observedShoreEdges(real),observedShoreEdges(real));
  });

  test('M1-02 lake shoreline never generates a second lake or unknown neighbor edge',()=>{
    const lake=chunk(0,0,'Lake',.39);
    const land=chunk(0,1,'None',.47);
    const connected=windowOf([lake,land],0,0,'lake-seed');
    assert.deepEqual(observedShoreEdges(connected).map(e=>[e.kind,e.side]),[['lake','south']]);
    assert.equal(observedShoreEdges(windowOf([lake,chunk(0,1,'Lake',.42)])).length,0);
    assert.deepEqual(waterTileEdge('south'),[{u:1,v:1},{u:0,v:1}]);
    assert.deepEqual(waterTileEdge('east'),[{u:1,v:0},{u:1,v:1}]);
    assert.deepEqual(waterTileEdge('north'),[{u:0,v:0},{u:1,v:0}]);
    assert.deepEqual(waterTileEdge('west'),[{u:0,v:1},{u:0,v:0}]);
  });

  test('M1-02 coastline curve and width stay bounded on compact/mobile and big screens',()=>{
    const sea=chunk(0,0,'Ocean',.34);
    const coast=chunk(1,0,'Coast',.46);
    const sourceTerrain=windowOf([sea,coast],0,0,'shore-form');
    const linked=ctxWithOps(), isolated=ctxWithOps();
    draw(sourceTerrain,linked.ctx);
    draw(windowOf([sea],0,0,'shore-form'),isolated.ctx);
    assert.equal(linked.ops.filter(op=>op.name==='quadraticCurveTo').length,3);
    assert.equal(isolated.ops.filter(op=>op.name==='quadraticCurveTo').length,0);
    const strokes=linked.ops.filter(op=>op.name==='stroke');
    assert.ok(strokes.every(op=>op.lineWidth<=5.5),'shore width overwhelms the map');
    assert.ok(strokes.some(op=>op.strokeStyle==='#78aca8'));
    assert.ok(strokes.some(op=>op.strokeStyle==='rgba(231,236,206,.69)'));
    assert.ok(shoreStrokeWidths(2000).bank<=5.5);
    assert.ok(shoreStrokeWidths(2).water>=.75);
    assert.ok(strokes.length>=3);
  });

  test('M1-02 coast is a sandy ground tile, open ocean remains water',()=>{
    const sand=ctxWithOps(),ocean=ctxWithOps();
    draw(windowOf([chunk(0,0,'Coast',.42)]),sand.ctx);
    draw(windowOf([chunk(0,0,'Ocean',.42)]),ocean.ctx);
    const firstFill=ops=>ops.find(op=>op.name==='fill')?.fillStyle;
    const sandy=firstFill(sand.ops),blue=firstFill(ocean.ops);
    const channels = color => {
      if (/^#[0-9a-f]{6}$/i.test(color)) return [
        parseInt(color.slice(1,3),16), parseInt(color.slice(3,5),16),
        parseInt(color.slice(5,7),16),
      ];
      const rgb = /^rgb\\(\\s*(\\d+)[, ]+\\s*(\\d+)[, ]+\\s*(\\d+)\\s*\\)$/.exec(color);
      assert.ok(rgb, 'unsupported Canvas CSS color: ' + color);
      return rgb.slice(1).map(Number);
    };
    const sandChannels = channels(sandy), seaChannels = channels(blue);
    assert.ok(sandChannels[0]>sandChannels[2]+25,'coast still painted as ocean blue');
    assert.ok(seaChannels[2]>seaChannels[0],'ocean no longer reads as water');
  });

  test('M1-02 ground details track only real Core grass/wetland/rock coverage',()=>{
    const plain=chunk(0,0,'None',.48,{grassCoverage01:0,
      wetlandCoverage01:0,rockCoverage01:0});
    const green={...plain,grassCoverage01:1,wetlandCoverage01:1,rockCoverage01:1};
    assert.deepEqual(observedGroundBrushes(plain,'terrain-seed'),[]);
    const brushes=observedGroundBrushes(green,'terrain-seed');
    assert.equal(brushes.filter(b=>b.kind==='grass').length,5);
    assert.equal(brushes.filter(b=>b.kind==='wetland').length,3);
    assert.equal(brushes.filter(b=>b.kind==='gravel').length,3);
    assert.ok(brushes.every(b=>b.u>=.13&&b.u<=.87&&b.v>=.13&&b.v<=.87));
    assert.deepEqual(brushes,observedGroundBrushes(green,'terrain-seed'));
    assert.notDeepEqual(brushes,observedGroundBrushes(green,'other-seed'));
    assert.deepEqual(observedGroundBrushes({...green,waterKind:'Lake'},'terrain-seed'),[]);
    assert.deepEqual(observedGroundBrushes({...green,waterKind:'Ocean'},'terrain-seed'),[]);
    const bare=ctxWithOps(),grassy=ctxWithOps();
    draw(windowOf([plain],0,0,'terrain-seed'),bare.ctx);
    draw(windowOf([green],0,0,'terrain-seed'),grassy.ctx);
    assert.ok(grassy.ops.filter(op=>op.name==='stroke').length
      >bare.ops.filter(op=>op.name==='stroke').length);
  });

  test('M1-02 relief accent responds only to observed Core elevation variations',()=>{
    assert.equal(observedSlopeContrast([.5,.5,.5,.5]),0);
    assert.equal(observedSlopeContrast([]),0);
    assert.equal(observedSlopeContrast([.1,NaN,.9]),0);
    assert.ok(observedSlopeContrast([.2,.5,.8,.4])>.2);
    const flat=ctxWithOps(),sloped=ctxWithOps();
    const detailed=Array.from({length:289},(_,i)=>.2+.55*(i%17)/16);
    draw(windowOf([chunk(0,0,'None',.42,{
      elevationSamples:Array(289).fill(.42),
      grassCoverage01:0,rockCoverage01:0,forestCoverage01:0,
    })]),flat.ctx);
    draw(windowOf([chunk(0,0,'None',.42,{
      elevationSamples:detailed,
      grassCoverage01:0,rockCoverage01:0,forestCoverage01:0,
    })]),sloped.ctx);
    assert.ok(sloped.ops.filter(op=>op.name==='quadraticCurveTo').length
      >flat.ops.filter(op=>op.name==='quadraticCurveTo').length);
  });
}
