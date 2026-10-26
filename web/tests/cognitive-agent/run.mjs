import assert from 'node:assert/strict';
import {source} from './compile.mjs';

const {
  cognitiveProposalJsonSchema,
  parseCognitiveProposal,
  parseCognitiveRequest,
}=await source('cognition/cognitive-contract.ts');
const { parseAutonomousGoalPreview } = await source('cognition/autonomous-goal-preview.ts');
const {
  BrowserCognitionProvider, createBrowserResidentCognition,
  BUILTIN_RESIDENT_MODEL,
}=await source('cognition/browser-cognition-provider.ts');

const {
  LocalCognitionProvider,
  discoverLocalCognitionModels,
  discoverAutomaticLocalCognition,
  AUTOMATIC_LOCAL_COGNITION_ENDPOINTS,
  isLoopbackCognitionUrl,
}=await source('cognition/local-cognition-provider.ts');
const {
  DeterministicFakeCognitionProvider,
}=await source('cognition/fake-cognition-provider.ts');
const {
  CognitionScheduler,
}=await source('cognition/cognition-scheduler.ts');
const {
  LifeLensCoreBridge,
}=await source('runtime/core-bridge.ts');

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

await test('loopback model discovery is bounded, unique and credential-free',async()=>{
  let seen;
  const models=await discoverLocalCognitionModels(
    'http://localhost:8080',
    new AbortController().signal,
    async(url,init)=>{
      seen={url:String(url),init};
      return new Response(JSON.stringify({
        data:[
          {id:'qwen-local'},
          {id:'qwen-local'},
          {id:'small-reasoner'},
          {id:''},
          {},
        ],
      }),{status:200,headers:{'content-type':'application/json'}});
    },
  );
  assert.deepEqual(models,['qwen-local','small-reasoner']);
  assert.equal(seen.url,'http://localhost:8080/v1/models');
  assert.equal(seen.init.method,'GET');
  assert.equal(seen.init.headers.authorization,undefined);
  await assert.rejects(
    discoverLocalCognitionModels(
      'https://example.com',
      new AbortController().signal,
    ),
    /loopback\/local/,
  );
});

await test('automatic model discovery uses only bounded free loopback services', async()=>{
  const called=[];
  const result=await discoverAutomaticLocalCognition(
    new AbortController().signal, async (url,init)=>{
      called.push({url:String(url),method:init.method});
      if(String(url).includes(':11434/')) throw Error('not running');
      if(String(url).includes(':1234/')) return new Response(JSON.stringify({
        data:[{id:'local-available-model'}],
      }),{status:200});
      throw Error('unexpected local model');
    },
  );
  assert.equal(result.baseUrl,'http://127.0.0.1:1234');
  assert.equal(result.model,'local-available-model');
  assert.deepEqual(called.map(c=>c.method),['GET','GET']);
  for(const endpoint of AUTOMATIC_LOCAL_COGNITION_ENDPOINTS)
    assert(isLoopbackCognitionUrl(endpoint));
  assert.equal(await discoverAutomaticLocalCognition(
    new AbortController().signal,
    async()=>{ throw Error('not running'); },
  ),null,'absence of local model must never fabricate a cloud model');
  assert.equal(await discoverAutomaticLocalCognition(
    AbortSignal.abort(),async()=>{throw Error('never called');},
  ),null);
});

await test('embedded WebGPU model can reason locally with no server or cloud key',async()=>{
  const req=request('7',120,['ImproveFoodSecurity']);
  assert.equal(await createBrowserResidentCognition(()=>false,async()=>{
    throw Error('load must not run without WebGPU');
  }),null);
  let loadedModel='';
  let observed;
  const provider=await createBrowserResidentCognition(()=>true,async model=>{
    loadedModel=model;
    return {chat:{completions:{create:async input=>{
      observed=input;
      return {choices:[{message:{content:JSON.stringify(proposal(req))}}]};
    }}}};
  });
  assert.equal(loadedModel,BUILTIN_RESIDENT_MODEL);
  assert(provider instanceof BrowserCognitionProvider);
  const actual=await provider.reason(req,new AbortController().signal);
  assert.equal(actual.intent,'ImproveFoodSecurity');
  assert.equal(observed.temperature,0);
  assert.equal(observed.response_format.type,'json_object');
  assert(!JSON.stringify(observed).includes('api.openai.com'));
  assert.equal((await createBrowserResidentCognition(()=>true,
    async()=>{throw Error('GPU out of memory');})),null);
  const bad=new BrowserCognitionProvider({chat:{completions:{
    create:async()=>({choices:[{message:{content:JSON.stringify({
      ...proposal(req), intent:'CreateFoodFromNothing',
    })}}]}),
  }}});
  await assert.rejects(bad.reason(req,new AbortController().signal),
    /violated the Core contract/);
});

