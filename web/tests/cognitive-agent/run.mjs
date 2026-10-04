import assert from 'node:assert/strict';
import {source} from './compile.mjs';

const {
  cognitiveProposalJsonSchema,
  parseCognitiveProposal,
}=await source('cognition/cognitive-contract.ts');
const {
  LocalCognitionProvider,
  isLoopbackCognitionUrl,
}=await source('cognition/local-cognition-provider.ts');
const {
  DeterministicFakeCognitionProvider,
}=await source('cognition/fake-cognition-provider.ts');
const {
  CognitionScheduler,
}=await source('cognition/cognition-scheduler.ts');

let passed=0;
async function test(name,fn){
  await fn();
  passed++;
  console.log('PASS',name);
}

const request=(actor='1',minute=100,allowedIntents=['ImproveFoodSecurity'])=>({
  actor,
  minute,
  trigger:'ResourceScarcity',
  needs:{hunger:.7,thirst:.2},
  personality:{conscientiousness:.8,curiosity:.4},
  emotion:{anxiety:.3,valence:-.2},
  memories:[{
    who:'0',minute:90,recallScore:.9,confidence:.9,importance:.9,
    emotionValence:-.7,emotionIntensity:.8,
    what:'food stores became scarce',where:'camp',tags:['food','scarcity'],
  }],
  beliefs:[],
  relationships:[{
    target:'2',socialBond:.8,affection:.7,trust:.9,respect:.7,
    conflict:.1,fear:0,grudge:0,
  }],
  allowedIntents,
});

const proposal=(req,intent=req.allowedIntents[0],targetResident=null)=>({
  actor:req.actor,
  intent,
  priority:.7,
  targetResident,
  rationale:'Short factual reason.',
});

await test('only loopback local endpoints are accepted',()=>{
  for(const value of [
    'http://localhost:8080',
    'http://127.0.0.1:8080/',
    'https://localhost:8443',
    'http://[::1]:8080',
  ])assert.equal(isLoopbackCognitionUrl(value),true,value);

  for(const value of [
    'https://api.openai.com',
    'http://192.168.0.2:8080',
    'https://example.com',
    'ftp://127.0.0.1/model',
    'not-a-url',
  ])assert.equal(isLoopbackCognitionUrl(value),false,value);

  assert.throws(
    ()=>new LocalCognitionProvider({baseUrl:'https://api.openai.com',model:'x'}),
    /loopback\/local/,
  );
});

await test('dynamic JSON schema contains only Core-supplied allowed intents',()=>{
  const req=request('1',100,['ExpandCultivation','ImproveFoodSecurity']);
  const schema=cognitiveProposalJsonSchema(req);
  assert.deepEqual(
    schema.properties.intent.enum,
    ['ExpandCultivation','ImproveFoodSecurity'],
  );
  assert.equal(schema.properties.actor.const,'1');
  assert.equal(schema.additionalProperties,false);
});

await test('proposal parser rejects invented intent, invalid targets and invalid priority',()=>{
  const req=request('1',100,['ImproveFoodSecurity','CooperateWithResident']);
  assert.equal(
    parseCognitiveProposal(proposal(req,'ImproveFoodSecurity'),req).intent,
    'ImproveFoodSecurity',
  );
  assert.throws(
    ()=>parseCognitiveProposal(proposal(req,'MigrateHousehold'),req),
    /not allowed/,
  );
  assert.throws(
    ()=>parseCognitiveProposal(proposal(req,'CooperateWithResident'),req),
    /requires another resident/,
  );
  assert.equal(
    parseCognitiveProposal(
      proposal(req,'CooperateWithResident','2'),
      req,
    ).targetResident,
    '2',
  );
  assert.throws(
    ()=>parseCognitiveProposal(
      {...proposal(req),priority:Number.NaN},
      req,
    ),
    /priority/,
  );
  assert.throws(
    ()=>parseCognitiveProposal(
      {...proposal(req),targetResident:'2'},
      req,
    ),
    /unexpected/,
  );
});

