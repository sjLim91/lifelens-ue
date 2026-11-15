import type { Resident } from '../runtime/core-types';
import { formatTechnique, formatMaterial } from '../localization/korean';
import { formatDay } from './observer-format';

const STATES = ['진행 중', '생존 행동으로 잠시 중단', '목표 달성', '계획 종료'];
const REASONS = ['', '긴급한 생존 행동 우선', '직접 도착한 자원이 고갈됨', '직접 도착한 원천이 사라짐',
  '실제 접근 경로를 찾지 못함', '물을 운반할 용기 없음', '실제 재료 부족', '개인 제작 지식 부족',
  '대상 없음', '실행 조건을 충족하지 못함', '질병으로 진행 불가', '원인 미확인', '원래 기한 만료',
  '최대 6단계 예산 소진', '새 목표를 선택함', '다른 경로로 필요한 물건을 확보함', '주민 사망', '실행 가능한 다음 단계 없음'];
const RESULTS = ['실행 중', '실제 성공', '실제 실패', '취소', '기한 만료', '경로 실패', '생존을 위해 중단'];
const ACTIONS: Record<string, string> = { Gather: '재료 확보', Craft: '제작', Experiment: '실험' };

export function ResidentProductionPanel({ resident }: { resident: Resident }) {
  const plan = resident.civilization?.productionPlan;
  return <details className="resident-mind-panel">
    <summary>이어가는 제작 목표 <span>{plan ? STATES[plan.state] ?? '상태 미확인' : '기록 없음'}</span></summary>
    {!plan ? <p className="hint">아직 구체적인 제작 목표를 시작하지 않았습니다.</p> : <>
      <p>{formatTechnique(plan.target)} {plan.targetStock}개 확보</p>
      <p className="hint">{plan.cognitiveOrigin ? '수락된 생각에서 선택한 목표' : '실제 생활 판단에서 선택한 목표'} · 목표 #{plan.id}</p>
      <p className="hint">시작 {formatDay(plan.createdMinute)} · 원래 기한 {formatDay(plan.expiresMinute)} · 실제 단계 {plan.steps.length}/6</p>
      {plan.reason > 0 && <p>{REASONS[plan.reason] ?? '원인 미확인'}</p>}
      <ol aria-label="실제 제작 계획 단계">
        {plan.steps.map(step => <li key={step.token}>
          <b>{ACTIONS[step.action] ?? '실제 작업'} · {step.action === 'Gather' ? formatMaterial(step.material) : formatTechnique(step.technique)}</b>
          <p className="hint">{RESULTS[step.result] ?? '결과 미확인'} · 명령 #{step.token} · {formatDay(step.issuedMinute)}</p>
          {step.attempted && <p className="hint">실측 보유량 {step.stockBefore} → {step.stockAfter}</p>}
          {step.reason > 0 && <p className="hint">{REASONS[step.reason] ?? '원인 미확인'}</p>}
        </li>)}
      </ol>
    </>}
  </details>;
}
