import { useSyncExternalStore } from 'react';
import type { CognitiveIntent } from '../cognition/cognitive-contract';
import { residentAI } from '../cognition/live-resident-ai';
import type { Resident } from '../runtime/core-types';
import { formatCognitiveIntent } from '../localization/korean';

// ONE resident AI per person. This panel observes its real thought handoff;
// it does not create a second agent, start a model, or let the viewer turn a
// character's mind on/off. Simulation boot owns automatic model discovery.
export function ResidentAILivePanel({ resident }: { resident: Resident }) {
  const state = useSyncExternalStore(
    residentAI.subscribe, residentAI.getSnapshot, residentAI.getSnapshot,
  );
  const decision = state.byResident[resident.id];
  const localInference = state.configuration;
  const auditReport = residentAI.getAuditReport();

  return <section className="resident-ai-live-config">
    <strong>주민 AI · 항상 실행</strong>
    <p className="hint">
      주민마다 고유한 기억·성격·관계·경험을 가진 하나의 사고 주체가
      목표와 행동을 결정합니다. 세계의 물리·이동·자원·건강 결과는
      Core가 검증합니다. 실행 중인 주민 AI를 켜거나 끄는 조작은 없습니다.
    </p>
    <p role="status">
      주민 AI 실행 중 · 무료 로컬 추론 {' '}
      {localInference
        ? `연결됨 (${localInference.model})`
        : state.discovering
          ? '자동 검색 중'
          : '연결 대기 (주민 생활은 계속됨)'}
    </p>
    <p className="hint">
      인지 제안 Core 승인 {state.accepted}회 · 거부/오류 {state.rejected}회
    </p>
    {decision && <p className="hint">
      현재 주민의 최근 판단: {decision.intent
        ? formatCognitiveIntent(decision.intent as CognitiveIntent) : '없음'}
      {' · '}{decision.status === 'recorded' ? 'Core 기록 승인'
        : decision.status === 'stale' ? '시효 만료'
        : decision.status === 'unavailable' ? '추론 응답 없음'
        : decision.status === 'error' ? '추론 오류' : 'Core 거부'}
    </p>}
    {decision?.latestPersonalExperience && <p className="hint">
      최근 참고한 실제 경험 ({decision.latestPersonalExperience.minute}분):
      {' '}{decision.latestPersonalExperience.what}
    </p>}
    <details className="resident-ai-session-audit">
      <summary>주민 AI 실행 증거</summary>
      <p className="hint">
        모델이 의견을 제안한 것과 실제 Core에서 행동이 실행된 것은
        다릅니다. 아래 수치는 관찰 기록이며 계획 성공을 보장하지 않습니다.
      </p>
      {auditReport
        ? <p className="hint">
            관찰 {auditReport.startMinute}~{auditReport.endMinute}분 ·
            Core 판단 승인 {auditReport.decisions.recorded}회 ·
            문명 성공 경험 증가 {auditReport.coreCivilizationSuccessEvidenceDelta}회 ·
            실패 경험 증가 {auditReport.coreCivilizationFailureEvidenceDelta}회
          </p>
        : <p className="hint">현재 기록된 실행 감사 구간 없음</p>}
      <p className="hint">
        무료 로컬 언어 추론 엔진을 찾지 못하면 해당 추론은 연결 대기 상태로 표시합니다.
        브라우저만으로 외부 네이티브 모델 서버가 자동 설치·실행되지는 않습니다.
        이때에도 같은 주민의 Core 생존과 경험은 중단되지 않습니다.
      </p>
    </details>
  </section>;
}