await test('local provider sends schema-constrained request with no cloud auth header',async()=>{
  const req=request('7',120,['ImproveFoodSecurity']);
  let seenUrl='';
  let seenInit;
  const provider=new LocalCognitionProvider(
    {baseUrl:'http://127.0.0.1:8080',model:'local-model'},
    async(url,init)=>{
      seenUrl=String(url);
      seenInit=init;
      return new Response(JSON.stringify({
        choices:[{message:{content:JSON.stringify(proposal(req))}}],
      }),{status:200,headers:{'content-type':'application/json'}});
    },
  );
  const result=await provider.reason(req,new AbortController().signal);
  assert.equal(result.intent,'ImproveFoodSecurity');
  assert.equal(seenUrl,'http://127.0.0.1:8080/v1/chat/completions');
  assert.equal(seenInit.method,'POST');
  assert.equal(seenInit.headers.authorization,undefined);
  const body=JSON.parse(seenInit.body);
  assert.equal(body.model,'local-model');
  assert.equal(body.temperature,0);
  assert.deepEqual(
    body.response_format.json_schema.schema.properties.intent.enum,
    ['ImproveFoodSecurity'],
  );
  assert(!body.messages[0].content.includes('chain-of-thought.'));
});

await test('deterministic fake provider produces repeatable typed proposals',async()=>{
  const provider=new DeterministicFakeCognitionProvider();
  const req=request('1',100,['CooperateWithResident','ImproveFoodSecurity']);
  const a=await provider.reason(req,new AbortController().signal);
  const b=await provider.reason(req,new AbortController().signal);
  assert.deepEqual(a,b);
  assert.equal(a.intent,'CooperateWithResident');
  assert.equal(a.targetResident,'2');
});

await test('scheduler bounds active and waiting work',async()=>{
  const resolvers=[];
  const provider={
    name:'controlled',
    reason:req=>new Promise(resolve=>{
      resolvers.push(()=>resolve(proposal(req)));
    }),
  };
  let minute=100;
  const scheduler=new CognitionScheduler(
    provider,
    ()=>minute,
    {maxConcurrent:1,maxQueued:1,timeoutMs:500,staleAfterSimulationMinutes:100},
  );
  const p1=scheduler.submit(request('1',100));
  const p2=scheduler.submit(request('2',100));
  const p3=scheduler.submit(request('3',100));
  assert.equal(await p3,null);
  assert.equal(resolvers.length,1);
  resolvers.shift()();
  assert.equal((await p1).actor,'1');
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.equal(resolvers.length,1);
  resolvers.shift()();
  assert.equal((await p2).actor,'2');
  scheduler.dispose();
});

await test('newer request from same resident invalidates the older late response',async()=>{
  const pending=new Map();
  const provider={
    name:'out-of-order',
    reason:req=>new Promise(resolve=>pending.set(req.minute,()=>resolve(proposal(req)))),
  };
  let minute=101;
  const scheduler=new CognitionScheduler(
    provider,
    ()=>minute,
    {maxConcurrent:2,maxQueued:2,timeoutMs:500,staleAfterSimulationMinutes:100},
  );
  const older=scheduler.submit(request('1',100));
  const newer=scheduler.submit(request('1',101));
  await new Promise(resolve=>setTimeout(resolve,0));
  pending.get(101)();
  assert.equal((await newer).actor,'1');
  pending.get(100)();
  assert.equal(await older,null);
  scheduler.dispose();
});

await test('timeout settles even when provider ignores AbortSignal forever',async()=>{
  const provider={
    name:'hung',
    reason:()=>new Promise(()=>{}),
  };
  const scheduler=new CognitionScheduler(
    provider,
    ()=>100,
    {maxConcurrent:1,maxQueued:0,timeoutMs:15,staleAfterSimulationMinutes:100},
  );
  const started=Date.now();
  assert.equal(await scheduler.submit(request()),null);
  assert(Date.now()-started<250);
  scheduler.dispose();
});

await test('response is discarded after simulation context becomes stale',async()=>{
  const provider={
    name:'instant',
    reason:async req=>proposal(req),
  };
  let minute=200;
  const scheduler=new CognitionScheduler(
    provider,
    ()=>minute,
    {maxConcurrent:1,maxQueued:0,timeoutMs:100,staleAfterSimulationMinutes:10},
  );
  assert.equal(await scheduler.submit(request('1',100)),null);
  scheduler.dispose();
});

await test('reset clears queued work and fail-closes subsequent stale completion',async()=>{
  let release;
  const provider={
    name:'reset',
    reason:req=>new Promise(resolve=>{
      release=()=>resolve(proposal(req));
    }),
  };
  const scheduler=new CognitionScheduler(
    provider,
    ()=>100,
    {maxConcurrent:1,maxQueued:1,timeoutMs:500,staleAfterSimulationMinutes:100},
  );
  const active=scheduler.submit(request('1',100));
  const queued=scheduler.submit(request('2',100));
  await new Promise(resolve=>setTimeout(resolve,0));
  scheduler.reset();
  assert.equal(await queued,null);
  release();
  assert.equal(await active,null);
  scheduler.dispose();
});

console.log(`Cognitive agent Web adapter: ${passed} tests passed`);