await test('disposed browser engine cannot return a delayed resident thought',async()=>{
  let resolve;
  const provider=new BrowserCognitionProvider({
    chat:{completions:{create:()=>new Promise(done=>{resolve=done;})}},
    unload:async()=>{},
  });
  const ac=new AbortController();
  const pending=provider.reason(request('1',100,['ImproveFoodSecurity']),
    ac.signal);
  await new Promise(done=>setTimeout(done,0));
  await provider.dispose();
  resolve({choices:[{message:{content:JSON.stringify(
    proposal(request('1',100,['ImproveFoodSecurity'])))}}]});
  assert.equal(await pending,null,
    'obsolete GPU result cannot revive a disposed resident provider');
});

await test('replacing WebGPU scheduler unloads GPU model once, not on World reset',async()=>{
  const req=request('7',120,['ImproveFoodSecurity']);
  let unloads=0,reasonCalls=0;
  const engine={
    chat:{completions:{create:async()=>{
      reasonCalls++;
      return {choices:[{message:{content:JSON.stringify(proposal(req))}}]};
    }}},
    unload:async()=>{unloads++;},
  };
  const provider=new BrowserCognitionProvider(engine);
  const scheduler=new CognitionScheduler(provider,()=>120,{
    maxConcurrent:1,maxQueued:0,timeoutMs:200,
    staleAfterSimulationMinutes:96,
  });
  assert.equal((await scheduler.submit(req)).actor,'7');
  scheduler.reset();
  assert.equal(unloads,0,
    'resetting the world should not unnecessarily reload resident language AI');
  assert.equal((await scheduler.submit(req)).actor,'7');
  assert.equal(reasonCalls,2);
  scheduler.dispose();
  await Promise.resolve();
  assert.equal(unloads,1,'discarding the old scheduler releases GPU resources');
  assert.equal(await provider.reason(req,new AbortController().signal),null,
    'a disposed GPU engine cannot generate new resident decisions');
  scheduler.dispose();
  assert.equal(unloads,1,'GPU teardown is idempotent');

  // A WebGPU driver unload failure must not become an unhandled rejection
  // or stop Core itself from continuing.
  const failing=new BrowserCognitionProvider({
    chat:{completions:{create:async()=>({choices:[]})}},
    unload:async()=>{throw Error('GPU lost while unloading');},
  });
  const lost=new CognitionScheduler(failing,()=>120,{
    maxConcurrent:1,maxQueued:0,timeoutMs:200,
    staleAfterSimulationMinutes:96,
  });
  lost.dispose();
  await Promise.resolve();
});

await test('Core bridge reads optional cognitive ABI and fails closed on corrupt or old runtime',()=>{
  const req=request('9',140,['ImproveFoodSecurity']);
  const baseClient={
    newGame:()=>true,
    runMinutes:()=>{},
    worldOverviewJson:()=>JSON.stringify({available:true,minute:140,livingResidents:1}),
    residentsJson:()=>JSON.stringify({available:true,residents:[]}),
    terrainWindowJson:()=>JSON.stringify({available:false,centerChunkX:0,centerChunkY:0,chunks:[]}),
  };
  const bridge=new LifeLensCoreBridge({
    ...baseClient,
    cognitiveRequestJson:(actor,trigger)=>{
      assert.equal(actor,'9');
      assert.equal(trigger,'Reflection');
      return JSON.stringify({...req,trigger});
    },
  });
  assert.deepEqual(bridge.cognitiveRequest('9','Reflection'),{
    ...req,
    trigger:'Reflection',
  });

  const corrupt=new LifeLensCoreBridge({
    ...baseClient,
    cognitiveRequestJson:()=>'{',
  });
  assert.equal(corrupt.cognitiveRequest('9','Reflection'),null);

  const malformed=new LifeLensCoreBridge({
    ...baseClient,
    cognitiveRequestJson:()=>JSON.stringify({...req,minute:'140'}),
  });
  assert.equal(malformed.cognitiveRequest('9','Reflection'),null);

  const oldRuntime=new LifeLensCoreBridge(baseClient);
  assert.equal(oldRuntime.cognitiveRequest('9','Reflection'),null);
});

