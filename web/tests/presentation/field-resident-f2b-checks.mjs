import assert from 'node:assert/strict';

export function fieldResidentF2bChecks({test,source,chunk,windowOf}){
  const {fieldResidentAction,fieldLifeStageScale,drawFieldArtifact}
    =source('render/field-resident-actions.ts');
  const {FieldResidentMotion,drawFieldResident}
    =source('render/field-resident-presentation.ts');
  const {paintFieldWorld}=source('render/field-observer-renderer.ts');
  const {ResidentContinuity}=source('runtime/resident-continuity.ts');

  const p=(overrides={})=>({
    active:true,kind:'Civilization',phase:'Interacting',
    civilizationIntent:'Gather',civilizationResourceNode:'real-resource',
    civilizationMaterial:'Wood',hasTargetGrid:true,
    targetGridX:16,targetGridY:16,contextActionToken:'5',...overrides,
  });
  const r=(presentation,overrides={})=>({
    id:'core-resident',name:'관찰 주민',sex:'Female',alive:true,
    hasPosition:true,gridX:16,gridY:16,presentation,...overrides,
  });
  const tool=(overrides={})=>({
    id:'actual-core-tool',designId:'design-1',creatorId:'core-resident',
    createdMinute:10,durability:1,uses:2,repairs:0,
    cutting:1,digging:0,carrying:0,heating:0,
    parts:[{material:'Flint',shape:1,joint:0,size:1},
      {material:'Wood',shape:0,joint:1,size:2}],
    ...overrides,
  });
  function canvas(){
    const calls=[],ctx={globalAlpha:1};
    for(const method of ['clearRect','fillRect','fillText','strokeText',
      'beginPath','moveTo','lineTo','closePath','fill','stroke',
      'arc','ellipse','rect','quadraticCurveTo','save','restore',
      'translate','scale','rotate'])ctx[method]=(...args)=>calls.push({
        method,args,fill:ctx.fillStyle,
      });
    return {ctx,calls};
  }

  test('F2b gather requires live Core resource, near real target and active interaction',()=>{
    assert.equal(fieldResidentAction(r(p()),16,16).pose,'gather');
    for(const bad of [
      {phase:'Moving'},{active:false},{kind:'Physical'},
      {civilizationResourceNode:''},{civilizationMaterial:'Unknown'},
      {hasTargetGrid:false},{targetGridX:50},
    ]){
      assert.equal(fieldResidentAction(r(p(bad)),16,16).pose,'idle',
        'unverified gather site created fake action: '+JSON.stringify(bad));
    }
    assert.equal(fieldResidentAction(r(p(),{alive:false}),16,16).pose,'idle');
  });

  test('F2b exact real tool parts only appear when Core explicitly attaches valid gather artifact',()=>{
    const actual=tool();
    const directive=p({artifactTool:actual});
    const action=fieldResidentAction(r(directive),16,16);
    assert.equal(action.pose,'gather');
    assert.equal(action.tool,actual);
    assert.equal(fieldResidentAction(r(p()),16,16).tool,null,
      'unheld tool was synthesized');
    const forbidden=[
      {contextActionToken:'0'},
      {contextActionToken:'invalid'},
      {artifactTool:tool({durability:.01})},
      {artifactTool:tool({parts:[]})},
      {artifactTool:tool({parts:[{material:'Wood',shape:9,size:2}]})},
      {artifactTool:tool({parts:[{material:'Wood',shape:0,size:0}]})},
    ];
    for(const invalid of forbidden){
      const bad=fieldResidentAction(r(p({...directive,...invalid})),16,16);
      assert.equal(bad.tool,null,'fabricated tool from '+JSON.stringify(invalid));
    }
    assert.equal(fieldResidentAction(r(p({artifactTool:actual,phase:'Moving'})),16,16).tool,null);
    const {ctx,calls}=canvas();
    drawFieldArtifact(ctx,actual);
    assert.ok(calls.some(c=>c.method==='rect'),'Core shaft part was not drawn');
    assert.ok(calls.some(c=>c.fill==='#65716d'),'Core flint tool material was lost');
    assert.equal(calls.at(-1).method,'restore');
    assert.deepEqual(actual,tool(), 'tool part drawing mutated Core artifact');
  });

  test('F2b building, local crafting, farming and physical needs have separate grounded poses',()=>{
    const build=p({civilizationIntent:'Craft',civilizationResourceNode:undefined,
      facilityId:'built-from-core',facilityKind:'Shelter',facilityAction:'Work'});
    assert.equal(fieldResidentAction(r(build),16,16).pose,'build');
    assert.equal(fieldResidentAction(r({...build,facilityAction:'Repair'}),16,16).pose,'build');
    assert.equal(fieldResidentAction(r({...build,facilityId:'0'}),16,16).pose,'idle');
    assert.equal(fieldResidentAction(r({...build,targetGridX:50}),16,16).pose,'idle');
    const craft=p({civilizationIntent:'Craft',
      civilizationResourceNode:undefined,facilityAction:'None',
      facilityId:'0',hasTargetGrid:false,civilizationTechnique:'RealTechnique'});
    assert.equal(fieldResidentAction(r(craft),16,16).pose,'craft');
    assert.equal(fieldResidentAction(r({...craft,civilizationTechnique:'None'}),16,16).pose,'idle');
    const workspace={...craft,hasTargetGrid:true,facilityId:'work-01',
      facilityKind:'WorkSurface',civilizationTechnique:undefined};
    assert.equal(fieldResidentAction(r(workspace),16,16).pose,'craft');
    for(const action of ['Plant','Water','Tend','Harvest']){
      assert.equal(fieldResidentAction(r({...build,facilityKind:'CultivatedPlot',
        facilityAction:action}),16,16).pose,'tend');
    }
    assert.equal(fieldResidentAction(r({...build,facilityKind:'Shelter',
      facilityAction:'Water'}),16,16).pose,'idle');
    for(const [goal,expected] of [['Sleep','sleep'],['Eat','eat'],
      ['Drink','drink'],['Wash','wash']]){
      const evidence={active:true,phase:'Interacting',kind:'Physical',
        physicalGoal:goal};
      assert.equal(fieldResidentAction(r(evidence),16,16).pose,expected);
      assert.equal(fieldResidentAction(r({...evidence,phase:'Moving'}),16,16).pose,'idle');
    }
    assert.equal(fieldResidentAction(r({active:true,phase:'Interacting',
      kind:'Social',socialIntent:'Talk'}),16,16).pose,'idle');
  });

  test('F2b life stage uses actual Core stage with safe age fallback and no identity mutation',()=>{
    const stageValues={Baby:.48,Toddler:.61,Child:.79,Teen:.93,
      YoungAdult:1,Adult:1,MiddleAge:.98,Elderly:.94};
    for(const [stage,scale] of Object.entries(stageValues))
      assert.equal(fieldLifeStageScale(r(null,{lifeStage:stage})),scale,stage);
    assert.equal(fieldLifeStageScale(r(null,{ageYears:5})),.61);
    assert.equal(fieldLifeStageScale(r(null,{ageYears:73})),.94);
    assert.equal(fieldLifeStageScale(r(null,{ageYears:11,lifeStage:'Adult'})),1);
    assert.equal(fieldLifeStageScale(r(null,{ageYears:NaN})),1);
    assert.equal(fieldLifeStageScale(r(null,{lifeStage:'FutureStage',ageYears:3})),1);
  });

  test('F2b hand, knee and mouth action poses render distinctly without invented inventory',()=>{
    const person=r(null,{genetics:{skinTone:.6,hairPigment:.2}});
    const common={facing:'S',step:.26};
    const signatures=new Map();
    for(const pose of ['idle','gather','build','craft','tend','eat','drink','wash']){
      const {ctx,calls}=canvas();
      drawFieldResident(ctx,person,80,120,65,95,{...common,pose});
      assert.equal(calls.at(0).method,'save');
      assert.equal(calls.at(-1).method,'restore');
      assert.ok(calls.length>25,pose);
      signatures.set(pose,JSON.stringify(calls.slice(0,80)));
    }
    assert.notEqual(signatures.get('gather'),signatures.get('build'));
    assert.notEqual(signatures.get('craft'),signatures.get('tend'));
    assert.notEqual(signatures.get('eat'),signatures.get('idle'));
    const {ctx,calls}=canvas();
    drawFieldResident(ctx,person,80,120,65,95,{
      ...common,pose:'gather',tool:tool(),
    });
    assert.ok(calls.some(c=>c.method==='rect'),
      'actual Core-authored artifact not shown in the resident hand');
    const noItem=canvas();
    drawFieldResident(noItem.ctx,person,80,120,65,95,{
      ...common,pose:'gather',tool:null,
    });
    assert.ok(!noItem.calls.some(c=>c.method==='rect'),
      'display generated a tool without Core evidence');
  });

  test('F2b motion state maps active Core actions and retains real movement priority',()=>{
    const tracker=new FieldResidentMotion();
    const person=r(p());
    const idle=tracker.frame(person,16,16,1000);
    assert.equal(idle.pose,'gather');
    const moved=tracker.frame(person,16.6,16,1110);
    assert.equal(moved.pose,'walk');
    assert.equal(tracker.frame(person,16.6,16,1520).pose,'gather');
    tracker.reset();
    const far=tracker.frame(r(p({targetGridX:55})),16,16,1700);
    assert.equal(far.pose,'idle');
  });

  test('F2b real age-scaled field actor preserves Core footpoint and selection',()=>{
    const terrain=windowOf([chunk(0,0,'None',.5,{forestCoverage01:0})],0,0,'stage');
    const {ctx}=canvas();
    const sprites={draw:()=>true};
    const continuity=new ResidentContinuity();
    const draw=(lifeStage)=>{
      const person=r(null,{lifeStage});
      const result=paintFieldWorld(ctx,{
        width:700,height:450,centerChunkX:0,centerChunkY:0,zoom:1,
      },{
        terrain,residents:[person],civilization:{available:false},
        centerX:0,centerY:0,zoom:1,selectedResidentId:person.id,
      },continuity,sprites);
      return result.markers.find(m=>m.kind==='resident');
    };
    const baby=draw('Baby'),adult=draw('Adult'),elder=draw('Elderly');
    assert.ok(baby.radius<adult.radius);
    assert.ok(elder.radius<adult.radius);
    assert.equal(baby.x,adult.x);
    assert.equal(baby.y,adult.y);
    assert.equal(elder.x,adult.x);
  });
}
