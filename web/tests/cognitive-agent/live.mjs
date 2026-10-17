import assert from 'node:assert/strict';
import { source } from './compile.mjs';

const { ResidentAIController, LIVE_AI_BUDGET } =
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
          emotionIntensity: .4, tags: [],
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
    controller, contexts, submitted, residents, world,
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
console.log('Live Resident AI: independent actor context, Core handoff, cancellation PASS');
