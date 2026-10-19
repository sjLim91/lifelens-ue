import assert from 'node:assert/strict';
import { source } from './compile.mjs';

const { ResidentAIController, LIVE_AI_BUDGET, liveActionableCognitiveRequest,
  residentAIObservedTrigger, residentAIExecutionBudget, BROWSER_MODEL_RETRY_MS } =
  await source('cognition/live-resident-ai.ts');
const tick = () => new Promise((resolve) => setTimeout(resolve, 0));

{
  const browser=residentAIExecutionBudget({name:'resident-browser-webgpu'});
  const native=residentAIExecutionBudget({name:'local-openai-compatible'});
  assert(browser.timeoutMs >= 20000,
    'GPU generation cannot inherit a 6-second network cutoff');
  assert(browser.timeoutMs > native.timeoutMs);
  assert.equal(browser.staleAfterSimulationMinutes,
    native.staleAfterSimulationMinutes,
    'slower hardware must never extend the authoritative Core world age');
  assert.equal(browser.maxConcurrent,1);
  assert.equal(native.maxConcurrent,1);
  assert.equal(native.timeoutMs,LIVE_AI_BUDGET.timeoutMs);
}

function fixture(provider, browserReasoningFactory) {
  let wall = 100;
  const world = {
    worldSeed: '781', populationSeed: '120', generationVersion: 4,
    minute: 480,
  };
  const residents = [
    { id: '1', alive: true, name: '서윤' },
    { id: '2', alive: true, name: '하린' },
  ];
  const contexts = [];
  const submitted = [];
  const runtime = {
    worldOverview: () => ({ ...world }),
    cognitiveRequest: (actor) => {
      const context = {
        actor, minute: world.minute, trigger: 'Reflection',
        needs: { hunger: actor === '1' ? .2 : .7 },
        personality: { empathy: actor === '1' ? .9 : .1 },
        emotion: { joy: .3 },
        memories: [{
          who: actor, what: actor === '1' ? '직접 목격' : '전해 들음',
          where: '출생지', minute: 472, recallScore: .7,
          confidence: .8, importance: .6, emotionValence: .3,
          emotionIntensity: .4,
          tags: ['action_experience','Civilization:Gather:PlantFood','failure'],
          source: actor === '1' ? 'DirectWitness' : 'ToldByOther',
          witnessed: actor === '1',
        }],
        beliefs: [], relationships: [],
        allowedIntents: ['CooperateWithResident'],
      };
      contexts.push(structuredClone(context));
      return context;
    },
    submitLocalCognitiveProposal: (request, proposal, seed) => {
      submitted.push({ request, proposal, seed });
      return seed === world.worldSeed;
    },
  };
  const controller = new ResidentAIController(() => wall, browserReasoningFactory);
  controller.attach(runtime);
  if (provider) controller.setProvider(provider);
  return {
    controller, contexts, submitted, residents, world, runtime,
    advance: () => { wall += LIVE_AI_BUDGET.wallCadenceMs; world.minute += 15; },
  };
}

const provider = {
  name: 'local-fake',
  async reason(request) {
    return {
      actor: request.actor, intent: 'CooperateWithResident', priority: .9,
      targetResident: request.actor === '1' ? '2' : '1',
      rationale: '관계 개선 시도',
    };
  },
};

{
  const f = fixture();
  assert.equal(f.controller.setEnabled(true), false,
    'no local model means no AI mutation');
  f.controller.dispose();
}

{
  const f = fixture(provider);
  assert.equal(f.controller.setEnabled(true), true);
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.submitted.length, 1);
  assert.equal(f.submitted[0].request.actor, '1');
  assert.equal(f.submitted[0].seed, '781');
  assert.equal(f.controller.getSnapshot().accepted, 1);
  assert.deepEqual(f.controller.getSnapshot().byResident['1'].latestPersonalExperience, {
    minute: 472, what: '직접 목격', kind: 'action',
  });

  f.advance();
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.submitted.length, 2);
  assert.equal(f.submitted[1].request.actor, '2');
  assert.equal(f.contexts[0].personality.empathy, .9);
  assert.equal(f.contexts[1].personality.empathy, .1);
  assert.notEqual(f.contexts[0].memories[0].what,
    f.contexts[1].memories[0].what);
  assert.equal(f.controller.getSnapshot().accepted, 2);
  assert.equal(f.controller.getSnapshot().byResident['2'].status, 'recorded');
  assert.equal(f.controller.getSnapshot().byResident['2'].latestPersonalExperience,
    null, 'hearsay must never appear as a firsthand action experience');

  f.controller.suspend(true);
  f.advance();
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.submitted.length, 2, 'no model mutation during fast forward');
  f.controller.dispose();
}