await test('Core local goal candidates are bounded, truthful read-only previews',()=>{
  const preview={
    available:true,
    actor:'9',
    minute:140,
    source:'CoreLocalGoalComposer',
    affectsSimulation:false,
    candidates:[{
      priority:0.73,
      expiresMinute:1580,
      conditions:[{
        metric:'ActorNeed',need:'Thirst',materialCode:0,
        targetResident:'0',operator:'AtMost',threshold:0.35,
      }],
      evidence:[{kind:'ActorNeed',index:0}],
    }],
  };
  assert.deepEqual(parseAutonomousGoalPreview(preview),preview);

  const client={
    newGame:()=>true,runMinutes:()=>{},
    worldOverviewJson:()=>JSON.stringify({available:true,minute:140,livingResidents:1}),
    residentsJson:()=>JSON.stringify({available:true,residents:[]}),
    terrainWindowJson:()=>JSON.stringify({available:false,centerChunkX:0,centerChunkY:0,chunks:[]}),
    autonomousGoalCandidatesJson:(_actor,trigger)=>{
      assert.equal(trigger,'Reflection');
      // Deliberately return another actor's data to test bridge authority.
      return JSON.stringify(preview);
    },
  };
  const bridge=new LifeLensCoreBridge(client);
  assert.deepEqual(bridge.autonomousGoalCandidates('9'),preview);
  assert.equal(bridge.autonomousGoalCandidates('2'),null,
    'unrelated resident ID cannot import someone else\'s goal preview');
  const oldRuntime={...client};
  delete oldRuntime.autonomousGoalCandidatesJson;
  assert.equal(new LifeLensCoreBridge(oldRuntime).autonomousGoalCandidates('9'),null);
  assert.throws(()=>parseAutonomousGoalPreview({...preview,affectsSimulation:true}),/read-only/);
  assert.throws(()=>parseAutonomousGoalPreview({...preview,source:'web-fake'}),/read-only/);
  assert.throws(()=>parseAutonomousGoalPreview({...preview,actor:'not-an-id'}),/identifier/);
  assert.throws(()=>parseAutonomousGoalPreview({...preview,candidates:Array(7).fill(preview.candidates[0])}),/candidate/);
  assert.throws(()=>parseAutonomousGoalPreview({
    ...preview,candidates:[{...preview.candidates[0],priority:Number.NaN}],
  }),/number/);
  assert.throws(()=>parseAutonomousGoalPreview({
    ...preview,candidates:[{
      ...preview.candidates[0],
      conditions:[{...preview.candidates[0].conditions[0],metric:'SpawnFacility'}],
    }],
  }),/predicate/);
});

await test('Core request parser is bounded and rejects malformed runtime context',()=>{
  const valid=request('1',100,['ImproveFoodSecurity']);
  assert.deepEqual(parseCognitiveRequest(valid),valid);
  assert.throws(
    ()=>parseCognitiveRequest({...valid,minute:Number.NaN}),
    /minute/,
  );
  assert.throws(
    ()=>parseCognitiveRequest({...valid,trigger:'FutureTrigger'}),
    /trigger/,
  );
  assert.throws(
    ()=>parseCognitiveRequest({
      ...valid,
      allowedIntents:['FutureIntent'],
    }),
    /allowed intents/,
  );
  assert.throws(
    ()=>parseCognitiveRequest({
      ...valid,
      memories:Array.from({length:33},()=>valid.memories[0]),
    }),
    /memories/,
  );
  assert.throws(
    ()=>parseCognitiveRequest({
      ...valid,
      personality:{curiosity:Number.POSITIVE_INFINITY},
    }),
    /personality/,
  );
});

