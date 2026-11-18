import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

export function fieldClarityF1bChecks({test,source,chunk,windowOf}) {
  const {fieldFacilityArt,fieldCoreClearances,fieldDecorativePositionClear,
    fieldSpriteVisualOcclusion,fieldVisualUnit}=source('render/field-scene-clarity.ts');
  const {paintFieldWorld}=source('render/field-observer-renderer.ts');
  const {FIELD_SPRITES}=source('render/field-asset-atlas.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');
  const physical=['PrimitiveStorage','WorkSurface','SleepingPlace','FirePit',
    'Furnace','CultivatedPlot','Shelter'];
  const expected=['store','workbench','sleeping','fire','furnace','farm','hut'];
  const base={
    kind:'Facility',sourceResidentId:'r1',state:'Operational',progress01:1,
    deliveredMaterialUnits:2,requiredMaterialUnits:2,
    active:true,lit:false,cropPlanted:false,cropGrowth01:0,cropMoisture01:0,
    cropCare01:0,cropHarvestUnits:0,
  };
  const facility=(kind,id='one',x=16,y=16,more={})=>({
    ...base,id:'facility:'+id,facilityKind:kind,gridX:x,gridY:y,...more,
  });
  const resident={
    id:'real-person',name:'실제 주민',sex:'Female',alive:true,hasPosition:true,
    gridX:16,gridY:16,
  };
  const terrain=(traces=[],coverage=0)=>
    ({...windowOf([chunk(0,0,'None',.5,{
      forestCoverage01:coverage,rockCoverage01:coverage,
      shrubCoverage01:coverage,grassCoverage01:coverage,
    })],0,0,'clarity-seed'),humanTraces:{total:traces.length,entries:traces}});
  const makeScene=(window,selectedResidentId=null,civilization={available:false})=>{
    const ops=[],ctx={globalAlpha:1};
    for(const name of ['clearRect','fillRect','fillText','strokeText',
      'beginPath','moveTo','lineTo','closePath','fill','stroke','arc',
      'ellipse','strokeRect','quadraticCurveTo']) {
      ctx[name]=(...args)=>ops.push({name,args,alpha:ctx.globalAlpha});
    }
    const sprites=[];
    const painter={draw:(_ctx,name,x,y,w,h)=>{
      sprites.push({name,x,y,w,h,opacity:ctx.globalAlpha});
      ops.push({name:'sprite',sprite:name,alpha:ctx.globalAlpha});
      return true;
    }};
    const result=paintFieldWorld(ctx,{
      width:700,height:450,zoom:1,centerChunkX:0,centerChunkY:0,
    },{
      terrain:window,residents:[resident],civilization,centerX:0,centerY:0,zoom:1,
      selectedResidentId,
    },new ResidentContinuity(),painter);
    return {ops,sprites,result};
  };

  test('F1b nearby seeded canopy positions are genuinely dispersed, deterministic and local',()=>{
    const seed='clarity-seed';
    const positions=Array.from({length:5},(_,i)=>({
      x:fieldVisualUnit(seed,0,0,1000+i*4),
      y:fieldVisualUnit(seed,0,0,1001+i*4),
    }));
    assert.ok(positions.every(p=>p.x>=0&&p.x<1&&p.y>=0&&p.y<1));
    const xs=positions.map(p=>p.x),ys=positions.map(p=>p.y);
    assert.ok(Math.max(...xs)-Math.min(...xs)>.48,'canopy clumped on one X point');
    assert.ok(Math.max(...ys)-Math.min(...ys)>.38,'canopy clumped on one Y point');
    assert.deepEqual(positions,Array.from({length:5},(_,i)=>({
      x:fieldVisualUnit(seed,0,0,1000+i*4),
      y:fieldVisualUnit(seed,0,0,1001+i*4),
    })));
    assert.notEqual(fieldVisualUnit(seed,0,0,1000),fieldVisualUnit('other-seed',0,0,1000));
  });

  test('F1b every operational Core facility kind has truthful different art, only Shelter is hut',()=>{
    assert.deepEqual(physical.map(kind=>fieldFacilityArt(facility(kind)).sprite),expected);
    assert.equal(fieldFacilityArt(facility('UnknownNewFacility')).sprite,'site');
    assert.equal(fieldFacilityArt(facility('PrimitiveStorage','planned',16,16,{state:'Planned'})).sprite,'site',
      'an unbuilt store was displayed as a completed structure');
    assert.equal(fieldFacilityArt(facility('Shelter','ruined',16,16,{state:'Ruined'})).sprite,'site',
      'ruins should not appear as completed houses');
    assert.ok(physical.every(kind=>fieldFacilityArt(facility(kind)).clearance>0));
    assert.equal(physical.filter(kind=>fieldFacilityArt(facility(kind)).sprite==='hut').length,1);
  });

  test('F1b village renders different actual work sites without duplicating fictional houses',()=>{
    const traces=physical.map((kind,i)=>facility(kind,String(i),4+i*3,8+(i%3)*6));
    const world=terrain(traces,.2);
    const town={available:true,settlements:[{
      id:'real-town',active:true,gridX:23,gridY:26,
    }]};
    const view=makeScene(world,'real-person',town);
    for(const type of [...expected,'flag']){
      assert.ok(view.sprites.some(item=>item.name===type),'missing sprite for '+type);
    }
    assert.equal(view.sprites.filter(item=>item.name==='hut').length,1,
      'fake additional houses remained around actual Core production sites');
    assert.equal(view.result.markers.filter(m=>m.kind==='trace').length,physical.length);
    assert.equal(view.result.markers.filter(m=>m.kind==='settlement').length,1);
    assert.equal(view.result.markers.filter(m=>m.kind==='resident').length,1);
    assert.equal(view.sprites.filter(item=>item.name==='citizen_f').length,1);
    assert.equal(view.sprites.filter(item=>item.name==='flag').length,1);
  });

  test('F1b decorative forest avoids real Core residents and operational facilities',()=>{
    const site=facility('WorkSurface','work',16,16);
    const env=terrain([site],.92);
    const original=JSON.stringify(env);
    const continuity=new ResidentContinuity();
    const reserved=fieldCoreClearances(env,[resident],{available:false},continuity);
    assert.ok(reserved.some(s=>s.category==='facility'&&s.id===site.id));
    assert.ok(reserved.some(s=>s.category==='resident'&&s.id===resident.id));
    assert.equal(fieldDecorativePositionClear(.5,.5,reserved,.17),false);
    assert.equal(fieldDecorativePositionClear(.02,.02,reserved,.015),true);
    const scene=makeScene(env,'real-person');
    assert.equal(JSON.stringify(env),original,'visual clearance mutated authoritative Core ecology');
    assert.ok(scene.result.markers.some(m=>m.id==='real-person'));
    assert.ok(scene.result.markers.some(m=>m.id===site.id));
    assert.ok(scene.sprites.filter(s=>['pine','cedar','oak'].includes(s.name)).length>0,
      'clearing sightlines erased all Core forest coverage');
  });

  test('F1b tree/structure occlusion fades only in front of observed resident',()=>{
    const resident=[{x:125,y:125,selected:false}];
    const sprite={name:'oak',x:128,y:133,w:76,h:108};
    assert.equal(fieldSpriteVisualOcclusion(sprite,resident),.52);
    assert.equal(fieldSpriteVisualOcclusion(sprite,[{...resident[0],selected:true}]),.23);
    assert.equal(fieldSpriteVisualOcclusion({...sprite,y:90},resident),1,
      'a tree behind the resident was incorrectly hidden');
    assert.equal(fieldSpriteVisualOcclusion({...sprite,x:350},resident),1);
    assert.equal(fieldSpriteVisualOcclusion({...sprite,name:'flowers'},resident),1);
    const scene=makeScene(terrain([facility('Shelter','s',17,15)],.82),'real-person');
    const all=scene.sprites.filter(s=>['oak','pine','cedar','hut'].includes(s.name));
    assert.ok(all.length>0);
    assert.ok(all.every(s=>s.opacity<=1&&s.opacity>=.23));
    const lastSprite=scene.ops.map(op=>op.name).lastIndexOf('sprite');
    assert.ok(scene.ops.slice(lastSprite+1).some(op=>op.name==='stroke'),
      'selected resident outline must render over any foreground scenery');
  });

  test('F1b mobile feed remains actionable and compact by default only on field view',()=>{
    const root=resolve(dirname(fileURLToPath(import.meta.url)),'../..');
    const overlay=readFileSync(resolve(root,'src/ui/observation-feed.tsx'),'utf8');
    const app=readFileSync(resolve(root,'src/App.tsx'),'utf8');
    const css=readFileSync(resolve(root,'src/ui/observer-shell.css'),'utf8');
    assert.match(overlay,/compactOnMobile = false/);
    assert.match(overlay,/aria-expanded=\{expanded\}/);
    assert.match(overlay,/onClick=\{\(\) => setExpanded/);
    assert.match(app,/compactOnMobile=\{activeRenderMode === 'field-canvas'\}/);
    assert.match(css,/field-compact-observation\.is-collapsed \.observation-feed-items/);
  });

  test('F1b only original bundled asset names are loaded; new facility sprite files exist',()=>{
    const root=resolve(dirname(fileURLToPath(import.meta.url)),'../../public/field');
    const required=['workbench','store','sleeping','furnace','farm','site','flag'];
    for(const sprite of required){
      assert.ok(FIELD_SPRITES.includes(sprite),sprite);
      const content=readFileSync(resolve(root,sprite+'.svg'),'utf8');
      assert.match(content,/^<svg xmlns="http:\/\/www\.w3\.org\/2000\/svg"/);
      assert.ok(content.length>650);
    }
    assert.equal(FIELD_SPRITES.length,20);
  });
}