{
  let finish;
  const f = fixture({
    name: 'deferred',
    reason: () => new Promise((resolve) => {
      finish = resolve;
    }),
  });
  f.controller.setEnabled(true);
  f.controller.observe(f.world, f.residents);
  await tick();
  f.world.worldSeed = 'replaced';
  f.controller.observe(f.world, f.residents);
  finish({
    actor: '1', intent: 'CooperateWithResident', priority: .9,
    targetResident: '2', rationale: '이전 세계',
  });
  await tick();
  assert.equal(f.submitted.length, 0,
    'stale inference must never be sent into replacement Core');
  f.controller.dispose();
}

{
  const f = fixture({
    name: 'invalid',
    reason: async (request) => ({
      actor: request.actor, intent: 'InventNewResource',
      targetResident: null, priority: .9, rationale: '없던 것 생성',
    }),
  });
  f.controller.setEnabled(true);
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.submitted.length, 0);
  assert.equal(f.controller.getSnapshot().rejected, 1);
  f.controller.dispose();
}
{
  const f = fixture(provider);
  f.runtime.cognitiveRequest = () => {
    throw new Error('Core simulation replaced during observation');
  };
  f.controller.setEnabled(true);
  // The render loop calls observe() without awaiting the async sample.
  // A transient Core failure must be reported, never unhandled.
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.submitted.length, 0);
  assert.equal(f.controller.getSnapshot().pendingActor, null);
  assert.equal(f.controller.getSnapshot().accepted, 0);
  assert.equal(f.controller.getSnapshot().rejected, 1);
  assert.equal(f.controller.getSnapshot().lastDecision.status, 'error');

  f.runtime.cognitiveRequest = actor => ({
    actor, minute: f.world.minute, trigger: 'Reflection',
    needs: { hunger: .2 }, personality: { empathy: .7 },
    emotion: { joy: .2 }, memories: [], beliefs: [], relationships: [],
    allowedIntents: ['CooperateWithResident'],
  });
  f.advance();
  await f.controller.sample('1');
  assert.equal(f.submitted.length, 1, 'recovered source can submit later');
  f.controller.dispose();
}

{
  const f = fixture(provider);
  for (let id = 3; id <= 40; id++) {
    f.residents.push({ id: String(id), alive: true, name: String(id) });
  }
  f.controller.setEnabled(true);
  for (let i = 0; i < f.residents.length; i++) {
    f.controller.observe(f.world, f.residents);
    await tick();
    f.advance();
  }
  assert.equal(f.submitted.length, 40,
    'all 40 living residents must receive an independent AI slot');
  assert.equal(new Set(f.submitted.map(x => x.request.actor)).size, 40,
    'the first 32 residents must not monopolize model access');
  assert.equal(f.controller.getSnapshot().byResident['40'].status, 'recorded');
  assert(Object.keys(f.controller.getSnapshot().byResident).length
    <= LIVE_AI_BUDGET.maxResidents,
    'status/history must remain bounded even as populations grow');
  f.controller.dispose();
}

{
  // Core cannot authorize free-local cognitive proposals for residents who
  // require direct care. Such residents must not consume the shared GPU/CPU
  // inference budget while new generations are being born.
  const f = fixture(provider);
  f.residents[0].lifeStage = 'Baby';
  f.residents[1].lifeStage = 'Toddler';
  f.controller.setEnabled(true);
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.contexts.length, 0,
    'two dependent residents must not trigger expensive model requests');
  assert.equal(f.submitted.length, 0);
  assert.equal(f.controller.getSnapshot().rejected, 0,
    'ineligible residents are not failed strategic attempts');

  // Newly autonomous residents become eligible without a game reset.
  f.residents[1].lifeStage = 'Child';
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.submitted.length, 1);
  assert.equal(f.submitted[0].request.actor, '2',
    'child must get the first available cognitive slot');
  f.advance();
  f.residents[0].lifeStage = 'YoungAdult';
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.submitted.length, 2);
  assert.equal(f.submitted[1].request.actor, '1',
    'resident joining the autonomous pool receives its next fair slot');
  assert.equal(f.controller.getSnapshot().accepted, 2);
  f.controller.dispose();
}

