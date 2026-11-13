import { useState, useSyncExternalStore } from 'react';
import { residentAI } from '../cognition/live-resident-ai';
import type { CognitiveIntent } from '../cognition/cognitive-contract';
import type { Resident } from '../runtime/core-types';
import { useObserverSnapshot } from '../state/use-observer-snapshot';
import { formatCognitiveIntent, formatResidentName } from '../localization/korean';
import { formatDay } from './observer-format';
import { residentStatusText } from './resident-status';
import { deriveResidentCausalEvidence, GOAL_OUTCOME_LABELS } from './resident-causal-evidence';

function CausalTimelineContents({ resident, residents }: {
  resident: Resident; residents: Resident[];
}) {
  const state = useSyncExternalStore(residentAI.subscribe, residentAI.getSnapshot, residentAI.getSnapshot);
  const { world } = useObserverSnapshot();
  const { thought, attempt, completed } = deriveResidentCausalEvidence(
    resident, state.byResident[resident.id], Number(world.minute) || 0,
  );
  const name = (id?: string) => {
    const target = residents.find(r => r.id === id);
    return target ? formatResidentName(target.name) : '상대 미확인';
  };
  return <div aria-label="생각과 실제 행동 기록">
    {thought ? <div className="causal-record">
      <time>{formatDay(thought.minute)}</time>
      <ol>
        <li><span>판단 제안</span><strong>{thought.intent
          ? formatCognitiveIntent(thought.intent as CognitiveIntent) : '제안 내용 없음'}</strong>
          {thought.target && <small>상대 · {name(thought.target)}</small>}</li>
        <li><span>제안 검증</span><strong>{thought.status === 'recorded' ? 'Core가 제안을 기록함'
          : thought.status === 'rejected' ? 'Core가 제안을 차단함'
          : thought.status === 'stale' ? '관찰 정보 만료'
          : thought.status === 'error' ? '추론 오류' : '추론 응답 대기'}</strong></li>
      </ol>
      <p className="hint">이 제안과 실제 행동을 연결하는 기록은 아직 없습니다.</p>
    </div> : <p className="hint">현재 세션에서 이 주민의 판단 제안이 기록되지 않았습니다.</p>}
    {attempt && <div className="causal-record">
      <time>{formatDay(attempt.minute)} · 행동 #{attempt.token}</time>
      <ol><li><span>실제 시도</span><strong>{residentStatusText(resident, residents)}</strong>
        <small>{attempt.phase === 'Moving' ? '목적지로 이동 중' : '현장에서 실행 중'} · 결과 대기</small>
      </li></ol>
    </div>}
    {completed.map(record => <div className="causal-record" key={record.token}>
      <time>{formatDay(record.minute)} · 행동 #{record.token}</time>
      <ol>
        <li><span>실제 실행</span><strong>{name(record.target)} · {record.intent === 'Approach' ? '다가가기' : '관계 회복'}</strong>
          <small>행동 번호가 일치하는 직접 경험</small></li>
        <li><span>결과</span><strong>{GOAL_OUTCOME_LABELS[record.outcome]}</strong></li>
        <li><span>학습</span><strong>{record.learned ? '자신의 신뢰 변화에 대한 믿음 갱신'
          : record.outcome === 'already-reached-before-action' ? '이미 달성한 목표 · 학습 보상 없음'
          : '이 행동과 일치하는 믿음 갱신 기록 없음'}</strong></li>
      </ol>
    </div>)}
    {!attempt && !completed.length && <p className="hint">행동 번호로 확인할 수 있는 시도·결과 기록이 아직 없습니다.</p>}
    <p className="hint">제안 승인, 실제 실행, 목표 달성을 각각 구분합니다. 최근 확인된 행동을 최대 6개 표시합니다.</p>
  </div>;
}

export function ResidentCausalTimeline(props: { resident: Resident; residents: Resident[] }) {
  const [open, setOpen] = useState(false);
  return <details className="resident-causal-timeline" onToggle={event => setOpen(event.currentTarget.open)}>
    <summary>생각과 실제 행동</summary>
    {open && <CausalTimelineContents {...props} />}
  </details>;
}
