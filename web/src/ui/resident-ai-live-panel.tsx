import { useState, useSyncExternalStore } from 'react';
import type { CognitiveIntent } from '../cognition/cognitive-contract';
import { residentAI } from '../cognition/live-resident-ai';
import type { Resident } from '../runtime/core-types';
import { formatCognitiveIntent } from '../localization/korean';

// Live action authority is separate from the original read-only Shadow UI.
export function ResidentAILivePanel({ resident }: { resident: Resident }) {
  const state = useSyncExternalStore(
    residentAI.subscribe, residentAI.getSnapshot, residentAI.getSnapshot,
  );
  const [endpoint, setEndpoint] = useState(
    state.configuration?.baseUrl ?? 'http://127.0.0.1:8080',
  );
  const [model, setModel] = useState(state.configuration?.model ?? '');
  const [error, setError] = useState('');
  const decision = state.byResident[resident.id];
  return <section className="resident-ai-live-config">
    <strong>자율 주민 AI — 실제 행동 연결</strong>
    <p className="hint">
      무료 로컬 모델 하나로 모든 주민이 각자의 기억·감정·성격을 바탕으로 판단합니다.
      현재 Core는 협력·갈등 해결 선택을 실행 경로로 연결합니다.
      다른 전략은 검증 후 기록하지만 아직 직접 실행하지 않습니다.
    </p>
    <form onSubmit={(event) => {
      event.preventDefault();
      setError('');
      if (!residentAI.configure(endpoint, model) || !residentAI.setEnabled(true)) {
        setError('로컬 모델 주소 또는 모델 설정을 확인해 주세요.');
      }
    }}>
      <label>로컬 모델 주소
        <input type="url" value={state.enabled ? state.configuration?.baseUrl ?? endpoint : endpoint}
          disabled={state.enabled} maxLength={300}
          onChange={(event) => setEndpoint(event.target.value)} />
      </label>
      <label>로컬 모델명
        <input value={state.enabled ? state.configuration?.model ?? model : model}
          disabled={state.enabled} maxLength={200}
          onChange={(event) => setModel(event.target.value)} />
      </label>
      {state.enabled
        ? <button type="button" onClick={() => residentAI.setEnabled(false)}>주민 AI 중지</button>
        : <button type="submit">주민 AI 시작</button>}
    </form>
    {error && <p role="alert">{error}</p>}
    <p role="status">
      {state.enabled ? '로컬 주민 AI 가동' : '기본 Core만 가동'}
      {' · '}Core 승인 {state.accepted}회 · 미승인/오류 {state.rejected}회
    </p>
    {decision && <p className="hint">
      현재 주민 최근 판단: {decision.intent
        ? formatCognitiveIntent(decision.intent as CognitiveIntent) : '없음'}
      {' · '}{decision.status === 'recorded' ? 'Core 기록 승인'
        : decision.status === 'stale' ? '시효 만료'
        : decision.status === 'unavailable' ? '모델 응답 없음'
        : decision.status === 'error' ? '추론 오류' : 'Core 거부'}
    </p>}
    <p className="hint">
      Core 승인 기록은 행동 실행이나 목표 성공을 의미하지 않습니다.
      무료 모델은 사용자가 로컬에서 실행해야 하며 유료 클라우드 모델은 연결하지 않습니다.
    </p>
  </section>;
}
