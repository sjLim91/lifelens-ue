import assert from 'node:assert/strict';

export function fieldResidentF2aChecks({test,source,chunk,windowOf}) {
  const {fieldHeading,fieldResidentPalette,FieldResidentMotion,
    drawFieldResident}=source('render/field-resident-presentation.ts');
  const {paintFieldWorld,FieldObserverRenderer}=source('render/field-observer-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');
  const r=(id='resident-alpha',o={})=>({
    id,name:'관찰 주민',alive:true,sex:'Female',
    hasPosition:true,gridX:16,gridY:16,...o,
  });
  const motionFrame=(motion,resident,x,y,t)=>motion.frame(resident,x,y,t);

  test('F2a eight compass facings follow actual grid-space vectors without changing Core coordinates',()=>{
    const points=[[1,0,'E'],[1,1,'SE'],[0,1,'S'],[-1,1,'SW'],
      [-1,0,'W'],[-1,-1,'NW'],[0,-1,'N'],[1,-1,'NE']];
    for(const [dx,dy,expected] of points){
      assert.equal(fieldHeading(dx,dy),expected);
      assert.equal(fieldHeading(dx*2,dy*2),expected);
    }
    assert.equal(fieldHeading(0,0),null);
    assert.equal(fieldHeading(NaN,1),null);
  });

  test('F2a identity palette is stable and genetics-based, without global randomness or Core writes',()=>{
    const first=r('life-guid-001',{genetics:{hairPigment:.05,skinTone:.07,
      buildPotential:.02,heightPotential:.95}});
    const second=r('life-guid-002',{genetics:{hairPigment:.91,skinTone:.92,
      buildPotential:.96,heightPotential:.1}});
    const sourceBefore=JSON.stringify(first);
    const p=fieldResidentPalette(first),q=fieldResidentPalette(second);
    assert.deepEqual(p,fieldResidentPalette(first));
    assert.notEqual(p.skin,q.skin);
    assert.notEqual(p.hair,q.hair);
    assert.notDeepEqual(p,q);
    assert.equal(JSON.stringify(first),sourceBefore);
    assert.ok(p.build>=.9&&p.build<=1.15);
    assert.ok(p.height>=.92&&p.height<=1.11);
    assert.ok(p.outfit.startsWith('#') && p.skinShadow.startsWith('#'));
    assert.notDeepEqual(fieldResidentPalette(r('life-guid-123')),
      fieldResidentPalette(r('life-guid-456')));
  });

  test('F2a an intent/target alone never fabricates walking or body displacement',()=>{
    const motion=new FieldResidentMotion();
    const target={active:true,kind:'Physical',phase:'Moving',
      hasTargetGrid:true,targetGridX:22,targetGridY:16};
    const person=r('walk-test',{presentation:target});
    const original=JSON.stringify(person);
    const first=motionFrame(motion,person,16,16,1000);
    assert.equal(first.facing,'E');
    assert.equal(first.pose,'idle');
    assert.equal(first.step,0);
    assert.equal(motionFrame(motion,person,16,16,1200).pose,'idle');
    assert.equal(JSON.stringify(person),original);
  });

  test('F2a genuine observed movement produces bounded steps then stops at the Core location',()=>{
    const motion=new FieldResidentMotion();
    const person=r('walker');
    motionFrame(motion,person,16,16,1000);
    const frame=motionFrame(motion,person,16.6,16,1100);
    assert.equal(frame.pose,'walk');
    assert.equal(frame.facing,'E');
    assert.equal(frame.step,0);
    assert.equal(motionFrame(motion,person,16.6,16,1220).pose,'walk');
    assert.equal(motionFrame(motion,person,16.6,16,1500).pose,'idle');
    assert.equal(motionFrame(motion,person,16.6,16,5000).pose,'idle');
    // A fast-forward or position refresh jump has no interpolated fake steps.
    const fast=motionFrame(motion,person,30,30,5050);
    assert.equal(fast.pose,'idle');
    assert.equal(motionFrame(motion,person,30,30,5070).pose,'idle');
    motion.reset();
    assert.equal(motion.size,0);
    assert.equal(motionFrame(motion,person,50,50,5100).pose,'idle');
  });

  test('F2a Core-reported sleep/work pose is not inferred from idle or unsupported behavior',()=>{
    const sleep=r('sleep',{presentation:{active:true,phase:'Interacting',
      kind:'Physical',physicalGoal:'Sleep'}});
    const work=r('work',{presentation:{active:true,phase:'Interacting',
      kind:'Civilization',civilizationIntent:'Craft',facilityAction:'Work',
      facilityId:'real-facility',hasTargetGrid:true,targetGridX:5,targetGridY:5}});
    const absent=r('nope',{presentation:{active:false,phase:'Interacting',
      kind:'Civilization'}});
    const tracker=new FieldResidentMotion();
    assert.equal(tracker.frame(sleep,5,5,100).pose,'sleep');
    assert.equal(tracker.frame(work,5,5,100).pose,'build');
    assert.equal(tracker.frame(absent,5,5,100).pose,'idle');
    tracker.prune([sleep]);
    assert.equal(tracker.size,1);
  });

  function visualContext(){
    const calls=[];
    const ctx={globalAlpha:1};
    for(const name of ['save','restore','translate','scale','rotate',
      'clearRect','fillRect','fillText','strokeText','beginPath','moveTo',
      'lineTo','quadraticCurveTo','closePath','fill','stroke','arc',
      'ellipse','strokeRect']) ctx[name]=(...args)=>calls.push({
        name,args,fillStyle:ctx.fillStyle,strokeStyle:ctx.strokeStyle,
      });
    return {ctx,calls};
  }

  test('F2a actual front/back/side and walking/working/sleeping art use different poses',()=>{
    const x=r('art-guid',{genetics:{skinTone:.6,hairPigment:.2}});
    const palette=fieldResidentPalette(x);
    const actions=['idle','walk','work','sleep'];
    const views={};
    for(const facing of ['N','NE','E','SE','S','SW','W','NW']){
      const {ctx,calls}=visualContext();
      drawFieldResident(ctx,x,70,95,50,75,{facing,pose:'idle',step:0});
      const faceMarks=calls.filter(c=>c.name==='ellipse'&&c.args[2]<4);
      views[facing]={
        faceMarks:faceMarks.length,
        ellipse:calls.filter(c=>c.name==='ellipse').length,
        translate:calls.filter(c=>c.name==='translate').length,
      };
      assert.equal(calls[0].name,'save');
      assert.equal(calls.at(-1).name,'restore');
      assert.ok(calls.some(c=>c.fillStyle===palette.outfit));
      assert.ok(calls.some(c=>c.fillStyle===palette.hair));
    }
    assert.ok(views.N.faceMarks<views.S.faceMarks,
      'back and front are still the same face icon');
    assert.ok(views.E.faceMarks<views.S.faceMarks,
      'profile must not have two forward eyes');
    for(const pose of actions){
      const {ctx,calls}=visualContext();
      drawFieldResident(ctx,x,70,95,50,75,{facing:'S',pose,step:.3});
      assert.ok(calls.length>20,pose);
      assert.equal(calls.at(-1).name,'restore');
    }
  });

  test('F2a live field renderer draws unique Core actor not a duplicated decorative citizen sprite',()=>{
    const terrain=windowOf([chunk(0,0,'None',.5,{forestCoverage01:0})],
      0,0,'live-characters');
    const resident=r('render-test',{gridX:16,gridY:16});
    const {ctx,calls}=visualContext();
    const drawn=[];
    const painter={draw:(_ctx,name)=>{drawn.push(name);return true;}};
    const continuity=new ResidentContinuity();
    const motion=new FieldResidentMotion();
    const options={
      terrain,residents:[resident],civilization:{available:false},
      centerX:0,centerY:0,zoom:1,selectedResidentId:resident.id,
    };
    const before=JSON.stringify(options);
    const first=paintFieldWorld(ctx,{
      width:700,height:450,zoom:1,centerChunkX:0,centerChunkY:0,
    },options,continuity,painter,motion,1000);
    assert.equal(first.result,undefined);
    assert.equal(first.markers.filter(m=>m.kind==='resident').length,1);
    assert.equal(first.sprites.citizen_f,1);
    assert.equal(drawn.includes('citizen_f'),false,
      'full actor portrait was drawn instead of individual F2 puppet');
    assert.ok(calls.some(c=>c.name==='scale'));
    assert.equal(JSON.stringify(options),before,'field motion mutated Core observation');
    const selectedRing=calls.filter(c=>c.name==='stroke');
    assert.ok(selectedRing.length>=1);
  });

  test('F2a renderer lifecycle resets actor motion on new world and does not change field selection',()=>{
    const {ctx}=visualContext();
    const canvas={
      width:700,height:450,
      getContext:()=>ctx,
      getBoundingClientRect:()=>({left:0,top:0,width:700,height:450}),
    };
    const renderer=new FieldObserverRenderer(canvas,new ResidentContinuity(),()=>{});
    try {
      const terrain=windowOf([chunk(0,0,'None',.5)],0,0,'motion-seed');
      renderer.draw({terrain,residents:[r('actor')],civilization:{available:false},
        centerX:0,centerY:0,zoom:1,selectedResidentId:'actor'});
      const p=source('render/field-projection.ts').createFieldProjection({
        width:700,height:450,zoom:1,centerChunkX:0,centerChunkY:0,
      }).gridPoint(16,16,.5);
      assert.equal(renderer.pickResident(p.x,p.y-8),'actor');
      renderer.resetMotion();
      assert.equal(renderer.pickResident(p.x,p.y-8),'actor');
    } finally {renderer.dispose();}
  });
}
