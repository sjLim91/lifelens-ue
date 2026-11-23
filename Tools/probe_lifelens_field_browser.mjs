#!/usr/bin/env node
// Field Visual V3 mobile viewport smoke: actual Core + browser SVG painting.
// No fake residents, no mocked terrain, no user phone/device claim.
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {mkdtempSync,mkdirSync,readFileSync,rmSync,writeFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,resolve} from 'node:path';

const url=process.argv[2],browser=process.argv[3]||'google-chrome',
  output=resolve(process.argv[4]||'field-visual-review.png');
if(!url||!/^https?:\/\//.test(url))throw Error('usage: probe-field URL [chrome] [screenshot.png]');
const profile=mkdtempSync(join(tmpdir(),'lifelens-field-v3-'));
const chrome=spawn(browser,[
  '--headless=new','--no-sandbox','--disable-dev-shm-usage','--disable-gpu',
  '--no-first-run','--remote-allow-origins=*','--remote-debugging-port=0',
  `--user-data-dir=${profile}`,'about:blank',
],{stdio:'ignore'});
let socket,sequence=0;
const pending=new Map(),errors=[],sleep=ms=>new Promise(r=>setTimeout(r,ms));
const send=(method,params={})=>new Promise((res,rej)=>{
  const id=++sequence;
  const timeout=setTimeout(()=>{pending.delete(id);rej(Error(method+' timeout'));},12000);
  pending.set(id,{res:x=>{clearTimeout(timeout);res(x);},
    rej:e=>{clearTimeout(timeout);rej(e);}});
  socket.send(JSON.stringify({id,method,params}));
});
try{
  const deadline=Date.now()+80000;
  let port;
  while(Date.now()<deadline){
    try{port=Number(readFileSync(join(profile,'DevToolsActivePort'),'utf8').split('\n')[0]);
      if(port>0)break;}catch{}
    if(chrome.exitCode!==null)throw Error('Chrome closed before field QA');
    await sleep(200);
  }
  assert(port,'Chrome debugging port unavailable');
  const tabs=await(await fetch(`http://127.0.0.1:${port}/json/list`)).json();
  const tab=tabs.find(t=>t.type==='page');
  assert(tab?.webSocketDebuggerUrl,'no browser target');
  socket=new WebSocket(tab.webSocketDebuggerUrl);
  socket.addEventListener('message',event=>{
    const msg=JSON.parse(event.data);
    if(msg.id!==undefined){
      const p=pending.get(msg.id);if(!p)return;pending.delete(msg.id);
      if(msg.error)p.rej(Error(msg.error.message));else p.res(msg.result);
    } else if(msg.method==='Runtime.exceptionThrown'){
      errors.push(msg.params?.exceptionDetails?.text??'runtime error');
    }
  });
  await Promise.race([
    new Promise((res,rej)=>{socket.addEventListener('open',res,{once:true});
      socket.addEventListener('error',rej,{once:true});}),
    sleep(12000).then(()=>{throw Error('WebSocket timeout');}),
  ]);
  await send('Runtime.enable');
  await send('Page.enable');
  await send('Emulation.setDeviceMetricsOverride',
    {width:390,height:844,deviceScaleFactor:1,mobile:true});
  await send('Page.addScriptToEvaluateOnNewDocument',{
    source:"try{localStorage.setItem('lifelens.web.renderMode','field-canvas');}catch(_){ }",
  });
  await send('Page.navigate',{url});
  let latest={};
  while(Date.now()<deadline){
    try{
      const response=await send('Runtime.evaluate',{returnByValue:true,
        expression:`(()=>{
          const el=document.querySelector('#worldCanvas');
          const status=document.querySelector('#status')?.textContent||'';
          const living=document.querySelector('#livingOverlay')?.textContent||'';
          const exception=document.querySelector('.error-card:not(.hidden) #errorText')?.textContent||'';
          const assets=performance.getEntriesByType('resource')
            .filter(r=>r.name.includes('/field/')&&r.name.includes('.svg')).length;
          let colors=0;
          if(el&&el.width>10&&el.height>10){
            const ctx=el.getContext('2d');
            if(ctx){
              const sample=ctx.getImageData(0,0,el.width,el.height).data;
              const set=new Set();
              for(let y=10;y<el.height-10;y+=Math.max(8,Math.floor(el.height/42)))
                for(let x=10;x<el.width-10;x+=Math.max(8,Math.floor(el.width/30))){
                  const k=(y*el.width+x)*4;
                  set.add(((sample[k]>>4)<<8)|((sample[k+1]>>4)<<4)|(sample[k+2]>>4));
                }
              colors=set.size;
            }
          }
          return {status,living,exception,assets,colors,width:el?.width||0,
            height:el?.height||0,opacity:el?.style.opacity||''};
        })()`,
      });
      latest=response?.result?.value??{};
      if(latest.exception)throw Error('Core/field error: '+latest.exception);
      if(latest.status.includes('코어 연결 실패'))throw Error('real Core did not boot');
      if(latest.status.includes('시뮬레이션 코어 연결됨')
        &&/인구\s+[1-9]\d*/.test(latest.living)
        &&latest.assets>=16&&latest.colors>=36&&latest.width>100
        &&latest.opacity!=='0') {
        await sleep(1200);
        const shot=await send('Page.captureScreenshot',{format:'png',captureBeyondViewport:false});
        assert(shot?.data,'empty screenshot');
        const bytes=Buffer.from(shot.data,'base64');
        assert(bytes.length>16000,'screenshot is unexpectedly small');
        mkdirSync(resolve(output,'..'),{recursive:true});
        writeFileSync(output,bytes);
        console.log('FIELD_VISUAL_BROWSER_PASS '+JSON.stringify({...latest,
          screenshot:output,bytes:bytes.length}));
        process.exitCode=0;
        break;
      }
    }catch(e){
      if(/Core\/field error|real Core/.test(String(e)))throw e;
    }
    await sleep(600);
  }
  if(process.exitCode!==0)throw Error('Field visual browser did not render: '
    +JSON.stringify(latest)+' exceptions='+errors.slice(-6).join('|'));
}catch(e){console.error('FIELD_VISUAL_BROWSER_FAIL '+String(e));process.exitCode=1;}
finally{
  socket?.close();chrome.kill('SIGTERM');await sleep(400);
  if(chrome.exitCode===null)chrome.kill('SIGKILL');
  rmSync(profile,{recursive:true,force:true});
}
