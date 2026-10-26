import { useSyncExternalStore } from 'react';
import type { CognitiveIntent } from '../cognition/cognitive-contract';
import { residentAI, residentAIStatusText } from '../cognition/live-resident-ai';
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
    <strong>주민 AI · Qwen 단일 모델</strong>
    <p className="hint">
      Qwen이 주민마다 고유한 기억·성격·관계·경험을 받아 판단을 제안합니다.
      실제 행동과 세계의 물리·이동·자원·건강 결과는 Core가 검증하고 실행합니다.
      모델이 준비되지 않으면 언어 추론은 대기하며 Core의 기본 생존 처리는 계속됩니다.
    </p>
    <p role="status">{residentAIStatusText(state)}</p>
    {localInference && <p className="hint">사용 모델: {localInference.model}</p>}
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
        Qwen 모델을 이 기기에 다운로드하여 실행합니다. 모델 호출 요금은 없으며
        첫 다운로드와 GPU 메모리가 필요합니다. WebGPU 미지원·메모리 부족·오류 시
        다른 AI로 전환하지 않고 Qwen 연결 대기로 표시합니다.
      </p>
    </details>
  </section>;
}

