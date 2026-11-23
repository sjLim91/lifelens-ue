import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

export function fieldReferenceF2dChecks({test,source,chunk,windowOf}){
  const {fieldForestDisplayClusters,fieldNaturalFloorColor,
    fieldConnectedFloorColor,fieldObservedRockFace}
    =source('render/field-natural-art.ts');
  const {createFieldProjection}=source('render/field-projection.ts');
  const {paintFieldWorld}=source('render/field-observer-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');

  test('F2d reference camera retains orthographic ground axes and Core height',()=>{
    const view={width:360,height:720,zoom:1.5,centerChunkX:0,centerChunkY:0,
      panX:1.2,panZ:-2};
    const p=createFieldProjection(view);
    const a=p.project(0,0,.5),x=p.project(1,0,.5),z=p.project(0,1,.5);
    assert.ok(x.x>a.x&&Math.abs(x.y-a.y)<1e-7);
    assert.ok(z.y>a.y&&Math.abs(z.x-a.x)<1e-7);
    assert.ok(p.project(0,0,.9).y<p.project(0,0,.1).y);
    for(const [u,v,h] of [[-.8,.21,.12],[.5,.77,.5],[.3,-.9,.95]]){
      const q=p.project(u,v,h),back=p.inverse(q.x,q.y,h);
      assert.ok(Math.abs(back.x-u*8)<1e-8);
      assert.ok(Math.abs(back.z-v*8)<1e-8);
    }
  });

  test('F2d forest canopy depends on true Core coverage and groups coherently',()=>{
    const rich=fieldForestDisplayClusters('actual-seed',10,4,.94,'TemperateForest');
    const retry=fieldForestDisplayClusters('actual-seed',10,4,.94,'TemperateForest');
    assert.deepEqual(rich,retry,'repeat loading changed decorative canopy layout');
    assert.ok(rich.length>=5&&rich.length<=9);
    assert.ok(rich.every(p=>p.u>=0&&p.u<=1&&p.v>=0&&p.v<=1));
    assert.equal(fieldForestDisplayClusters('actual-seed',10,4,0,'TemperateForest').length,0,
      'a bare chunk gained invented woodland');
    assert.ok(fieldForestDisplayClusters('actual-seed',10,4,.35,'Taiga')
      .every(p=>p.name!=='oak'),'conifer biome displayed a broadleaf oak');
    // The canonical decision is coherence with the observed world, not
    // the F2-d fixed diagonal stamp coordinates. Adjacent chunks must not
    // reproduce the same two placements in every cell.
    const adjacent=fieldForestDisplayClusters('actual-seed',11,4,.94,'TemperateForest');
    assert.notDeepEqual(rich.map(p=>[p.u,p.v]),adjacent.map(p=>[p.u,p.v]),
      'forest repeated identical tile-local placement in adjacent chunks');
  });

  test('F2d flat Core rock biome never creates a fabricated steep cliff',()=>{
    assert.equal(fieldObservedRockFace([.5,.5,.5,.5],1),false);
    assert.equal(fieldObservedRockFace([.2,.2,.4,.5],.8),true);
    assert.equal(fieldObservedRockFace([.2,.2,.4,.5],0),false);
    assert.equal(fieldObservedRockFace([Number.NaN,.2,.4,.5],1),false);
  });

  test('F2d neighboring observed land blends; Core water does not become land',()=>{
    const a=chunk(0,0,'None',.47,{grassCoverage01:.85,forestCoverage01:.25});
    const b=chunk(1,0,'None',.59,{grassCoverage01:.05,forestCoverage01:.93});
    const t=fieldNaturalFloorColor(a),target=fieldNaturalFloorColor(b);
    assert.ok(/^#[0-9a-f]{6}$/i.test(t));
    assert.notEqual(t,target);
    const mix=fieldConnectedFloorColor(a,{east:b},.98,.5);
    assert.notEqual(t,mix,'ground still has a hard unblended chunk edge');
    assert.equal(fieldConnectedFloorColor(a,{east:b},.5,.5),t);
    const lake=chunk(1,0,'Lake',.5);
    assert.equal(fieldConnectedFloorColor(a,{east:lake},.98,.5),t,
      'land ground blurred into a body of water that Core says is absent');
  });

  test('F2d real Core wildlife and actual facilities remain authoritative',()=>{
    const terrain=windowOf([chunk(0,0,'None',.5,{forestCoverage01:.9,
      shrubCoverage01:.85,grassCoverage01:.8,rockCoverage01:1})],0,0,'core-real');
    terrain.wildlife={available:true,geographicallyConditioned:true,
      regionalWeatherApplied:true,animals:[
        {id:'hare-001',species:'Hare',sex:'Male',gridX:8,gridY:8,ageDays:24},
      ]};
    const resident={id:'resident-001',name:'주민',sex:'Female',alive:true,
      hasPosition:true,gridX:16,gridY:16};
    const original=JSON.stringify(terrain),sprites=[];
    const painter={draw:(_ctx,name,x,y,w,h)=>{sprites.push(name);return true;}};
    const ctx={globalAlpha:1};
    for(const method of ['clearRect','fillRect','fillText','strokeText',
      'beginPath','moveTo','lineTo','closePath','fill','stroke','ellipse',
      'quadraticCurveTo'])ctx[method]=()=>{};
    const out=paintFieldWorld(ctx,{width:360,height:720,zoom:1,
      centerChunkX:0,centerChunkY:0},{
      terrain,residents:[resident],civilization:{available:false},
      centerX:0,centerY:0,zoom:1,selectedResidentId:resident.id,
      worldMinute:720,environment:{available:true,precipitationType:'None',
        calendar:{daylight01:1}},
    },new ResidentContinuity(),painter);
    assert.equal(JSON.stringify(terrain),original,'presentation changed Core terrain');
    assert.equal(out.markers.filter(m=>m.kind==='animal').length,1);
    assert.equal(out.markers.filter(m=>m.kind==='resident').length,1);
    assert.equal(sprites.filter(x=>x==='cliff').length,0,
      'flat but rocky ground gained fake mountain cliffs');
  });

  test('F2d reference interpretation is pinned for every future rendering agent',()=>{
    const root=resolve(dirname(fileURLToPath(import.meta.url)),'../../..');
    const rule=readFileSync(resolve(root,'docs/FIELD_VISUAL_REFERENCE_CONTRACT_2026-11-19.md'),'utf8');
    const agents=readFileSync(resolve(root,'AGENTS.md'),'utf8');
    assert.match(rule,/시각적 품질과 2D 필드 아트 표현 방식의 레퍼런스/);
    assert.match(rule,/Core biome\/coverage\/고도/);
    assert.match(rule,/거미 구역/);
    assert.match(agents,/FIELD_VISUAL_REFERENCE_CONTRACT_2026-11-19/);
  });
}
