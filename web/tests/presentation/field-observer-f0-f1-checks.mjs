import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

export function fieldObserverF0F1Checks({test,source,chunk,windowOf}) {
  const {createFieldProjection}=source('render/field-projection.ts');
  const {paintFieldWorld,FieldObserverRenderer}=source('render/field-observer-renderer.ts');
  const {FieldSpriteAtlas,FIELD_SPRITES}=source('render/field-asset-atlas.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');
  const {WORLD_GRID_CONTRACT}=source('runtime/lifelens-contract.ts');
  const view={width:700,height:450,zoom:1,centerChunkX:0,centerChunkY:0,panX:0,panZ:0};
  const resident={id:'core-resident',name:'현재 주민',alive:true,sex:'Female',
    hasPosition:true,gridX:16,gridY:16};
  const env=(opts={})=>windowOf([chunk(0,0,'None',.5,opts)],0,0,'field-art-seed');
  const options=(terrain,overrides={})=>({
    terrain,residents:[resident],civilization:{available:false},
    centerX:0,centerY:0,zoom:1,selectedResidentId:resident.id,...overrides,
  });
  const fakeCtx=()=>{
    const ops=[];
    const ctx={};
    for(const op of ['clearRect','fillRect','fillText','strokeText','beginPath',
      'moveTo','lineTo','closePath','fill','stroke','arc','ellipse','strokeRect',
      'quadraticCurveTo','drawImage'])ctx[op]=(...args)=>ops.push([op,...args]);
    return {ctx,ops};
  };
  const paint=(terrain,overrides={})=>{
    const {ctx,ops}=fakeCtx(),drawn=[];
    const sprites={draw:(_ctx,name,x,y,w,h)=>{
      drawn.push({name,x,y,w,h});return true;
    }};
    const result=paintFieldWorld(ctx,view,options(terrain,overrides),
      new ResidentContinuity(),sprites);
    return {result,drawn,ops};
  };

  test('F0 field is top-down close-up rather than diamond world map, Core pan/height invert',()=>{
    const p=createFieldProjection({...view,panX:1.5,panZ:-1.2});
    const point=p.project(.23,-.37,.65);
    const inverse=p.inverse(point.x,point.y,.65);
    assert.ok(Math.abs(inverse.x-1.84)<1e-8);
    assert.ok(Math.abs(inverse.z+2.96)<1e-8);
    const a=p.project(0,0,.5),b=p.project(1,0,.5),c=p.project(0,1,.5);
    assert.ok(b.x>a.x&&Math.abs(b.y-a.y)<1e-9);
    assert.ok(c.y>a.y&&Math.abs(c.x-a.x)<1e-9);
    assert.ok(p.tile>100,'camera remains a tiny map instead of close-up field');
    const elevated=p.project(.1,.2,.8);
    const low=p.project(.1,.2,.2);
    assert.ok(elevated.y<low.y,'Core elevation was flattened');
    assert.ok(createFieldProjection({...view,width:340,height:700}).tile>80,
      'mobile field view is too zoomed out');
  });

  test('F1 original forest, cliffs, grass and bushes respond to observed Core ecology',()=>{
    const rich=env({forestCoverage01:.91,rockCoverage01:.86,
      shrubCoverage01:.9,grassCoverage01:.7});
    const barren=env({forestCoverage01:0,rockCoverage01:0,
      shrubCoverage01:0,grassCoverage01:0});
    const first=paint(rich),second=paint(rich),empty=paint(barren);
    const count=(x,names)=>x.drawn.filter(d=>names.includes(d.name)).length;
    assert.ok(count(first,['pine','cedar','oak'])>=8,'forest cover did not make substantial woodland');
    assert.ok(count(first,['cliff'])>=1,'Core rock cover did not create sizeable rock scenery');
    assert.ok(count(first,['bush'])>=2);
    assert.equal(count(empty,['pine','cedar','oak','cliff','bush','flowers']),0);
    assert.deepEqual(first.drawn,second.drawn,'art placement must be deterministic from world seed');
    assert.equal(first.result.chunks,1);
    assert.ok(first.result.tile>100);
  });

  test('F1 close field has flowers only with grass habitat and not a fake resource',()=>{
    const grass=paint(env({grassCoverage01:.9,forestCoverage01:.22}));
    const bare=paint(env({grassCoverage01:.0,forestCoverage01:.22}));
    assert.ok(grass.drawn.some(x=>x.name==='flowers'));
    assert.ok(!bare.drawn.some(x=>x.name==='flowers'));
    assert.ok(grass.result.markers.every(m=>m.kind!=='resource'),
      'decorative flowers became invented harvestable nodes');
  });

  test('F0 actual Core resident, facility and settlement are drawn/selectable at their grid point',()=>{
    const trace={id:'facility:real',kind:'Facility',gridX:6,gridY:6,
      sourceResidentId:resident.id,facilityKind:'FirePit',
      state:'Operational',progress01:1,deliveredMaterialUnits:2,
      requiredMaterialUnits:2,active:true,lit:true,
      cropPlanted:false,cropGrowth01:0,cropMoisture01:0,
      cropCare01:0,cropHarvestUnits:0};
    const terrain={...env(),humanTraces:{total:1,entries:[trace]}};
    const {ctx}=fakeCtx();
    const canvas={width:700,height:450,
      getContext:kind=>kind==='2d'?ctx:null,
      getBoundingClientRect:()=>({left:10,top:20,width:700,height:450})};
    const renderer=new FieldObserverRenderer(canvas,new ResidentContinuity(),()=>{});
    try {
      renderer.draw(options(terrain,{civilization:{available:true,
        settlements:[{id:'real-settlement',active:true,gridX:23,gridY:23}]}}));
      const p=createFieldProjection(view);
      const residentPoint=p.gridPoint(16,16,.5);
      assert.equal(renderer.pickResident(residentPoint.x+10,residentPoint.y+12),resident.id);
      const fire=p.gridPoint(6,6,.5);
      assert.equal(renderer.pickHumanTrace(fire.x+10,fire.y+20),trace.id);
      const settlement=p.gridPoint(23,23,.5);
      assert.equal(renderer.pickSettlement(settlement.x+10,settlement.y+20),'real-settlement');
      assert.equal(renderer.pickResident(-800,-900),null);
      assert.equal(renderer.pickHumanTrace(-800,-900),null);
      const ground=renderer.groundPointAtClient(residentPoint.x+10,residentPoint.y+20);
      assert.ok(ground && Number.isFinite(ground.x) && Number.isFinite(ground.z));
    } finally {renderer.dispose();}
  });

  test('F1 wildlife is shown only for actual observed Core animals, never generated monsters',()=>{
    const t=env({forestCoverage01:.45});
    t.wildlife={available:true,geographicallyConditioned:true,regionalWeatherApplied:true,
      animals:[{id:'real-deer',species:'Deer',sex:'Male',gridX:8,gridY:8,ageDays:12}]};
    const actual=paint(t);
    assert.equal(actual.drawn.filter(d=>d.name==='deer').length,1);
    assert.equal(actual.result.markers.filter(m=>m.kind==='animal').length,1);
    const empty=paint(env({forestCoverage01:.45}));
    assert.ok(!empty.drawn.some(x=>x.name==='deer'));
    assert.equal(empty.result.markers.filter(m=>m.kind==='animal').length,0);
  });

  test('F1 original transparent SVG field assets exist and are independent of external services',()=>{
    const root=resolve(dirname(fileURLToPath(import.meta.url)),'../../public/field');
    assert.equal(FIELD_SPRITES.length,13);
    for(const name of FIELD_SPRITES) {
      const content=readFileSync(resolve(root,name+'.svg'),'utf8');
      assert.ok(content.startsWith('<svg xmlns="http://www.w3.org/2000/svg"'),name);
      assert.ok(content.includes('</svg>'),name);
      assert.ok(content.length>650,'incomplete authored image '+name);
      assert.ok(!/https?:\/\//.test(content.replace('http://www.w3.org/2000/svg','')),
        'asset refers to remote paid dependency '+name);
    }
  });

  test('F0 field atlas does not allocate GPU contexts, retries or images per resident',()=>{
    const atlas=new FieldSpriteAtlas(()=>{});
    const ctx=fakeCtx().ctx;
    assert.equal(atlas.draw(ctx,'pine',0,0,44,90),false);
    assert.equal(atlas.draw(ctx,'pine',0,0,44,90),false);
    atlas.dispose();
    assert.equal(atlas.draw(ctx,'pine',0,0,44,90),false);
  });
}
