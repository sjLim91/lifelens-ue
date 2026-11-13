import { useEffect, useState, useSyncExternalStore } from 'react';
import type { CognitiveCausalJournal, CognitiveTraceStep } from '../cognition/cognitive-causal-journal';
import { residentAI } from '../cognition/live-resident-ai';
import type { CognitiveIntent } from '../cognition/cognitive-contract';
import type { Resident } from '../runtime/core-types';
import { useObserverSnapshot } from '../state/use-observer-snapshot';
import { formatCognitiveIntent, formatResidentName, formatCivilizationIntent, formatSocialIntent, formatMaterial, formatParentingAction, formatTechnique } from '../localization/korean';
import { formatDay } from './observer-format';
import { residentStatusText } from './resident-status';
import { deriveResidentCausalEvidence, GOAL_OUTCOME_LABELS } from './resident-causal-evidence';

function LegacyCausalTimelineContents({ resident, residents }: {
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

const RESULT_LABELS = ['발행됨 · 결과 대기', '행동 실행 성공', '행동 실행 실패', '실행 결과 확인 전 종료', '유효 시간 만료', '경로 이동 실패', '생존을 위해 중단'];
const REJECTION_LABELS = ['', '현재 조건에서 허용되지 않음', '관찰 정보 유효 기간 만료', '중복 승인 차단'];
function stepLabel(step: CognitiveTraceStep): string {
  if (step.kind === 1) return formatSocialIntent(step.action);
  if (step.kind === 2) return formatCivilizationIntent(step.action)
    + (step.material && step.material !== 'Unknown' ? ` · ${formatMaterial(step.material)}` : '');
  return step.kind === 3 ? formatParentingAction(step.action) : step.kind === 4
    ? '지식 전수' + (step.technique && step.technique !== 'None' ? ` · ${formatTechnique(step.technique)}` : '') : '물품 교환';
}
function ConnectedCausalTimeline(props: { resident: Resident; residents: Resident[] }) {
  const { world } = useObserverSnapshot();
  const actor = props.resident.id;
  const seed = world.worldSeed, population = world.populationSeed, generation = world.generationVersion;
  const [journal, setJournal] = useState<CognitiveCausalJournal | null | undefined>(undefined);
  useEffect(() => {
    const read = () => {
      const next = residentAI.inspectCoreCausalJournal(actor);
      setJournal(next && next.worldSeed === String(seed) && next.populationSeed === String(population)
        && next.generationVersion === generation ? next : null);
    };
    setJournal(undefined); read();
    const timer = window.setInterval(read, 1000);
    return () => window.clearInterval(timer);
  }, [actor, seed, population, generation]);
  if (journal === undefined || journal && (journal.actor !== actor || journal.worldSeed !== String(seed)
    || journal.populationSeed !== String(population) || journal.generationVersion !== generation)) return <p className="hint">Core의 연결 기록을 읽고 있습니다.</p>;
  if (journal === null) return <>
    <p className="hint">전체 연결 기록을 읽을 수 없어 이전의 개별 관찰을 표시합니다.</p>
    <LegacyCausalTimelineContents {...props} />
  </>;
  const name = (id: string) => {
    const target = props.residents.find(r => r.id === id);
    return target ? formatResidentName(target.name) : '상대 미확인';
  };
  return <div aria-label="생각과 실제 행동 기록">
    {!journal.records.length && <p className="hint">아직 Core에 기록된 판단 제안이 없습니다. 제안 승인만으로 행동이나 성공을 만들지 않습니다.</p>}
    {journal.records.map(record => <article className="causal-record" key={record.id}>
      <time>{formatDay(record.observedMinute)} · 판단 #{record.id}</time>
      <ol>
        <li><span>판단 제안</span><strong>{record.intent === 'None' ? '유효한 의도가 없는 제안' : formatCognitiveIntent(record.intent)}</strong>
          {record.target !== '0' && <small>상대 · {name(record.target)}</small>}</li>
        <li><span>Core 검증</span><strong>{record.accepted ? '승인됨' : REJECTION_LABELS[record.rejection]}</strong>
          <small>{formatDay(record.minute)}</small></li>
      </ol>
      {record.accepted && !record.steps.length && <p className="hint">{record.selectionState === 2 ? '더 최근의 판단으로 대체되어 새 행동 연결이 없습니다.' : record.selectionState === 3
        ? '행동 선택에 반영되는 기간이 지났으며 새 행동 연결이 없습니다.' : '승인 후 이 판단과 연결되어 발행된 행동은 아직 없습니다.'}</p>}
      {record.steps.map((step, index) => <details className="causal-step" key={step.token === '0' ? 'fact:' + step.exchangeFact : step.token} open={index >= record.steps.length - 2}>
        <summary>{step.token === '0' ? `교환 사실 #${step.exchangeFact}` : `행동 #${step.token}`} · {stepLabel(step)}</summary>
        <ol>
          <li><span>연결 근거</span><strong>{step.source === 0 ? '이 판단이 실제 계획에서 선택됨'
            : step.source === 1 ? '기존 후보 평가에 이 판단의 선호가 반영됨'
            : `행동 #${step.previousToken}에서 이어진 실제 단계`}</strong>
            <small>{formatDay(step.issuedMinute)} · {step.exchangeFact !== '0' ? '등록된 Core 교환 사실' : 'Core 발행 행동 번호'}</small></li>
          <li><span>실제 결과</span><strong>{RESULT_LABELS[step.result]}</strong>
            {step.resultMinute >= 0 && <small>{step.result === 3 && '종료 확인 · '}{formatDay(step.resultMinute)}{!step.attempted && ' · 실행 성공으로 계산하지 않음'}</small>}</li>
          <li><span>학습</span><strong>{step.learning.changed ? '직접 경험으로 믿음 갱신'
            : step.result === 0 ? '실행 결과 이후 확인' : '이번 단계의 새로운 학습 증거 없음'}</strong>
            {step.learning.changed && <small>경험 증거 {step.learning.beforeCount} → {step.learning.afterCount}
              {' · '}믿음 {step.learning.beforeStance.toFixed(2)} → {step.learning.afterStance.toFixed(2)}</small>}</li>
        </ol>
      </details>)}
      {record.omittedSteps > 0 && <p className="hint">이 판단의 이전 단계 {record.omittedSteps}개는 보관 범위를 벗어났습니다.</p>}
    </article>)}
    {journal.olderRecords > 0 && <p className="hint">이전 판단 {journal.olderRecords}개는 현재 표시 범위 밖에 있습니다.</p>}
    <p className="hint">각 단계는 Core가 실제로 사용한 제안과 행동 번호로 연결합니다. 행동 실행 성공과 제안 전체 목표의 달성은 따로 판단합니다. 최근 판단 64개를 보관하고, 이 주민의 최근 8개를 표시합니다.</p>
  </div>;
}

export function ResidentCausalTimeline(props: { resident: Resident; residents: Resident[] }) {
  const [open, setOpen] = useState(false);
  return <details className="resident-causal-timeline" onToggle={event => setOpen(event.currentTarget.open)}>
    <summary>생각과 실제 행동</summary>
    {open && <ConnectedCausalTimeline {...props} />}
  </details>;
}