{
  const f = fixture();
  const original = f.runtime.cognitiveRequest('1');
  original.allowedIntents = [
    'ImproveFoodSecurity', 'TradeWithResident', 'CareForDependent',
    'ImproveShelter', 'CooperateWithResident', 'ResolveConflict',
  ];
  const safe = liveActionableCognitiveRequest(original);
  assert.deepEqual(safe.allowedIntents, [
    'ImproveFoodSecurity', 'CooperateWithResident', 'ResolveConflict',
  ]);
  assert.equal(original.allowedIntents.length, 6,
    'the authoritative Core request must not be modified in place');
  f.controller.dispose();
}

{
  // A free local model must not receive journal-only strategies that appear
  // to make progress but can never execute a Core action.
  const received = [];
  const f = fixture({
    name: 'local-only-executable',
    reason: async (request) => {
      received.push([...request.allowedIntents]);
      return {
        actor: request.actor,
        intent: 'ImproveFoodSecurity',
        targetResident: null, priority: .9, rationale: '실제 식량 채집',
      };
    },
  });
  const read = f.runtime.cognitiveRequest;
  f.runtime.cognitiveRequest = (actor) => {
    const request = read(actor);
    request.allowedIntents = [
      'TeachKnowledge', 'TradeWithResident', 'MigrateHousehold',
      'ImproveFoodSecurity', 'CareForDependent',
    ];
    return request;
  };
  f.controller.setEnabled(true);
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.deepEqual(received, [['ImproveFoodSecurity']]);
  assert.equal(f.submitted.length, 1);
  assert.equal(f.submitted[0].proposal.intent, 'ImproveFoodSecurity');
  assert.deepEqual(f.contexts[0].allowedIntents, [
    'CooperateWithResident',
  ], 'underlying fixture Core observations are not mutated');
  f.controller.dispose();
}

{
  // When Core advertises ONLY future, non-executable intents, refuse to
  // make a costly model call. Shadow can still study them read-only.
  let calls = 0;
  const f = fixture({
    name: 'inert-intents',
    reason: async () => {
      calls += 1;
      return null;
    },
  });
  const read = f.runtime.cognitiveRequest;
  f.runtime.cognitiveRequest = (actor) => ({
    ...read(actor),
    allowedIntents: ['TeachKnowledge', 'MigrateHousehold', 'ImproveShelter'],
  });
  f.controller.setEnabled(true);
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(calls, 0);
  assert.equal(f.submitted.length, 0);
  assert.equal(f.controller.getSnapshot().pendingActor, null);
  assert.equal(f.controller.getSnapshot().rejected, 0);
  f.controller.dispose();
}


{
  // Browser audit is a truthful observation window, not a model's claim of
  // action completion. Baseline already-known beliefs must never be reported
  // as successes that occurred during the audited period.
  const { ResidentAISessionAudit } =
    await source('cognition/resident-ai-session-audit.ts');
  const audit = new ResidentAISessionAudit();
  const world = {
    worldSeed: '42', populationSeed: '7', generationVersion: 4,
    minute: 480, livingResidents: 1,
  };
  const civilian = 'experienced-action-outcome:Civilization:Gather:PlantFood';
  const physical = 'experienced-action-outcome:Eat';
  const base = [{ id: '1', alive: true, beliefs: [
    { subject: '0', proposition: civilian, supportCount: 2, contradictionCount: 0 },
  ] }];
  assert.equal(audit.begin(world, 'baseline', null), true);
  audit.observe(world, base, 'baseline');
  assert.equal(audit.snapshot().coreCivilizationSuccessEvidenceDelta, 0);
  const next = { ...world, minute: 487 };
  audit.observe(next, [{ id: '1', alive: true, beliefs: [
    { subject: '0', proposition: civilian, supportCount: 3, contradictionCount: 2 },
    { subject: '0', proposition: physical, supportCount: 1, contradictionCount: 0 },
  ] }], 'baseline');
  const report = audit.snapshot();
  assert.equal(report.coreCivilizationSuccessEvidenceDelta, 1);
  assert.equal(report.coreCivilizationFailureEvidenceDelta, 2);
  assert.equal(report.corePhysicalSuccessEvidenceDelta, 1);
  assert.equal(report.modelRequests, 0);
  assert.equal(report.mode, 'baseline');
  assert.equal(JSON.stringify(report).includes('actor'), false,
    'export must not include personal memory text, resident IDs, or model rationale');
  audit.observe({ ...world, minute: 489 }, base, 'local-ai');
  assert.equal(audit.snapshot().endedReason, 'mode-changed');
  assert.equal(audit.snapshot().comparableWithinObservedWindow, false);

  assert.equal(audit.begin(next, 'local-ai', 'test-model'), true);
  audit.observe(next, base, 'local-ai');
  audit.inference({ memories: [{
    source: 'DirectWitness', witnessed: true,
    tags: ['action_experience', 'Civilization:Gather:PlantFood', 'failure'],
  }] });
  audit.result('recorded');
  assert.equal(audit.snapshot().requestsWithFirsthandExperience, 1);
  assert.equal(audit.snapshot().decisions.recorded, 1);
  audit.observe({ ...world, minute: 750 }, base, 'local-ai');
  assert.equal(audit.snapshot().coverageGap, true);
  assert.equal(audit.snapshot().comparableWithinObservedWindow, false);
  audit.finish('fast-forward');
  assert.equal(audit.snapshot().endedReason, 'fast-forward');
}

