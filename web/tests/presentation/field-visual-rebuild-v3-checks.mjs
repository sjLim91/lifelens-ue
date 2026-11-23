import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';

export function fieldVisualRebuildV3Checks({test,source,chunk,windowOf}){
  const {fieldWorldGroundColor,FieldTerrainCompositor,paintFieldShore}
    =source('render/field-terrain-compositor.ts');
  const {fieldObservedCampWashes,paintFieldCampGround}
    =source('render/field-settlement-illustration.ts');
  const {paintFieldWorld}=source('render/field-observer-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');
  const {drawFieldResident}=source('render/field-resident-presentation.ts');

  test('V3 terrain paints continuously through actual neighboring Core land',()=>{
    const a=chunk(0,0,'None',.53,{forestCoverage01:.83,grassCoverage01:.17});
    const b=chunk(1,0,'None',.49,{forestCoverage01:.14,grassCoverage01:.9});
    const c=chunk(1,1,'None',.53,{forestCoverage01:.2,grassCoverage01:.5});
    const d=chunk(0,1,'None',.59,{forestCoverage01:.6,grassCoverage01:.4});
    const lookup=(x,y)=>[a,b,c,d].find(q=>q.x===x&&q.y===y);
    const left=fieldWorldGroundColor('same-world',0,0,1,.38,lookup);
    const right=fieldWorldGroundColor('same-world',1,0,0,.38,lookup);
    assert.ok(left.every((value,i)=>Math.abs(value-right[i])<1e-7),
      'land still exposes a rectangular chunk edge');
    const top=fieldWorldGroundColor('same-world',0,0,.6,1,lookup);
    const bottom=fieldWorldGroundColor('same-world',0,1,.6,0,lookup);
    assert.ok(top.every((value,i)=>Math.abs(value-bottom[i])<1e-7),
      'adjacent surface rows do not blend');
    assert.notDeepEqual(fieldWorldGroundColor('same-world',0,0,.1,.1,lookup),
      fieldWorldGroundColor('same-world',0,0,.75,.75,lookup));
    assert.deepEqual(fieldWorldGroundColor('same-world',0,0,.25,.25,lookup),
      fieldWorldGroundColor('same-world',0,0,.25,.25,lookup));
  });
  test('V3 water is observed, not artificially smeared onto authoritative land',()=>{
    const land=chunk(0,0,'None',.5,{forestCoverage01:.9});
    const water=chunk(1,0,'Lake',.5);
    const lookup=(x,y)=>[land,water].find(q=>q.x===x&&q.y===y);
    const landEdge=fieldWorldGroundColor('seed',0,0,1,.5,lookup);
    const landInterior=fieldWorldGroundColor('seed',0,0,.99,.5,lookup);
    const waterEdge=fieldWorldGroundColor('seed',1,0,0,.5,lookup);
    assert.ok(Math.abs(landEdge[1]-landInterior[1])<6);
    assert.ok(waterEdge[2]>waterEdge[0]);
    assert.ok(Math.abs(landEdge[1]-waterEdge[1])>12,
      'waterland contrast disappeared into a fictional wetland');
  });
  test('V3 shoreline needs a real Core water-to-land boundary',()=>{
    const lake=chunk(0,0,'Lake',.5),land=chunk(1,0,'None',.5);
    const noShore=()=>{const calls=[],ctx={};
      for(const fn of ['beginPath','moveTo','lineTo','stroke'])ctx[fn]=(...args)=>calls.push([fn,...args]);
      return {ctx,calls};
    };
    const l=noShore();
    paintFieldShore(l.ctx,lake,()=>undefined,(x,y)=>({x,y}),()=>.5,'seed');
    assert.equal(l.calls.length,0,'unknown edge created unsupported coastline');
    const w=noShore();
    paintFieldShore(w.ctx,lake,(x,y)=>x===0&&y===0?lake:
      x===1&&y===0?land:undefined,(x,y)=>({x,y}),()=>.5,'seed');
    assert.equal(w.calls.filter(x=>x[0]==='stroke').length,1);
    const notWater=noShore();
    paintFieldShore(notWater.ctx,land,()=>lake,(x,y)=>({x,y}),()=>.5,'seed');
    assert.equal(notWater.calls.length,0);
  });
  test('V3 terrain draws one continuous bitmap instead of horizontal raster-strip gaps',()=>{
    const oldDocument=globalThis.document;
    const canvasContext={
      createImageData:(w,h)=>({data:new Uint8ClampedArray(w*h*4)}),
      putImageData:()=>{},
      createRadialGradient:()=>({addColorStop:()=>{}}),
    };
    for(const fn of ['beginPath','moveTo','lineTo','stroke','ellipse','fill',
      'quadraticCurveTo','closePath'])canvasContext[fn]=()=>{};
    globalThis.document={createElement:()=>({
      width:80,height:80,getContext:()=>canvasContext,
    })};
    try{
      const actual=chunk(0,0,'None',.5,{grassCoverage01:.6,
        forestCoverage01:.4,rockCoverage01:.2});
      const painter=new FieldTerrainCompositor(),ops=[],ctx={
        drawImage:(...args)=>ops.push(args),
      };
      for(const fn of ['beginPath','moveTo','lineTo','stroke'])
        ctx[fn]=()=>{};
      const result=painter.paint(ctx,'visual-ground',actual,
        (x,y)=>x===0&&y===0?actual:undefined,
        (x,y,h)=>({x:x*180+200,y:y*145+320-(h-.5)*150}),
        ()=>.5);
      assert.equal(result,true);
      assert.equal(ops.length,1,
        'separate height-shifted image slices produced visible horizontal gaps');
      assert.ok(ops[0][3]>100&&ops[0][4]>100);
    }finally{
      if(oldDocument===undefined)delete globalThis.document;
      else globalThis.document=oldDocument;
    }
  });

  test('V3 only real facility positions generate muted occupation ground',()=>{
    const input=[{id:'a',x:100,y:110,kind:'Shelter'},
      {id:'b',x:150,y:112,kind:'FirePit'},
      {id:'c',x:160,y:140,kind:'CultivatedPlot'},
      {id:'broken',x:Infinity,y:0,kind:'WorkSurface'}];
    const actual=fieldObservedCampWashes(input,120);
    assert.equal(actual.length,2);
    assert.ok(actual.every(x=>x.x>=100&&x.x<=150&&x.radius<=120*.47));
    const calls=[],ctx={createRadialGradient:()=>({
      addColorStop:(...args)=>calls.push(['stop',...args]),
    })};
    for(const fn of ['beginPath','ellipse','fill'])ctx[fn]=(...args)=>calls.push([fn,...args]);
    paintFieldCampGround(ctx,input,120);
    assert.equal(calls.filter(x=>x[0]==='ellipse').length,2);
    paintFieldCampGround({}/* Canvas lacking gradients */,input,120);
  });
  test('V3 original twenty field SVGs are all replaced with layered local artwork',()=>{
    const root=resolve(dirname(fileURLToPath(import.meta.url)),'../../public/field');
    const names=['pine','cedar','oak','bush','cliff','rock','flowers',
      'citizen_f','citizen_m','deer','hare','hut','fire','workbench',
      'store','sleeping','furnace','farm','site','flag'];
    for(const name of names){
      const art=readFileSync(resolve(root,name+'.svg'),'utf8');
      assert.ok(art.includes('<svg')&&art.includes('</svg>'),name);
      assert.ok(art.length>2500,name+' is still a placeholder icon');
      assert.ok((art.match(/<path /g)||[]).length>=6,name+' lacks illustrated layers');
      assert.ok(!/https?:\/\//.test(art.replace('http://www.w3.org/2000/svg','')),
        name+' uses an external image/resource');
    }
  });
  test('V3 Core actor/animal/facility counts stay authoritative without generated entities',()=>{
    const land=windowOf([chunk(0,0,'None',.5,{forestCoverage01:.85,
      grassCoverage01:.89,rockCoverage01:.23})],0,0,'strict-core-world');
    land.wildlife={available:true,geographicallyConditioned:true,
      regionalWeatherApplied:true,animals:[]};
    const resident={id:'actual',name:'주민',sex:'Female',alive:true,
      hasPosition:true,gridX:16,gridY:16};
    const prev=JSON.stringify({land,resident});
    const calls=[],ctx={globalAlpha:1};
    for(const fn of ['clearRect','fillRect','beginPath','moveTo','lineTo',
      'closePath','stroke','fill','ellipse','fillText','strokeText','quadraticCurveTo']){
      ctx[fn]=(...args)=>calls.push([fn,...args]);
    }
    const out=paintFieldWorld(ctx,{width:375,height:720,zoom:1,
      centerChunkX:0,centerChunkY:0},
      {terrain:land,residents:[resident],civilization:{available:false},
        centerX:0,centerY:0,zoom:1},
      new ResidentContinuity(),{draw:()=>true});
    assert.equal(out.markers.filter(x=>x.kind==='resident').length,1);
    assert.equal(out.markers.filter(x=>x.kind==='animal').length,0);
    assert.equal(out.markers.filter(x=>x.kind==='trace').length,0);
    assert.equal(JSON.stringify({land,resident}),prev);
    assert.equal(new FieldTerrainCompositor().paint(ctx,'seed',land.chunks[0],
      ()=>undefined,()=>({x:0,y:0}),()=>.5),false);
  });
  test('V3 layered character rig still anchors sleep to actual Core world point',()=>{
    const calls=[],ctx={};
    for(const fn of ['save','restore','translate','scale','rotate','beginPath',
      'ellipse','fill','stroke','moveTo','lineTo','closePath','quadraticCurveTo']){
      ctx[fn]=(...args)=>calls.push([fn,...args]);
    }
    drawFieldResident(ctx,{id:'r-001',name:'관찰자',sex:'Male'},
      120,210,65,100,{facing:'S',pose:'sleep',step:0});
    const translations=calls.filter(x=>x[0]==='translate');
    assert.deepEqual(translations[0],[ 'translate',120,210 ]);
    assert.deepEqual(translations[1],[ 'translate',0,55 ]);
    assert.equal(calls.at(-1)[0],'restore');
  });
}
