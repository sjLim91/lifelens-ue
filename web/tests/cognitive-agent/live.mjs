import assert from 'node:assert/strict';
import { source } from './compile.mjs';

const { ResidentAIController, LIVE_AI_BUDGET, liveActionableCognitiveRequest } =
  await source('cognition/live-resident-ai.ts');
const tick = () => new Promise((resolve) => setTimeout(resolve, 0));

function fixture(provider) {
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
  const controller = new ResidentAIController(() => wall);
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
  const submit = f.runtime.submitLocalCognitiveProposal;
  let originalCoreRequest;
  f.runtime.cognitiveRequest = (actor) => {
    const request = read(actor);
    request.allowedIntents = [
      'TeachKnowledge', 'TradeWithResident', 'MigrateHousehold',
      'ImproveFoodSecurity', 'CareForDependent',
    ];
    originalCoreRequest = request;
    return request;
  };
  // Core fingerprints the entire original request (including all intents).
  // Rejecting a filtered copy here reproduces Core's exact-request boundary.
  f.runtime.submitLocalCognitiveProposal = (request, proposal, seed) => {
    if (request !== originalCoreRequest) return false;
    return submit(request, proposal, seed);
  };
  f.controller.setEnabled(true);
  f.controller.observe(f.world, f.residents);
  await tick();
  assert.deepEqual(received, [['ImproveFoodSecurity']],
    'the model must still see only the Core-executable intent');
  assert.equal(f.submitted.length, 1,
    'the original request must pass unchanged to Core submission');
  assert.equal(f.submitted[0].request, originalCoreRequest);
  assert.deepEqual(f.submitted[0].request.allowedIntents, [
    'TeachKnowledge', 'TradeWithResident', 'MigrateHousehold',
    'ImproveFoodSecurity', 'CareForDependent',
  ]);
  assert.equal(f.submitted[0].proposal.intent, 'ImproveFoodSecurity');
  assert.equal(f.controller.getSnapshot().accepted, 1,
    'a valid executable decision must not be rejected by Core fingerprinting');
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

console.log('Live Resident AI: independent actor context, Core handoff, cancellation PASS');