{
  // The real live controller exposes explicit, mode-locked observation
  // windows. Disabling AI closes the report without erasing the evidence.
  const f = fixture(provider);
  f.controller.setEnabled(true);
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.controller.beginAuditSession(), true);
  f.advance();
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.equal(f.controller.getAuditReport().mode, 'local-ai');
  assert.equal(f.controller.getAuditReport().modelRequests, 1);
  assert.equal(f.controller.getAuditReport().decisions.recorded, 1);
  f.controller.setEnabled(false);
  assert.equal(f.controller.getAuditReport().endedReason, 'stopped');
  assert.equal(f.controller.getAuditReport().decisions.recorded, 1);
  f.controller.dispose();
}

{
  // A matching seed alone is insufficient. Require the same clock window,
  // confirmed Core approvals, observable Core belief data and no gaps.
  const { compareResidentAISessionReports } =
    await source('cognition/resident-ai-session-comparison.ts');
  const { ResidentAISessionAudit } =
    await source('cognition/resident-ai-session-audit.ts');
  const audit = new ResidentAISessionAudit();
  const world = { worldSeed:'81', populationSeed:'15', generationVersion:4, minute:480 };
  audit.begin(world, 'baseline', null);
  const residents = [{ id:'1', alive:true, beliefs:[] }];
  audit.observe(world, residents, 'baseline');
  audit.observe({ ...world, minute:510 }, residents, 'baseline');
  const baseline = audit.snapshot();
  const candidate = {
    ...baseline, mode:'local-ai', modelName:'offline-local-model',
    modelRequests:2, requestsWithFirsthandExperience:1,
    decisions:{...baseline.decisions, recorded:1},
    coreCivilizationSuccessEvidenceDelta:4,
    coreCivilizationFailureEvidenceDelta:2,
  };
  const valid = compareResidentAISessionReports(baseline, candidate);
  assert.equal(valid.comparable, true);
  assert.equal(valid.civilizationSuccessDifference, 4);
  assert.equal(valid.civilizationFailureDifference, 2);
  assert.equal(valid.localModelRequests, 2);
  assert.equal(valid.localCoreApprovals, 1);
  const mismatched = compareResidentAISessionReports(baseline, {
    ...candidate, startMinute:481,
  });
  assert.equal(mismatched.comparable, false);
  assert(mismatched.issues.some(issue => issue.includes('시뮬레이션')));
  const unobserved = compareResidentAISessionReports(baseline, {
    ...candidate, readableEvidenceResidentSamples:0,
    coverageGap:true,
  });
  assert.equal(unobserved.comparable, false);
  assert(unobserved.issues.some(issue => issue.includes('연속성')));
  assert.equal(compareResidentAISessionReports({}, candidate).comparable,false);
  assert.equal(compareResidentAISessionReports(baseline, {
    ...candidate, modelName:{toString(){ throw Error('unsafe input');}},
  }).comparable, false, 'malformed imported data must never be trusted');
}


