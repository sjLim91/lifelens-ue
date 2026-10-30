#!/usr/bin/env node
// Real browser smoke test: wait for Core bootstrap via DevTools, not Chrome
// --dump-dom (which can hang on unrelated WebLLM/WebGL background requests).
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdtempSync, readFileSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const url = process.argv[2];
const browser = process.argv[3] || 'google-chrome';
if (!url || !/^https?:\/\//.test(url)) throw Error('usage: probe <https-url> [chrome]');
const profile = mkdtempSync(join(tmpdir(), 'lifelens-live-core-'));
const chrome = spawn(browser, [
  '--headless=new', '--no-sandbox', '--disable-dev-shm-usage',
  '--disable-gpu', '--no-first-run', '--no-default-browser-check',
  '--remote-allow-origins=*', '--remote-debugging-port=0',
  `--user-data-dir=${profile}`, 'about:blank',
], { stdio: 'ignore' });
let socket;
const exceptions = [];
const pending = new Map();
let sequence = 0;
const deadline = Date.now() + 50000;
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
function send(method, params = {}) {
  return new Promise((resolve, reject) => {
    const id = ++sequence;
    const timer = setTimeout(() => {
      pending.delete(id);
      reject(Error(`DevTools timeout: ${method}`));
    }, 10000);
    pending.set(id, {
      resolve: result => { clearTimeout(timer); resolve(result); },
      reject: error => { clearTimeout(timer); reject(error); },
    });
    socket.send(JSON.stringify({ id, method, params }));
  });
}
try {
  let port;
  while (Date.now() < deadline) {
    try {
      port = Number(readFileSync(join(profile, 'DevToolsActivePort'), 'utf8').split('\n')[0]);
      if (port > 0) break;
    } catch {}
    if (chrome.exitCode !== null) throw Error(`Chrome exited ${chrome.exitCode}`);
    await sleep(200);
  }
  assert(port, 'Chrome did not expose its remote debugging port');
  const tabs = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
  const target = tabs.find(tab => tab.type === 'page');
  assert(target?.webSocketDebuggerUrl, 'Chrome did not open a page target');
  socket = new WebSocket(target.webSocketDebuggerUrl);
  socket.addEventListener('message', event => {
    const message = JSON.parse(event.data);
    if (message.id !== undefined) {
      const result = pending.get(message.id);
      if (!result) return;
      pending.delete(message.id);
      if (message.error) result.reject(Error(message.error.message));
      else result.resolve(message.result);
      return;
    }
    if (message.method === 'Runtime.exceptionThrown') {
      exceptions.push(
        message.params?.exceptionDetails?.exception?.description
        ?? message.params?.exceptionDetails?.text ?? 'Unknown JS exception',
      );
    }
  });
  await Promise.race([
    new Promise((resolve, reject) => {
      socket.addEventListener('open', resolve, { once: true });
      socket.addEventListener('error', reject, { once: true });
    }),
    sleep(10000).then(() => { throw Error('Chrome DevTools WebSocket connection timed out'); }),
  ]);
  await send('Runtime.enable');
  await send('Page.enable');
  // Use product-provided emergency 2D mode; this tests real Core bootstrap
  // without software-WebGL stalls on CI, and never injects fake Core state.
  await send('Page.addScriptToEvaluateOnNewDocument', {
    source: "try { localStorage.setItem('lifelens.web.renderMode','legacy-canvas'); } catch (_) {}",
  });
  const navigation = await send('Page.navigate', { url });
  if (navigation.errorText) throw Error(`Chrome navigation failed: ${navigation.errorText}`);
  let latest = {};
  while (Date.now() < deadline) {
    try {
      const output = await send('Runtime.evaluate', {
        expression: `({href:location.href,status:document.querySelector('#status')?.textContent || null,
          living:document.querySelector('#livingOverlay')?.textContent || null,
          error:document.querySelector('.error-card:not(.hidden) #errorText')?.textContent || null})`,
        returnByValue: true,
      });
      latest = output?.result?.value ?? {};
      if (latest.status?.includes('시뮬레이션 코어 연결됨')
          && /인구\s+[1-9]\d*/.test(latest.living || '') && !latest.error) {
        console.log('LIVE_LIFELENS_CORE_BROWSER_PASS ' + JSON.stringify(latest));
        process.exitCode = 0;
        break;
      }
      if (latest.status?.includes('시뮬레이션 코어 연결 실패')) {
        throw Error('Actual LifeLensCore boot error: ' + JSON.stringify(latest));
      }
      if (latest.href?.startsWith('chrome-error:')) {
        throw Error('Chrome navigation failed: ' + JSON.stringify(latest));
      }
    } catch (error) {
      if (/Actual LifeLensCore|Chrome navigation failed/.test(String(error))) throw error;
      // Navigation invalidates old execution contexts; poll the new context.
    }
    await sleep(500);
  }
  if (process.exitCode !== 0) {
    throw Error('Core not ready: ' + JSON.stringify(latest)
      + ' JS exceptions: ' + exceptions.slice(-5).join(' | ').slice(0,1500));
  }
} catch (error) {
  console.error(String(error));
  process.exitCode = 1;
} finally {
  socket?.close();
  chrome.kill('SIGTERM');
  await sleep(500);
  if (chrome.exitCode === null) chrome.kill('SIGKILL');
  rmSync(profile, { recursive: true, force: true });
}
