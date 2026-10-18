import type { ResidentAISessionReport } from './resident-ai-session-audit';

// Only compare sanitized summary counters. Two matching seeds do not prove
// that the *complete* initial Core world state was identical.
export interface ResidentAIAuditComparison {
  comparable: boolean;
  issues: string[];
  baselineCivilizationSuccess: number | null;
  localCivilizationSuccess: number | null;
  civilizationSuccessDifference: number | null;
  baselineCivilizationFailure: number | null;
  localCivilizationFailure: number | null;
  civilizationFailureDifference: number | null;
  localModelRequests: number | null;
  localCoreApprovals: number | null;
  interpretation: string;
}

const integer = (value: unknown): value is number =>
  typeof value === 'number' && Number.isSafeInteger(value) && value >= 0;

function isAuditReport(raw: unknown): raw is ResidentAISessionReport {
  if (raw === null || typeof raw !== 'object' || Array.isArray(raw)) return false;
  const r = raw as Record<string, unknown>;
  if (r.schemaVersion !== 1
    ||r.kind !== 'lifelens-live-ai-observational-session'
    ||(r.mode !== 'baseline' && r.mode !== 'local-ai')
    ||typeof r.worldSeed !== 'string' ||typeof r.populationSeed !== 'string'
    ||(r.modelName !== null && typeof r.modelName !== 'string')
    ||(r.generationVersion !== null && typeof r.generationVersion !== 'number')
    ||typeof r.comparableWithinObservedWindow !== 'boolean'
    ||typeof r.coverageGap !== 'boolean'
    ||typeof r.endedReason !== 'string'
    ||!integer(r.startMinute) ||!integer(r.endMinute)
    ||!integer(r.observedRefreshes)
    ||!integer(r.readableEvidenceResidentSamples)
    ||!integer(r.modelRequests)
    ||!integer(r.coreCivilizationSuccessEvidenceDelta)
    ||!integer(r.coreCivilizationFailureEvidenceDelta)) return false;
  const decision = r.decisions;
  if (!decision || typeof decision !== 'object' || Array.isArray(decision)) return false;
  const d = decision as Record<string, unknown>;
  return ['recorded','stale','rejected','unavailable','error']
    .every(key => integer(d[key]));
}

export function compareResidentAISessionReports(
  baselineInput: unknown, candidateInput: unknown,
): ResidentAIAuditComparison {
  const baseline = isAuditReport(baselineInput) ? baselineInput : null;
  const candidate = isAuditReport(candidateInput) ? candidateInput : null;
  const issues: string[] = [];
  if (!baseline) issues.push('기본 Core 측정 JSON 형식이 올바르지 않습니다.');
  if (!candidate) issues.push('현재 로컬 AI 측정 기록이 없거나 형식이 올바르지 않습니다.');
  if (baseline && candidate) {
    if (baseline.mode !== 'baseline' || candidate.mode !== 'local-ai')
      issues.push('첫 번째 기록은 기본 Core, 두 번째 기록은 실제 로컬 AI 모드여야 합니다.');
    if (baseline.worldSeed !== candidate.worldSeed
      ||baseline.populationSeed !== candidate.populationSeed
      ||baseline.generationVersion !== candidate.generationVersion)
      issues.push('시드·인구 시드·세계 생성 버전이 서로 다릅니다.');
    if (baseline.startMinute !== candidate.startMinute
      ||baseline.endMinute !== candidate.endMinute)
      issues.push('시뮬레이션 측정 시작·종료 분이 서로 다릅니다.');
    if (baseline.endMinute <= baseline.startMinute
      ||candidate.endMinute <= candidate.startMinute)
      issues.push('두 기록 모두 시작 이후 실제 시뮬레이션 시간이 흘러야 합니다.');
    if (baseline.coverageGap ||candidate.coverageGap
      ||!baseline.comparableWithinObservedWindow
      ||!candidate.comparableWithinObservedWindow)
      issues.push('측정 도중 고속 진행·모드 변경 등으로 관측 연속성이 깨졌습니다.');
    if (baseline.readableEvidenceResidentSamples === 0
      ||candidate.readableEvidenceResidentSamples === 0)
      issues.push('Core 개인 경험 증거를 읽을 수 있는 주민 표본이 없습니다.');
    if (baseline.modelRequests !== 0)
      issues.push('기본 Core 기록에 모델 요청이 포함되어 있습니다.');
    if (candidate.modelRequests === 0 ||candidate.decisions.recorded === 0)
      issues.push('로컬 AI 측정에 Core가 승인한 모델 판단이 없습니다.');
    if (candidate.modelName === null ||candidate.modelName.trim() === '')
      issues.push('로컬 모델 이름이 기록되지 않아 실제 모델 실행을 확인할 수 없습니다.');
  }
  return {
    comparable: issues.length === 0,
    issues,
    baselineCivilizationSuccess: baseline?.coreCivilizationSuccessEvidenceDelta ?? null,
    localCivilizationSuccess: candidate?.coreCivilizationSuccessEvidenceDelta ?? null,
    civilizationSuccessDifference: baseline && candidate
      ? candidate.coreCivilizationSuccessEvidenceDelta -
        baseline.coreCivilizationSuccessEvidenceDelta : null,
    baselineCivilizationFailure: baseline?.coreCivilizationFailureEvidenceDelta ?? null,
    localCivilizationFailure: candidate?.coreCivilizationFailureEvidenceDelta ?? null,
    civilizationFailureDifference: baseline && candidate
      ? candidate.coreCivilizationFailureEvidenceDelta -
        baseline.coreCivilizationFailureEvidenceDelta : null,
    localModelRequests: candidate?.modelRequests ?? null,
    localCoreApprovals: candidate?.decisions.recorded ?? null,
    interpretation: '동일 관찰 창의 경험 증거 차이만 비교합니다. 같은 시드라도 초기 Core 상태 전체의 일치를 보증하지 않으며, 차이가 AI 제안 때문에 발생했다는 인과 증거는 아닙니다.',
  };
}
