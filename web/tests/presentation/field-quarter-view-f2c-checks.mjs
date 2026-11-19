import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

export function fieldQuarterViewF2cChecks({test,source,chunk,windowOf}){
  const {createFieldProjection}=source('render/field-projection.ts');
  const {fieldLightAndWeather,paintFieldAtmosphere}
    =source('render/field-weather-presentation.ts');
  const {paintFieldWorld}=source('render/field-observer-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');

  test('F2c actual 3/4 projection is reversible and Core height raises the field',()=>{
    for(const view of [
      {width:360,height:780,zoom:1,centerChunkX:0,centerChunkY:0,panX:2,panZ:-3},
      {width:900,height:540,zoom:1.7,centerChunkX:-2,centerChunkY:4,panX:0,panZ:0},
    ]){
      const p=createFieldProjection(view),o=p.project(0,0,.5);
      const east=p.project(1,0,.5),south=p.project(0,1,.5);
      assert.ok(east.x>o.x&&east.y>o.y);
      assert.ok(south.x<o.x&&south.y>o.y);
      for(const [x,y,h] of [[-.8,.5,.1],[.25,-.4,.89],[0,0,.5]]){
        const screen=p.project(x,y,h),back=p.inverse(screen.x,screen.y,h);
        assert.ok(Math.abs(back.x-x*8)<1e-7);
        assert.ok(Math.abs(back.z-y*8)<1e-7);
      }
      assert.ok(p.project(.2,-.1,.8).y<p.project(.2,-.1,.2).y);
    }
  });

  test('F2c actual rain/night Core signals produce visual cues, day and unknown weather do not',()=>{
    const ctx={ops:[]};
    for(const name of ['fillRect','beginPath','moveTo','lineTo','stroke','fill','ellipse']){
      ctx[name]=(...args)=>ctx.ops.push([name,...args]);
    }
    const evening={worldMinute:21*60+11,environment:{
      available:true,precipitationType:'Rain',summary:'Rain',
      precipitationIntensity01:.75,cloudCover01:.8,
      calendar:{hourOfDay:21,daylight01:0},
    }};
    assert.ok(fieldLightAndWeather(evening).nightOpacity>.4);
    paintFieldAtmosphere(ctx,350,700,evening,6000);
    assert.ok(ctx.ops.some(x=>x[0]==='fillRect'),'night did not darken the field');
    assert.ok(ctx.ops.some(x=>x[0]==='lineTo'),'Core rain produced no weather strokes');
    const count=ctx.ops.length;
    ctx.ops=[];
    paintFieldAtmosphere(ctx,350,700,evening,6000);
    assert.equal(ctx.ops.length,count,'weather effects must be repeatable for given frame');
    ctx.ops=[];
    paintFieldAtmosphere(ctx,350,700,{
      worldMinute:12*60,environment:{available:true,precipitationType:'None',
        precipitationIntensity01:0,cloudCover01:0,calendar:{daylight01:1}},
    },6000);
    assert.equal(ctx.ops.length,0,'clear noon gained fake rain or night');
    ctx.ops=[];
    paintFieldAtmosphere(ctx,350,700,{worldMinute:12*60,environment:null},6000);
    assert.equal(ctx.ops.length,0,'unknown weather gained fake precipitation');
    ctx.ops=[];
    paintFieldAtmosphere(ctx,350,700,{worldMinute:12*60,environment:{
      available:true,precipitationType:'Snow',precipitationIntensity01:.8,
      calendar:{daylight01:1},
    }},6000);
    assert.ok(ctx.ops.some(x=>x[0]==='ellipse'),'Core snowfall was invisible');
  });

  test('F2c the hare is materially smaller than a resident and deer in one real Core window',()=>{
    const r={id:'live-resident',name:'주민',sex:'Female',alive:true,
      hasPosition:true,gridX:16,gridY:16};
    const t=windowOf([chunk(0,0,'None',.5)],0,0,'scale-check');
    t.wildlife={available:true,geographicallyConditioned:true,
      regionalWeatherApplied:true,animals:[
        {id:'hare',species:'Hare',sex:'Male',ageDays:80,gridX:6,gridY:8},
        {id:'deer',species:'Deer',sex:'Female',ageDays:800,gridX:26,gridY:23},
      ]};
    const ops=[],ctx={globalAlpha:1};
    for(const name of ['clearRect','fillRect','fillText','strokeText','beginPath',
      'moveTo','lineTo','closePath','fill','stroke','ellipse',
      'quadraticCurveTo']){
      ctx[name]=(...args)=>ops.push({name,args});
    }
    const pictures=[];
    const painter={draw:(_ctx,name,x,y,w,h)=>{
      pictures.push({name,x,y,w,h});return true;
    }};
    const opts={terrain:t,residents:[r],civilization:{available:false},
      centerX:0,centerY:0,zoom:1,worldMinute:720,environment:null};
    const before=JSON.stringify(opts);
    const result=paintFieldWorld(ctx,{
      width:360,height:740,zoom:1,centerChunkX:0,centerChunkY:0,
    },opts,new ResidentContinuity(),painter);
    const byName=(name)=>pictures.find(x=>x.name===name);
    assert.ok(byName('hare')&&byName('deer')&&byName('citizen_f'));
    assert.ok(byName('hare').w < byName('citizen_f').w*.45,
      'hare remained human-sized');
    assert.ok(byName('deer').w > byName('hare').w*1.7);
    assert.equal(result.markers.filter(m=>m.kind==='animal').length,2);
    assert.equal(JSON.stringify(opts),before,'presentation changed the Core snapshot');
  });

  test('F2c field mobile HUD uses compact chrome without hiding observer actions',()=>{
    const root=resolve(dirname(fileURLToPath(import.meta.url)),'../..');
    const css=readFileSync(resolve(root,'src/ui/observer-shell.css'),'utf8');
    const engine=readFileSync(resolve(root,'src/observer-engine.ts'),'utf8');
    assert.match(css,/field-observer-world \.world-overview-card/);
    assert.match(css,/field-observer-world \.mobile-navigation button/);
    assert.match(engine,/worldMinute: selected.world.minute/);
    assert.match(engine,/environment: selected.environment/);
  });
}