{
  // Only the actor's firsthand Core-written source-loss memory can switch
  // the model to a ResourceScarcity context; Core still re-observes authority.
  const newFailure = {
    who: '1', source: 'DirectWitness', witnessed: true,
    minute: 479, recallScore: .2, confidence: 1,
    importance: .2, emotionValence: -.3, emotionIntensity: .2,
    what: '실제 문명 행동 Civilization:Gather:PlantFood 수행을 시도했지만 실패했다 (직접 확인한 원인: source-exhausted)',
    where: '내가 직접 시도한 행동',
    tags: ['action_experience','Civilization:Gather:PlantFood','failure'],
  };
  const base = {
    actor:'1', minute:480, trigger:'Reflection',
    allowedIntents:['ImproveFoodSecurity','ExploreOpportunity'],
    memories:[newFailure],
  };
  assert.equal(residentAIObservedTrigger(base), 'ResourceScarcity');
  assert.equal(residentAIObservedTrigger({
    ...base, memories:[{...newFailure, source:'ToldByOther', witnessed:false}],
  }), 'Reflection');
  assert.equal(residentAIObservedTrigger({
    ...base, memories:[{...newFailure, what:'source-exhausted'}],
  }), 'Reflection', 'local model text cannot forge source-loss provenance');
  assert.equal(residentAIObservedTrigger({
    ...base, minute:2000,
  }), 'Reflection', 'expired direct experience cannot trigger scarcity');
  assert.equal(residentAIObservedTrigger({
    ...base, memories:[newFailure, {
      ...newFailure, minute:480, tags:[
        'action_experience','Civilization:Gather:PlantFood','success',
      ], what:'실제 문명 행동 Civilization:Gather:PlantFood 수행에 성공했다',
    }],
  }), 'Reflection', 'new firsthand success supersedes older resource loss');
  assert.equal(residentAIObservedTrigger({
    ...base, memories:[{
      ...newFailure, tags:['action_experience','Civilization:Gather:Water','failure'],
      what:'실제 문명 행동 Civilization:Gather:Water 수행을 시도했지만 실패했다 (직접 확인한 원인: container-missing)',
    }],
  }), 'Reflection', 'missing vessel is not depleted water');

  const observedTriggers=[];
  const f=fixture({
    name:'evidence-grounded-local',
    reason: async (request)=>{
      observedTriggers.push(request.trigger);
      return {
        actor:request.actor, intent:'ExploreOpportunity',
        priority:.9, targetResident:null, rationale:'새로운 자원 탐색',
      };
    },
  });
  const read=f.runtime.cognitiveRequest;
  f.runtime.cognitiveRequest=(actor,trigger)=>{
    const request=read(actor);
    request.trigger=trigger;
    request.allowedIntents=['ImproveFoodSecurity','ExploreOpportunity'];
    request.memories=[{...newFailure}];
    return request;
  };
  f.controller.setEnabled(true);
  f.controller.observe(f.world,f.residents);
  await tick();
  assert.deepEqual(observedTriggers,['ResourceScarcity']);
  assert.equal(f.contexts.length,2, 'Core is re-read with verified event trigger');
  assert.equal(f.submitted.length,1);
  assert.equal(f.submitted[0].request.trigger,'ResourceScarcity');
  assert.equal(f.submitted[0].proposal.intent,'ExploreOpportunity');
  assert.equal(f.controller.getSnapshot().accepted,1);
  f.controller.dispose();
}