await test('issued context identities stay lossless and incomplete receipts fail closed',()=>{
  const legacy=request();
  const issued={...legacy,worldSeed:'18446744073709551615',
    populationSeed:'9',generationVersion:4,contextFingerprint:'9007199254740993'};
  assert.deepEqual(parseCognitiveRequest(issued),issued);
  for(const patch of [
    {contextFingerprint:'9007199254740993.0'},
    {contextFingerprint:'18446744073709551616'},
    {contextFingerprint:9007199254740992},
    {populationSeed:undefined},
    {generationVersion:0},
  ])assert.throws(()=>parseCognitiveRequest({...issued,...patch}),/receipt|generation/);
  let calls=0,received;
  const bridge=new LifeLensCoreBridge({
    submitLocalCognitiveProposal:(...args)=>{calls++;received=args;return true;},
  });
  assert.equal(bridge.submitLocalCognitiveProposal(legacy,proposal(legacy),'1'),false);
  assert.equal(bridge.submitLocalCognitiveProposal(issued,proposal(issued),'2'),false);
  assert.equal(calls,0);
  assert.equal(bridge.submitLocalCognitiveProposal(issued,proposal(issued),issued.worldSeed),true);
  assert.equal(calls,1);
  assert.equal(received[8],issued.contextFingerprint);
  assert.equal(received[2],issued.worldSeed);
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
  assert(body.messages[0].content.includes('Do not provide hidden chain-of-thought.'));
  assert(!body.messages[0].content.includes('show your chain of thought'));
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

await test('full single-model queue admits fresher thought from the same resident',async()=>{
  const entered=[];
  const release=[];
  const provider={name:'local-model',reason:req=>new Promise(resolve=>{
    entered.push([req.actor,req.minute]);
    release.push(()=>resolve(proposal(req)));
  })};
  const scheduler=new CognitionScheduler(provider,()=>105,{
    maxConcurrent:1,maxQueued:1,timeoutMs:1000,staleAfterSimulationMinutes:100,
  });
  const active=scheduler.submitObserved(request('1',100));
  const old=scheduler.submitObserved(request('2',101));
  const newThought=scheduler.submitObserved(request('2',105));
  assert.equal((await old).status,'stale',
    'older same-resident queue item must release its reserved slot immediately');
  assert.equal((await scheduler.submitObserved(request('2',103))).status,'stale',
    'a new older Core observation may not displace the newest queued mind');
  assert.equal((await scheduler.submitObserved(request('3',105))).status,'busy',
    'another resident may not steal the occupied one-slot queue');
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.deepEqual(entered,[['1',100]]);
  release.shift()();
  assert.equal((await active).status,'validated');
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.deepEqual(entered,[['1',100],['2',105]],
    'only the latest admitted same-resident request reaches local inference');
  release.shift()();
  assert.equal((await newThought).status,'validated');
  scheduler.dispose();
});

await test('superseded queued resident thoughts never consume local inference',async()=>{
  const entered=[];
  const pending=[];
  const provider={name:'single-webgpu-slot',reason:req=>new Promise(resolve=>{
    entered.push({actor:req.actor,minute:req.minute});
    pending.push(()=>resolve(proposal(req)));
  })};
  const scheduler=new CognitionScheduler(provider,()=>102,{
    maxConcurrent:1,maxQueued:2,timeoutMs:500,
    staleAfterSimulationMinutes:100,
  });
  const active=scheduler.submitObserved(request('1',100));
  const obsolete=scheduler.submitObserved(request('2',101));
  const newest=scheduler.submitObserved(request('2',102));
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.deepEqual(entered,[{actor:'1',minute:100}]);
  pending.shift()();
  assert.equal((await active).status,'validated');
  const discarded=await obsolete;
  assert.equal(discarded.status,'stale');
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.deepEqual(entered,[
    {actor:'1',minute:100},
    {actor:'2',minute:102},
  ],'the obsolete request must never enter the local model');
  pending.shift()();
  assert.equal((await newest).status,'validated');
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

await test('older Core minute cannot cancel a newer active resident thought',async()=>{
  const entered=[];
  const resolve=[];
  const provider={name:'single-resident',reason:req=>new Promise(done=>{
    entered.push(req.minute);
    resolve.push(()=>done(proposal(req)));
  })};
  const scheduler=new CognitionScheduler(provider,()=>105,{
    maxConcurrent:2,maxQueued:2,timeoutMs:1000,
    staleAfterSimulationMinutes:100,
  });
  const newest=scheduler.submitObserved(request('1',105));
  await new Promise(done=>setTimeout(done,0));
  assert.deepEqual(entered,[105]);
  assert.equal((await scheduler.submitObserved(request('1',104))).status,'stale',
    'older observation must not supersede an already active newer mind');
  assert.deepEqual(entered,[105],
    'older rejected observation must not call the shared local model');
  resolve.shift()();
  assert.equal((await newest).status,'validated',
    'the newer authentic inference remains authoritative');
  scheduler.dispose();
});

await test('scheduler rejects malformed Core minute without consuming inference',async()=>{
  let calls=0;
  const scheduler=new CognitionScheduler({
    name:'must-not-run',reason:async req=>{calls++;return proposal(req);},
  },()=>100,{
    maxConcurrent:1,maxQueued:1,timeoutMs:1000,
    staleAfterSimulationMinutes:100,
  });
  for(const minute of [-1,0.25,Number.NaN,Number.POSITIVE_INFINITY,
                       Number.MAX_SAFE_INTEGER+1]){
    assert.equal((await scheduler.submitObserved(request('1',minute))).reason,
      'invalid_schema');
  }
  assert.equal(calls,0);
  scheduler.dispose();
});

await test('newest Core minute survives when a newer provider finishes before an older one',async()=>{
  const entered=[];
  const pending=new Map();
  const provider={name:'out-of-order-overlapping',reason:req=>new Promise(resolve=>{
    entered.push(req.minute);
    pending.set(req.minute,()=>resolve(proposal(req)));
  })};
  const scheduler=new CognitionScheduler(provider,()=>106,{
    maxConcurrent:2,maxQueued:0,timeoutMs:1000,
    staleAfterSimulationMinutes:100,
  });
  const older=scheduler.submitObserved(request('1',100));
  const newer=scheduler.submitObserved(request('1',105));
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.deepEqual(entered,[100,105]);
  pending.get(105)();
  assert.equal((await newer).status,'validated');
  assert.equal((await scheduler.submitObserved(request('1',103))).status,
    'stale','older observation must not regain authority while superseded work is active');
  assert.deepEqual(entered,[100,105],
    'discarded older request cannot consume a local inference slot');
  const freshest=scheduler.submitObserved(request('1',106));
  await new Promise(resolve=>setTimeout(resolve,0));
  assert.deepEqual(entered,[100,105,106]);
  pending.get(106)();
  assert.equal((await freshest).status,'validated');
  assert.equal((await scheduler.submitObserved(request('1',104))).status,'stale',
    'most recent settled Core minute must outlive still-running older inference');
  pending.get(100)();
  assert.equal((await older).status,'stale');
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

await test('reset releases an abort-ignoring active slot without waiting for timeout',async()=>{
  let calls=0;
  let releaseOld;
  const provider={name:'reset-hung',reason:req=>{
    calls++;
    return calls===1 ? new Promise(resolve=>{releaseOld=()=>resolve(proposal(req));}) : Promise.resolve(proposal(req));
  }};
  const scheduler=new CognitionScheduler(provider,()=>100,
    {maxConcurrent:1,maxQueued:1,timeoutMs:1000,staleAfterSimulationMinutes:100});
  const active=scheduler.submit(request('1',100));
  const queued=scheduler.submit(request('2',100));
  await new Promise(resolve=>setTimeout(resolve,0));
  scheduler.reset();
  assert.equal(await queued,null);
  const cancelled=await Promise.race([active,new Promise(resolve=>setTimeout(()=>resolve('pending'),40))]);
  assert.equal(cancelled,null,'reset must settle an active caller promptly');
  assert.equal((await scheduler.submit(request('3',100))).actor,'3');
  releaseOld();
  assert.equal(calls,2,'late old completion must not restart cancelled queued work');
  scheduler.dispose();
});

await test('dispose promptly settles an abort-ignoring provider and rejects new work',async()=>{
  const scheduler=new CognitionScheduler({name:'hung',reason:()=>new Promise(()=>{})},()=>100,
    {maxConcurrent:1,maxQueued:0,timeoutMs:1000,staleAfterSimulationMinutes:100});
  const active=scheduler.submit(request());
  scheduler.dispose();
  assert.equal(await Promise.race([active,new Promise(resolve=>setTimeout(()=>resolve('pending'),40))]),null);
  assert.equal(await scheduler.submit(request()),null);
});

await test('invalid or rewound simulation time fail-closes provider results',async()=>{
  for(const now of [NaN,Infinity,99]){
    const scheduler=new CognitionScheduler({name:'instant',reason:async req=>proposal(req)},()=>now,
      {maxConcurrent:1,maxQueued:0,timeoutMs:100,staleAfterSimulationMinutes:100});
    assert.equal(await scheduler.submit(request('1',100)),null);
    scheduler.dispose();
  }
});

await test('a throwing context reader settles null and leaves the scheduler usable',async()=>{
  let fail=true;
  const scheduler=new CognitionScheduler({name:'instant',reason:async req=>proposal(req)},()=>{
    if(fail)throw Error('context unavailable');
    return 100;
  },{maxConcurrent:1,maxQueued:0,timeoutMs:100,staleAfterSimulationMinutes:100});
  assert.equal(await scheduler.submit(request()),null);
  fail=false;
  assert.equal((await scheduler.submit(request('2',100))).actor,'2');
  scheduler.dispose();
});

console.log(`Cognitive agent Web adapter: ${passed} tests passed`);

await import('./shadow.mjs');
