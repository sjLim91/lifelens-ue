import { useState, useSyncExternalStore } from 'react';
import type { CognitiveIntent } from '../cognition/cognitive-contract';
import { residentAI } from '../cognition/live-resident-ai';
import { compareResidentAISessionReports, type ResidentAIAuditComparison } from '../cognition/resident-ai-session-comparison';
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
  const [auditMessage, setAuditMessage] = useState('');
  const [baselineJson, setBaselineJson] = useState('');
  const [comparison, setComparison] = useState<ResidentAIAuditComparison | null>(null);
  const auditReport = residentAI.getAuditReport();
  const decision = state.byResident[resident.id];
  return <section className="resident-ai-live-config">
    <strong>자율 주민 AI — 실제 행동 연결</strong>
    <p className="hint">
      무료 로컬 모델 하나로 모든 주민이 각자의 기억·감정·성격을 바탕으로 판단합니다.
      Core는 협력·갈등 해결과 식량·물·재료 채집, 탐색, 가능한 제작·재배
      판단을 기존 물리 행동으로 연결합니다. 아직 실행 경로가 없는
      전략은 실동작 AI의 선택지에서 제외하며, 관찰용 Shadow 기능에서는 확인할 수 있습니다.
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
    {decision?.latestPersonalExperience && <p className="hint">
      AI가 실제로 참고한 최근 개인 경험 (시뮬레이션 {decision.latestPersonalExperience.minute}분):
      {' '}{decision.latestPersonalExperience.what}
    </p>}
    <section className="resident-ai-session-audit">
      <strong>주민 AI 실행 증거 감사</strong>
      <p className="hint">
        같은 시드·인구 시드·시작 시점에서 기본 Core와 로컬 AI를 각각 실행해
        측정 결과를 비교할 수 있습니다. 측정 시작 전 실행 모드를 선택하세요.
      </p>
      <button type="button" onClick={() => {
        setAuditMessage(residentAI.beginAuditSession()
          ? '현재 세계·실행 모드에서 새 측정을 시작했습니다.'
          : '세계가 준비되지 않았거나 고속 진행 중입니다.');
      }}>현재 모드 측정 시작</button>
      <button type="button" disabled={!auditReport} onClick={async () => {
        const latest = residentAI.getAuditReport();
        if (!latest) return;
        try {
          await navigator.clipboard.writeText(JSON.stringify(latest, null, 2));
          setAuditMessage('측정 JSON을 복사했습니다. 다른 실행 결과와 비교하세요.');
        } catch {
          setAuditMessage('클립보드 복사에 실패했습니다. 브라우저 권한을 확인해 주세요.');
        }
      }}>측정 JSON 복사</button>
      {auditReport && <p className="hint">
        {auditReport.mode === 'local-ai' ? '로컬 AI' : '기본 Core'} ·
        시뮬레이션 {auditReport.startMinute}~{auditReport.endMinute}분 ·
        승인 {auditReport.decisions.recorded}회 ·
        Core 문명 경험 성공 증거 증가 {auditReport.coreCivilizationSuccessEvidenceDelta}회 ·
        실패 증거 증가 {auditReport.coreCivilizationFailureEvidenceDelta}회
        {auditReport.comparableWithinObservedWindow ? '' : ' · 비교 주의: 측정 연속성 깨짐'}
      </p>}
      {auditMessage && <p role="status" className="hint">{auditMessage}</p>}
      <details>
        <summary>기본 Core / 로컬 AI 측정 결과 비교</summary>
        <p className="hint">
          먼저 기본 Core 측정 JSON을 복사한 뒤, 동일 세계 시드·인구 시드·시작 시점에서
          로컬 AI 측정을 진행하세요. 시작·종료 시각이 정확히 같아야 비교할 수 있습니다.
        </p>
        <textarea aria-label="기본 Core 측정 JSON"
          value={baselineJson} maxLength={16000} rows={3}
          placeholder="기본 Core 측정 JSON 붙여넣기"
          onChange={(event) => { setBaselineJson(event.target.value); setComparison(null); }} />
        <button type="button" onClick={() => {
          let previous: unknown;
          try { previous = JSON.parse(baselineJson); }
          catch { setAuditMessage('기본 Core 측정 JSON을 읽을 수 없습니다.'); return; }
          const result = compareResidentAISessionReports(previous, residentAI.getAuditReport());
          setComparison(result);
          setAuditMessage(result.comparable
            ? '두 관찰 구간의 기본 조건이 일치합니다.'
            : '비교 조건이 일치하지 않습니다. 아래 사유를 확인하세요.');
        }}>동일 조건 비교</button>
        {comparison && <p className="hint">
          {comparison.comparable
            ? `문명 성공 경험 증거 차이(AI - 기본) ${comparison.civilizationSuccessDifference}회, 실패 증거 차이 ${comparison.civilizationFailureDifference}회`
            : comparison.issues.join(' / ')}
          {' '}{comparison.interpretation}
        </p>}
      </details>
      <p className="hint">
        측정 중 AI 모드 변경·새 게임·고속 진행 시 비교가 제한됩니다.
        Core 경험 증거는 실제 행동의 기록이지만, AI 때문에 발생했다고 자동 귀속하지 않습니다.
        주민 이름·개인 기억 내용·모델의 판단 이유는 내보내지 않습니다.
      </p>
    </section>
    <p className="hint">
      Core 승인 기록은 행동 실행이나 목표 성공을 의미하지 않습니다.
      무료 모델은 사용자가 로컬에서 실행해야 하며 유료 클라우드 모델은 연결하지 않습니다.
    </p>
  </section>;
}
