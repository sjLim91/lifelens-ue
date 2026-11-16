import { useState } from 'react';
import type { Resident } from '../runtime/core-types';
import type { AutonomousGoalPreview } from '../cognition/autonomous-goal-preview';
import { shadowObserver } from '../cognition/shadow-observer';
import { useObserverSnapshot } from '../state/use-observer-snapshot';
import { formatDay, formatPercent, formatResidentName } from './observer-format';
import { residentStatusText } from './resident-status';
import { GOAL_EVIDENCE_LABELS, goalConditionText, residentPersonalityFacts } from './resident-observation';

export function ResidentMindPanel({ resident, residents }: { resident: Resident; residents: Resident[] }) {
  const { world } = useObserverSnapshot();
  const worldKey = `${world.worldSeed}:${world.populationSeed}:${world.generationVersion}`;
  const [observation, setObservation] = useState<{ worldKey: string; preview: AutonomousGoalPreview | null } | null>(null);
  const preview = observation?.worldKey === worldKey && observation.preview?.actor === resident.id
    && observation.preview.minute <= Number(world.minute) ? observation.preview : null;
  const facts = residentPersonalityFacts(resident);
  const p = resident.presentation;
  const mind = resident.mind;
  const domains: Record<string, string> = { resource: '자원', tool: '도구', person: '사람', action: '행동', ordinary: '일상', danger: '위험', animal: '동물', place: '장소', trace: '흔적' };
  const sources: Record<string, string> = { DirectWitness: '직접 경험', ToldByOther: '전해 들음', Inferred: '추정' };
  const active = resident.alive === true && p?.active === true && p.phase !== 'Idle';
  const target = residents.find(r => r.id === p?.targetResidentId);
  const inspect = () => setObservation({ worldKey, preview: shadowObserver.inspectCoreAutonomousGoals(resident.id) });
  return <>
    <details className="resident-mind-panel">
      <summary>성격 <span>{facts.length > 0 ? `${facts.length}가지 특성` : '관찰 기록 없음'}</span></summary>
      <div className="focused-life-chips">
        {facts.map(fact => <span key={fact.key}>{fact.label} <b>{formatPercent(fact.value)}</b></span>)}
      </div>
      {!facts.length && <p className="hint">아직 관찰된 성격 수치가 없습니다.</p>}
    </details>
    <details className="resident-mind-panel" onToggle={e => { if (e.currentTarget.open) inspect(); }}>
      <summary>목표와 계획</summary>
      <section aria-label="현재 실행 계획">
        <h3>현재 실행 계획</h3>
        <p><b>{active ? residentStatusText(resident, residents) : '현재 발행된 실행 계획 없음'}</b></p>
        {active && <p className="hint">{p.phase === 'Moving' ? '목적지로 이동 중' : '현장에서 행동 중'}
          {target && ` · 상대 ${formatResidentName(target.name)}`}
          {p.hasTargetGrid && ` · 목적지 ${p.targetGridX}, ${p.targetGridY}`}
          {p.contextActionToken && p.contextActionToken !== '0' && ` · 행동 #${p.contextActionToken}`}
        </p>}
      </section>
      <section aria-label="다음 목표 후보">
        <h3>다음 목표 후보</h3>
        <p className="hint">현재 욕구·기억·믿음에서 관찰한 후보입니다. 승인되거나 실행된 계획을 뜻하지 않습니다.</p>
        {preview ? <>
          <p className="hint">{formatDay(preview.minute)} 기준</p>
          {preview.candidates.length ? <ol className="resident-goal-candidates">
            {preview.candidates.map((goal, index) => <li key={index}>
              <strong>{goal.conditions.map(c => goalConditionText(c, residents)).join(' · ')}</strong>
              <small>우선순위 {formatPercent(goal.priority)} · 유효 시각 {formatDay(goal.expiresMinute)}</small>
              <small>근거 · {[...new Set(goal.evidence.map(e => GOAL_EVIDENCE_LABELS[e.kind]))].join(' · ')}</small>
            </li>)}
          </ol> : <p className="hint">현재 관찰된 추가 목표 후보가 없습니다.</p>}
        </> : <p className="hint" role="status">{observation === null
          ? '목표 후보를 확인하고 있습니다.'
          : '현재 실행 중인 월드에서 이 주민의 목표 후보를 읽을 수 없습니다.'}</p>}
        <button type="button" disabled={resident.alive !== true} onClick={inspect}>목표 후보 새로 확인</button>
      </section>
    </details>
    {mind && <details className="resident-mind-panel">
      <summary>경험과 마음 <span>{mind.eligibleEpisodes}건의 개인 근거</span></summary>
      <div className="focused-life-chips">
        {([['호기심', mind.curiosity], ['습관', mind.habit], ['장소 애착', mind.attachment], ['위험 감수', mind.riskTolerance], ['경험한 상대 신뢰', mind.trust]] as const)
          .map(([label, value]) => <span key={label}>{label} <b>{formatPercent(value)}</b></span>)}
      </div>
      <p className="hint">기억의 출처를 구분한 개인 판단입니다. 현재 자원의 잔량이나 행동 성공을 보장하지 않습니다.</p>
      {mind.summaries.length ? <ul>{mind.summaries.map(item => <li key={`${item.domain}:${item.source}:${item.subject}`}>
        <b>{domains[item.domain] ?? '기억'} · {sources[item.source]}</b> {item.episodes}건
        {(item.successes > 0 || item.failures > 0) && ` · 실제 성공 ${item.successes} / 실패 ${item.failures}`}
        <small> · {formatDay(item.lastMinute)}까지</small>
      </li>)}</ul> : <p className="hint">아직 정리할 개인 경험이 없습니다.</p>}
      {mind.omittedSummaries > 0 && <p className="hint">추가 {mind.omittedSummaries}건의 원래 기억은 저장되어 있습니다.</p>}
      {mind.places.length > 0 && <p className="hint">기억한 장소 {mind.places.length}곳 · 직접 경험과 추정을 구분하여 보관합니다.</p>}
    </details>}
  </>;
}
