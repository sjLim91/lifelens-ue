import assert from 'node:assert/strict';
import { build } from 'esbuild';
import { renderToStaticMarkup } from 'react-dom/server';
import { createElement } from 'react';
import { mkdtempSync, rmSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { pathToFileURL } from 'node:url';
const temporary = mkdtempSync(resolve(import.meta.dirname, 'ui-review-'));
try {
  const output = join(temporary, 'panel.mjs');
  await build({ entryPoints: [resolve(import.meta.dirname, '../../src/ui/cognitive-shadow-panel.tsx')], outfile: output, bundle: true, platform: 'node', format: 'esm', jsx: 'automatic', external: ['react', 'react-dom'] });
  const { CognitiveShadowPanel } = await import(pathToFileURL(output));
  const resident = { id: '1', name: '가람', alive: true, activityLabel: 'Idle' };
  const context = { actor: '1', minute: 100, trigger: 'Reflection', needs: { hunger: .7 }, personality: { empathy: .8 }, emotion: { joy: .4 }, memories: [], beliefs: [], relationships: [], allowedIntents: ['ImproveFoodSecurity'] };
  const record = { id: 1, actor: '1', name: '가람', minute: 100, actual: { activityLabel: 'Idle' }, context, provider: 'free-local-fixture', outcome: { status: 'validated', reason: 'validated', proposal: { actor: '1', intent: 'ImproveFoodSecurity', priority: .6, targetResident: null, rationale: '배고픔을 확인했습니다.' } } };
  const state = { enabled: true, automatic: false, suspended: false, status: 'ready', pendingActor: null, records: [record], configuration: { baseUrl: 'http://localhost:8080', model: 'local-model' } };
  const controller = { subscribe: () => () => {}, getSnapshot: () => state };
  const render = () => renderToStaticMarkup(createElement(CognitiveShadowPanel, { resident, residents: [resident], controller }));
  let html = render();
  assert(html.includes('value="local-model"'), 'active configuration appears after panel remount');
  assert(!html.includes('free-local-fixture'), 'provider enum is registered in Korean');
  assert(html.startsWith('<details')); assert(!/<details[^>]*\bopen\b/.test(html), 'advanced UI is closed by default');
  for (const text of ['고급 관찰', '관찰 시뮬레이션 시각', '현재 실제 행동', '관찰 당시 실제 행동', '식량 확보 개선', '판단 이유', '성격', '감정', '중요 기억', '중요 신념', '관계', '현재 욕구', '실행 안 함']) assert(html.includes(text), text);
  for (const [status, reason, text] of [['rejected', 'invalid_json', '잘못된 JSON'], ['timeout', 'timeout', '응답 시간 초과'], ['error', 'provider_error', '로컬 제공자 처리 실패'], ['cancelled', 'cancelled', '요청 취소']]) {
    state.records = [{ ...record, id: 2, outcome: { status, reason, proposal: null } }, record];
    html = render(); assert(html.includes(text)); assert(html.includes('식량 확보 개선'), 'latest validated proposal survives failures');
  }
  assert(!/ImproveFoodSecurity|invalid_json|provider_error/.test(html), 'enum tokens are localized');
  console.log('PASS Shadow resident detail: collapsed UI, Korean labels, context, status, latest valid result');
} finally { rmSync(temporary, { recursive: true, force: true }); }