{
  // The SAME AI reflects when TWO independently witnessed executions of
  // the same real Core action fail, rather than waiting for a scripted NPC.
  const failure = {
    who:'1', source:'DirectWitness', witnessed:true, minute:478,
    recallScore:.6, confidence:1, importance:.6,
    emotionValence:-.4,emotionIntensity:.7,
    what:'실제 문명 행동 Civilization:Gather:PlantFood 수행을 시도했지만 실패했다 (직접 확인한 원인: access-unavailable)',
    where:'Core 직접 행동',
    tags:['action_experience','Civilization:Gather:PlantFood','failure'],
  };
  const second={...failure,minute:479};
  const base={actor:'1',minute:480,trigger:'Reflection',
    allowedIntents:['ExploreOpportunity','ImproveFoodSecurity'],
    memories:[failure,second]};
  assert.equal(residentAIObservedTrigger(base),'RepeatedFailure');
  assert.equal(residentAIObservedTrigger({
    ...base,memories:[second,failure],
  }),'RepeatedFailure','memory insertion order does not change evidence');
  assert.equal(residentAIObservedTrigger({
    ...base,memories:[failure,{...second,minute:478}],
  }),'Reflection','duplicate same tick is not two separate attempts');
  assert.equal(residentAIObservedTrigger({
    ...base,memories:[failure,{...second,source:'ToldByOther',witnessed:false}],
  }),'Reflection','secondhand news is not firsthand failure');
  assert.equal(residentAIObservedTrigger({
    ...base,memories:[failure,{...second,who:'2'}],
  }),'Reflection','other residents cannot rewrite this person\'s experience');
  assert.equal(residentAIObservedTrigger({
    ...base,memories:[failure,second,{
      ...second,minute:480,tags:[
        'action_experience','Civilization:Gather:PlantFood','success',
      ],what:'실제 문명 행동 Civilization:Gather:PlantFood 수행에 성공했다',
    }],
  }),'Reflection','latest real success resets failure streak');
  assert.equal(residentAIObservedTrigger({...base,minute:2000}),
    'Reflection','one-day-old attempts expire');
  assert.equal(residentAIObservedTrigger({
    ...base,memories:[failure,second,{...second,minute:480,
      what:'실제 문명 행동 Civilization:Gather:PlantFood 수행을 시도했지만 실패했다 (직접 확인한 원인: source-removed)',
    }],
  }),'ResourceScarcity','physically removed source is stronger direct evidence');

  const triggers=[];
  const f=fixture({name:'learn-from-real-failure',reason:async request=>{
    triggers.push(request.trigger);
    return {actor:request.actor,intent:'ExploreOpportunity',priority:.8,
      targetResident:null,rationale:'직접 실패한 경로 대신 새 접근을 찾는다'};
  }});
  const read=f.runtime.cognitiveRequest;
  f.runtime.cognitiveRequest=(actor,trigger)=>{
    const context=read(actor);
    context.trigger=trigger;
    context.allowedIntents=['ImproveFoodSecurity','ExploreOpportunity'];
    context.memories=[failure,second];
    return context;
  };
  f.controller.setEnabled(true);
  f.controller.observe(f.world,f.residents);
  await tick();
  assert.deepEqual(triggers,['RepeatedFailure']);
  assert.equal(f.contexts.length,2,'Core must re-authorize factual reflection');
  assert.equal(f.submitted[0].request.trigger,'RepeatedFailure');
  assert.equal(f.submitted[0].proposal.intent,'ExploreOpportunity');
  f.controller.dispose();
}

// Production auto-start must require no human click or manual model name.
// A free localhost model may appear later and joins the SAME resident agency.
{
  const previousFetch=globalThis.fetch;
  const seen=[];
  let online=false;
  globalThis.fetch=async (url,init) => {
    const address=String(url);
    seen.push({address,method:init?.method});
    assert(address.startsWith('http://127.0.0.1:'));
    if (!online) throw Error('local model absent');
    if (init?.method==='GET') return new Response(JSON.stringify({
      data:[{id:'free-local-reasoner'}],
    }),{status:200});
    const request=JSON.parse(init.body).messages[1].content;
    const actor=JSON.parse(request).actor;
    return new Response(JSON.stringify({choices:[{message:{content:JSON.stringify({
      actor, intent:'CooperateWithResident', priority:.85,
      targetResident:actor==='1'?'2':'1', rationale:'실제 관계를 확인한다',
    })}}]}),{status:200});
  };
  try {
    const f=fixture();
    f.controller.startAutomatically();
    await tick();
    await tick();
    assert.equal(f.controller.getSnapshot().enabled,true);
    assert.equal(f.controller.getSnapshot().automatic,true);
    assert.equal(f.controller.getSnapshot().configuration,null);
    assert.equal(f.submitted.length,0,'no invented language response');
    online=true;
    for(let i=0;i<13;++i) f.advance();
    f.controller.observe(f.world,f.residents);
    await tick();
    await tick();
    assert.equal(f.controller.getSnapshot().configuration?.model,
      'free-local-reasoner');
    f.controller.observe(f.world,f.residents);
    await tick();
    assert.equal(f.submitted.length,1,
      'auto-started resident must reason without a manual start button');
    assert.equal(f.submitted[0].proposal.actor,'1');
    assert(seen.every(x=>x.address.includes('127.0.0.1')));
    f.controller.dispose();
  } finally {
    globalThis.fetch=previousFetch;
  }
}

