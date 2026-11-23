import assert from 'node:assert/strict';

export function fieldF2fHabitatChecks({test,source,chunk,windowOf}){
  const {fieldForestDisplayClusters,fieldContinuousGroundNoise,
    fieldGroundDetailMarks}=source('render/field-natural-art.ts');
  const {fieldVisualUnit}=source('render/field-scene-clarity.ts');
  const {drawFieldResident}=source('render/field-resident-presentation.ts');
  const {paintFieldWorld}=source('render/field-observer-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');

  test('F2f canopy comes from continuous world patches, not fixed diagonal stamps',()=>{
    const seed='canopy-test';
    const tree=fieldForestDisplayClusters(seed,2,3,.92,'TemperateForest');
    assert.deepEqual(tree,fieldForestDisplayClusters(seed,2,3,.92,'TemperateForest'));
    assert.ok(tree.length>=6&&tree.length<=9);
    const others=[fieldForestDisplayClusters(seed,3,3,.92,'TemperateForest'),
      fieldForestDisplayClusters(seed,2,4,.92,'TemperateForest')];
    assert.ok(others.every(group=>group.length===tree.length));
    assert.ok(others.some(group=>JSON.stringify(group)!==JSON.stringify(tree)));
    assert.ok(tree.every(p=>p.u>=0&&p.u<=1&&p.v>=0&&p.v<=1));
    assert.equal(fieldForestDisplayClusters(seed,2,3,0,'TemperateForest').length,0);
    assert.ok(fieldForestDisplayClusters(seed,2,3,.8,'Taiga')
      .every(p=>p.name!=='oak'),'biome cannot invent an unrelated hardwood');
    // Exact same world-position canopy potential is continuous across chunks.
    assert.equal(fieldContinuousGroundNoise(seed,(2+1)*.58,(3+.4)*.58),
      fieldContinuousGroundNoise(seed,(3+0)*.58,(3+.4)*.58));
    // Check selected candidates have above-average contiguous-patch potential.
    const unit=(s)=>fieldVisualUnit(seed,2,3,s);
    const candidates=Array.from({length:25},(_,i)=>{
      const u=.03+((i%5)+.12+unit(600+i*2)*.76)*.188;
      const v=.03+(Math.floor(i/5)+.12+unit(601+i*2)*.76)*.188;
      return fieldContinuousGroundNoise(seed,(2+u)*.58,(3+v)*.58)*.64
        +fieldContinuousGroundNoise(seed,(2+u)*1.43,(3+v)*1.43)*.30
        +(unit(850+i)-.5)*.18;
    });
    const selection=tree.map(p=>{
      // The selected positions still retain exact candidate positions.
      const i=Array.from({length:25},(_,i)=>i).find(i=>
        Math.abs(p.u-(.03+((i%5)+.12+unit(600+i*2)*.76)*.188))<1e-7
        &&Math.abs(p.v-(.03+(Math.floor(i/5)+.12+unit(601+i*2)*.76)*.188))<1e-7);
      assert.notEqual(i,undefined);
      return candidates[i];
    });
    const avg=values=>values.reduce((s,v)=>s+v,0)/values.length;
    assert.ok(avg(selection)>=avg(candidates),'forest candidates ignored world patch coherence');
  });

  test('F2f micro ground details improve close habitat without creating resources',()=>{
    const rich=fieldGroundDetailMarks('seed',1,2,.95,.7,.2);
    const bare=fieldGroundDetailMarks('seed',1,2,0,0,0);
    assert.ok(rich.length>=55&&rich.length<=80);
    assert.deepEqual(rich,fieldGroundDetailMarks('seed',1,2,.95,.7,.2));
    assert.deepEqual(bare,[]);
    assert.ok(rich.every(p=>p.u>=0&&p.u<=1&&p.v>=0&&p.v<=1));
  });

  test('F2f sleeping puppet is centered around its exact Core position, not shifted beside a bed',()=>{
    const calls=[],ctx={globalAlpha:1};
    for(const fn of ['save','restore','translate','rotate','scale','ellipse','fill',
      'stroke','beginPath','moveTo','lineTo','closePath','quadraticCurveTo']){
      ctx[fn]=(...args)=>calls.push({fn,args});
    }
    drawFieldResident(ctx,{id:'resident-sleep',sex:'Male'},150,200,64,85,
      {facing:'S',pose:'sleep',step:0});
    const translations=calls.filter(x=>x.fn==='translate');
    assert.deepEqual(translations[0].args,[150,200]);
    assert.deepEqual(translations[1].args,[0,55]);
    const rotation=calls.find(x=>x.fn==='rotate');
    assert.ok(rotation && Math.abs(rotation.args[0]+Math.PI/2)<1e-9);
    assert.equal(calls.at(-1).fn,'restore');
  });

  test('F2f decorative tree variation does not invent Core entities or move resident',()=>{
    const terrain=windowOf([chunk(0,0,'None',.5,{forestCoverage01:1,
      shrubCoverage01:.8,grassCoverage01:1})],0,0,'visual-truth');
    const person={id:'real-resident',name:'주민',sex:'Female',
      alive:true,hasPosition:true,gridX:16,gridY:16};
    const before=JSON.stringify({terrain,person});
    const painted=[],ctx={globalAlpha:1,filter:'none'};
    for(const fn of ['save','restore','translate','scale','beginPath','moveTo',
      'lineTo','closePath','fill','stroke','clearRect','fillRect','fillText',
      'strokeText','quadraticCurveTo','ellipse']){
      ctx[fn]=()=>{};
    }
    const output=paintFieldWorld(ctx,{
      width:400,height:720,zoom:1,centerChunkX:0,centerChunkY:0,
    },{terrain,residents:[person],civilization:{available:false},
      centerX:0,centerY:0,zoom:1},
      new ResidentContinuity(),{draw:(_ctx,name,x,y,w,h)=>{
        painted.push({name,x,y,w,h,filter:_ctx.filter});return true;
      }});
    const trees=painted.filter(p=>['pine','cedar','oak'].includes(p.name));
    assert.ok(trees.length>=3,'real forest coverage lost all visuals');
    assert.ok(trees.every(p=>p.filter.includes('hue-rotate(')));
    assert.equal(output.markers.filter(m=>m.kind==='resident').length,1);
    assert.equal(output.markers.filter(m=>m.kind==='animal').length,0);
    assert.equal(JSON.stringify({terrain,person}),before);
  });
}
