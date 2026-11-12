import type { ResidentAIState } from './live-resident-ai';
import type { ResidentAISessionReport } from './resident-ai-session-audit';

// Single-device observation report. This is intentionally evidence-grade, not
// a causal AL-1 certificate: a Core experience arising later may be unrelated
// to the preceding Qwen suggestion.
export interface ResidentAIDeviceEvidenceReport {
  schemaVersion: 1;
  kind: 'lifelens-qwen-device-observation';
  collectedFrom: 'current-browser-session';
  modelConnected: boolean;
  modelName: string | null;
  modelLoadMs: number | null;
  inferenceAttempts: number;
  lastInferenceMs: number | null;
  maxInferenceMs: number | null;
  inferenceTimeouts: number;
  audit: ResidentAISessionReport | null;
  proof: {
    modelReady: boolean;
    coreApprovedTypedProposal: boolean;
    corePhysicalOutcomeObserved: boolean;
    actionCausedByModelProven: false;
  };
  interpretation: string;
}

function nonnegative(value: number | null): number | null {
  return value !== null && Number.isFinite(value) && value >= 0
    ? value : null;
}

export function buildResidentAIDeviceEvidenceReport(
  state: ResidentAIState,
  report: ResidentAISessionReport | null,
): ResidentAIDeviceEvidenceReport {
  const modelConnected = state.configuration?.baseUrl === 'browser-webgpu'
    && state.enabled === true && state.discovering === false;
  const observedQwenWindow = report?.mode === 'local-ai'
    && report.modelName !== null && report.modelName.trim().length > 0;
  const coreApprovedTypedProposal = observedQwenWindow
    && report.decisions.recorded > 0;
  const corePhysicalOutcomeObserved = observedQwenWindow
    && (report.coreCivilizationSuccessEvidenceDelta
      + report.corePhysicalSuccessEvidenceDelta) > 0;
  return {
    schemaVersion: 1,
    kind: 'lifelens-qwen-device-observation',
    collectedFrom: 'current-browser-session',
    modelConnected,
    modelName: report?.mode === 'local-ai'
      ? report.modelName
      : state.configuration?.model ?? null,
    modelLoadMs: nonnegative(state.modelLoadMs),
    inferenceAttempts: state.inferenceAttempts,
    lastInferenceMs: nonnegative(state.lastInferenceMs),
    maxInferenceMs: nonnegative(state.maxInferenceMs),
    inferenceTimeouts: state.inferenceTimeouts,
    audit: report === null ? null : {
      ...report, decisions: { ...report.decisions },
    },
    proof: {
      modelReady: modelConnected,
      coreApprovedTypedProposal,
      corePhysicalOutcomeObserved,
      actionCausedByModelProven: false,
    },
    interpretation: '이 JSON은 현재 브라우저의 관측 기록입니다. 모델 연결·Core 판단 승인·Core 물리 경험 증가는 서로 다른 증거이며 인과관계가 자동으로 입증되지 않습니다. 삼성 갤럭시 실기기 성공을 외부에서 확인한 기록도 아닙니다.',
  };
}