{
  // An initial mobile GPU or cached-model failure must not permanently
  // strand the single resident mind in 'waiting'. Recovery is automatic,
  // bounded and does not invent local or paid inference.
  const previousFetch = globalThis.fetch;
  globalThis.fetch = async () => { throw Error('native model absent'); };
  try {
    let loads = 0;
    const f = fixture(undefined, async () => {
      loads += 1;
      return loads === 1 ? null : provider;
    });
    f.controller.startAutomatically();
    await tick();
    await tick();
    assert.equal(loads,1,'one initial on-device load attempt');
    assert.equal(f.controller.getSnapshot().enabled,true);
    assert.equal(f.controller.getSnapshot().configuration,null);
    for(let i=0;i<13;++i) f.advance();
    f.controller.observe(f.world,f.residents);
    await tick();
    await tick();
    assert.equal(loads,1,'no aggressive download retry after one minute');
    for(let i=0;i<48;++i) f.advance();
    f.controller.observe(f.world,f.residents);
    await tick();
    await tick();
    assert(BROWSER_MODEL_RETRY_MS>=300000);
    assert.equal(loads,2,'recover after the bounded cooldown');
    assert.equal(f.controller.getSnapshot().configuration?.model,
      'Qwen2.5-0.5B-Instruct-q4f16_1-MLC');
    assert.equal(f.controller.getSnapshot().automatic,true);
    f.controller.observe(f.world,f.residents);
    await tick();
    assert.equal(f.submitted.length,1,
      'same resident automatically uses recovered language reasoning');
    f.controller.dispose();
  } finally {
    globalThis.fetch=previousFetch;
  }
}

{
  // A stale, long-running WebGPU load cannot attach its old provider
  // after a new Core source replaces the previous attachment.
  const previousFetch=globalThis.fetch;
  globalThis.fetch=async () => { throw Error('native model absent'); };
  try {
    const pending=[];
    let obsoleteUnloads=0;
    const staleProvider={...provider,dispose:async()=>{obsoleteUnloads++;}};
    const f=fixture(undefined, () => new Promise(resolve => pending.push(resolve)));
    f.controller.startAutomatically();
    await tick();
    await tick();
    assert.equal(pending.length,1);
    f.controller.attach(f.runtime);
    await tick();
    await tick();
    assert.equal(pending.length,2);
    pending[0](staleProvider);
    await tick();
    assert.equal(obsoleteUnloads,1,
      'orphaned prior-world WebGPU model must release its GPU memory');
    assert.equal(f.controller.getSnapshot().configuration,null,
      'old world WebGPU completion cannot install stale model');
    pending[1](null);
    await tick();
    f.controller.dispose();
  } finally {
    globalThis.fetch=previousFetch;
  }
}

{
  // A successful in-browser model is NOT an HTTP localhost configuration.
  // Reattaching Core after a world/server refresh previously threw when
  // rebuildScheduler tried LocalCognitionProvider('browser-webgpu').
  const previousFetch=globalThis.fetch;
  globalThis.fetch=async () => { throw Error('no local model server'); };
  try {
    let loaded=0;
    const f=fixture(undefined,async () => {
      loaded++;
      return provider;
    });
    f.controller.startAutomatically();
    await tick();
    await tick();
    assert.equal(loaded,1);
    assert.equal(f.controller.getSnapshot().configuration?.baseUrl,
      'browser-webgpu');
    assert.doesNotThrow(() => f.controller.attach(f.runtime),
      'reattach must not mistake in-process GPU engine for localhost URL');
    await tick();
    await tick();
    assert.equal(loaded,2,'reattach should automatically restore browser AI');
    assert.equal(f.controller.getSnapshot().configuration?.baseUrl,
      'browser-webgpu');
    assert.equal(f.controller.getSnapshot().automatic,true);
    f.controller.observe(f.world,f.residents);
    await tick();
    assert.equal(f.submitted.length,1,
      'reconnected resident must regain local language reasoning');
    f.controller.dispose();
  } finally {
    globalThis.fetch=previousFetch;
  }
}

console.log('Live Resident AI: independent actor context, Core handoff, cancellation PASS');
