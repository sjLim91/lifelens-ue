import assert from 'node:assert/strict';

export function fieldF2eClarityChecks({test,source,chunk,windowOf}){
  const {fieldVisualSpriteTile,fieldContinuousGroundNoise,fieldGroundDetailMarks}
    =source('render/field-natural-art.ts');
  const {paintObservedFieldRiver}=source('render/field-river-presentation.ts');
  const {paintFieldWorld}=source('render/field-observer-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');

  test('F2e landscape zoom exceeds sprite zoom without moving any Core pick coordinates',()=>{
    const base=Math.min(360*.49,700*.35);
    assert.equal(fieldVisualSpriteTile(base,360,700),base);
    assert.ok(fieldVisualSpriteTile(base*3.2,360,700)<base*1.44);
    assert.ok(fieldVisualSpriteTile(base*3.2,360,700)>base*1.4);
    assert.equal(fieldVisualSpriteTile(base*.7,360,700),base*.7);
  });

  test('F2e continuous ground pigment uses absolute world position, not tiling boundaries',()=>{
    const seed='landscape-seed';
    const edge=fieldContinuousGroundNoise(seed,18,7);
    assert.equal(edge,fieldContinuousGroundNoise(seed,17+1,7));
    assert.ok(edge>=-1&&edge<=1);
    assert.notEqual(edge,fieldContinuousGroundNoise(seed,18.3,7.5));
    assert.equal(edge,fieldContinuousGroundNoise(seed,18,7));
    const first=fieldGroundDetailMarks(seed,2,3,.8,.6,.2);
    assert.deepEqual(first,fieldGroundDetailMarks(seed,2,3,.8,.6,.2));
    assert.ok(first.length>0);
    assert.ok(first.every(x=>x.u>=0&&x.u<=1&&x.v>=0&&x.v<=1));
    assert.deepEqual(fieldGroundDetailMarks(seed,2,3,0,0,0),[]);
  });

  test('F2e the same real Core fire and hare stay readable across mobile zooms',()=>{
    const resident={id:'r',name:'주민',sex:'Female',alive:true,hasPosition:true,
      gridX:16,gridY:16};
    const terrain=windowOf([chunk(0,0,'None',.5,{forestCoverage01:0,
      grassCoverage01:.75})],0,0,'zoom-truth');
    terrain.humanTraces={total:1,entries:[{id:'fire',kind:'Facility',
      gridX:22,gridY:20,sourceResidentId:'r',facilityKind:'FirePit',
      state:'Operational',active:true,lit:true,progress01:1,
      deliveredMaterialUnits:2,requiredMaterialUnits:2,
      cropPlanted:false,cropGrowth01:0,cropMoisture01:0,
      cropCare01:0,cropHarvestUnits:0}]};
    terrain.wildlife={available:true,geographicallyConditioned:true,
      regionalWeatherApplied:true,animals:[
        {id:'hare',species:'Hare',sex:'Female',gridX:11,gridY:11,ageDays:20},
    ]};
    const input={terrain,residents:[resident],civilization:{available:false},
      centerX:0,centerY:0,zoom:1,selectedResidentId:resident.id};
    const before=JSON.stringify(input),read=(zoom)=>{
      const figures=[],ctx={globalAlpha:1};
      for(const method of ['clearRect','fillRect','fillText','strokeText',
        'beginPath','moveTo','lineTo','closePath','fill','stroke','ellipse',
        'quadraticCurveTo'])ctx[method]=()=>{};
      const painter={draw:(_ctx,name,x,y,w,h)=>{
        figures.push({name,w,h});return true;
      }};
      const stats=paintFieldWorld(ctx,{width:360,height:700,zoom,
        centerChunkX:0,centerChunkY:0},
        {...input,zoom},new ResidentContinuity(),painter);
      return {figures,stats};
    };
    const near=read(3.2),far=read(1);
    const size=(state,name)=>state.figures.find(p=>p.name===name)?.w;
    assert.ok(size(far,'fire')>0&&size(near,'fire')>size(far,'fire'));
    assert.ok(size(near,'fire')<size(far,'fire')*1.45,
      'camera magnification made a giant bonfire');
    assert.ok(size(near,'hare')<size(far,'hare')*1.45);
    assert.equal(near.stats.markers.filter(m=>m.kind==='animal').length,1);
    assert.equal(far.stats.markers.filter(m=>m.kind==='animal').length,1);
    assert.equal(JSON.stringify(input),before,'read-only art changed Core truth');
  });

  test('F2e Core-linked river uses a bounded multi-curve painted corridor',()=>{
    const ops=[],ctx={lineWidth:0};
    for(const method of ['beginPath','moveTo','lineTo','closePath','fill','stroke']){
      ctx[method]=(...args)=>ops.push({method,args,width:ctx.lineWidth});
    }
    paintObservedFieldRiver(ctx,{x:30,y:60},{x:160,y:70},
      {width01:.06,bend:.09},170);
    assert.equal(ops.filter(x=>x.method==='fill').length,5);
    assert.equal(ops.filter(x=>x.method==='stroke').length,16);
    const vertex=ops.find(x=>x.method==='moveTo').args;
    assert.ok(Math.hypot(vertex[0]-30,vertex[1]-60)<15);
    assert.ok(ops.some(x=>x.method==='lineTo' && Math.hypot(x.args[0]-160,x.args[1]-70)<12));
    ops.length=0;
    paintObservedFieldRiver(ctx,{x:30,y:60},{x:30,y:60},
      {width01:.06,bend:.09},170);
    assert.equal(ops.length,0,'unsupported zero-length link gained a water line');
  });

  test('F2e default art still draws only actual Core animal IDs, not RPG monsters',()=>{
    const terrain=windowOf([chunk(0,0,'None',.5,{grassCoverage01:1})],
      0,0,'no-extra-world');
    terrain.wildlife={available:true,geographicallyConditioned:true,
      regionalWeatherApplied:true,animals:[]};
    const ops=[],ctx={globalAlpha:1};
    for(const method of ['clearRect','fillRect','fillText','strokeText',
      'beginPath','moveTo','lineTo','closePath','fill','stroke','ellipse',
      'quadraticCurveTo'])ctx[method]=(...args)=>ops.push([method,...args]);
    const drawn=[];
    const result=paintFieldWorld(ctx,{width:360,height:700,zoom:1,
      centerChunkX:0,centerChunkY:0},
      {terrain,residents:[],civilization:{available:false},
      centerX:0,centerY:0,zoom:1},
      new ResidentContinuity(),{draw:(_ctx,name)=>{drawn.push(name);return true;}});
    assert.equal(result.markers.filter(x=>x.kind==='animal').length,0);
    assert.ok(!drawn.includes('hare')&&!drawn.includes('deer'));
  });
}
