import { useMemo, useState } from 'react';
import { observerActions } from '../state/observer-actions';
import {
  MAX_FAST_FORWARD_DAYS,
  MINUTES_PER_DAY,
  type FastForwardState,
  type FastForwardSummary,
} from '../state/fast-forward';
import {
  formatDay,
  formatFacilityKind,
  formatFacilityState,
  formatLifeEvent,
  formatMaterial,
  formatTechnique,
} from './observer-format';

function signed(value: number): string {
  if (value > 0) return `+${Math.round(value)}`;
  if (value < 0) return `${Math.round(value)}`;
  return '±0';
}

function changeLabel(before: number, after: number): string {
  return `${Math.round(before)} → ${Math.round(after)} (${signed(after - before)})`;
}

function Summary({
  summary,
  onClose,
}: {
  summary: FastForwardSummary;
  onClose: () => void;
}) {
  const resourceChanges = summary.resourceChanges.slice(0, 6);
  const facilityChanges = summary.facilityChanges.slice(0, 8);
  const discoveries = summary.newDiscoveries.slice(0, 8);
  const lifeEvents = summary.lifeEvents.slice(-10);

  return (
    <div className="fast-forward-summary">
      <div className="fast-forward-summary-head">
        <div>
          <strong>{summary.requestedDays}일 진행 완료</strong>
          <small>
            {formatDay(summary.startMinute)} → {formatDay(summary.endMinute)}
          </small>
        </div>
        <button type="button" onClick={onClose} aria-label="건너뛰기 결과 닫기">
          닫기
        </button>
      </div>

      <div className="fast-forward-summary-grid">
        <span>전체 인구 <b>{changeLabel(summary.totalResidentsBefore, summary.totalResidentsAfter)}</b></span>
        <span>생존 인구 <b>{changeLabel(summary.livingResidentsBefore, summary.livingResidentsAfter)}</b></span>
        <span>사망 누계 <b>{changeLabel(summary.deceasedResidentsBefore, summary.deceasedResidentsAfter)}</b></span>
        <span>가구 <b>{changeLabel(summary.householdsBefore, summary.householdsAfter)}</b></span>
        <span>커플 <b>{changeLabel(summary.couplesBefore, summary.couplesAfter)}</b></span>
        <span>임신 <b>{changeLabel(summary.pregnanciesBefore, summary.pregnanciesAfter)}</b></span>
        <span>시설 <b>{changeLabel(summary.facilitiesBefore, summary.facilitiesAfter)}</b></span>
        <span>지식 사실 <b>{changeLabel(summary.techniqueFactsBefore, summary.techniqueFactsAfter)}</b></span>
        <span>저장 자원 <b>{changeLabel(summary.storedUnitsBefore, summary.storedUnitsAfter)}</b></span>
        <span>주요 생애사건 <b>{changeLabel(summary.majorLifeEventsBefore, summary.majorLifeEventsAfter)}</b></span>
      </div>

      {summary.newResidents.length > 0 && (
        <div className="fast-forward-change-group">
          <h4>새로 태어난 주민</h4>
          <div className="fast-forward-chips">
            {summary.newResidents.slice(0, 10).map((resident) => (
              <span key={resident.id}>{resident.name}</span>
            ))}
          </div>
        </div>
      )}

      {summary.newlyDeceased.length > 0 && (
        <div className="fast-forward-change-group">
          <h4>사망한 주민</h4>
          <div className="fast-forward-chips">
            {summary.newlyDeceased.slice(0, 10).map((resident) => (
              <span key={resident.id}>{resident.name}</span>
            ))}
          </div>
        </div>
      )}

      {facilityChanges.length > 0 && (
        <div className="fast-forward-change-group">
          <h4>시설 변화</h4>
          {facilityChanges.map((change) => (
            <div className="fast-forward-change-row" key={change.id}>
              <span>{formatFacilityKind(change.kind)}</span>
              <b>
                {change.fromState
                  ? `${formatFacilityState(change.fromState)} → ${formatFacilityState(change.toState)}`
                  : `새 시설 · ${formatFacilityState(change.toState)}`}
              </b>
            </div>
          ))}
          {summary.facilityChanges.length > facilityChanges.length && (
            <small>외 {summary.facilityChanges.length - facilityChanges.length}건</small>
          )}
        </div>
      )}

      {discoveries.length > 0 && (
        <div className="fast-forward-change-group">
          <h4>새로운 발견</h4>
          {discoveries.map((discovery) => (
            <div className="fast-forward-change-row" key={discovery.factId}>
              <span>{formatTechnique(discovery.technique)}</span>
              <b>{discovery.discovererName || '주민'} · {formatDay(discovery.minute)}</b>
            </div>
          ))}
        </div>
      )}

      {lifeEvents.length > 0 && (
        <div className="fast-forward-change-group">
          <h4>주요 삶의 변화</h4>
          {lifeEvents.map((event, index) => (
            <div
              className="fast-forward-change-row"
              key={`${event.residentId}:${event.minute}:${event.type}:${index}`}
            >
              <span>{event.residentName}</span>
              <b>{formatLifeEvent(event.type)} · {formatDay(event.minute)}</b>
            </div>
          ))}
        </div>
      )}

      {resourceChanges.length > 0 && (
        <div className="fast-forward-change-group">
          <h4>자연 자원 변화</h4>
          {resourceChanges.map((change) => (
            <div className="fast-forward-change-row" key={change.material}>
              <span>{formatMaterial(change.material)}</span>
              <b>{Math.round(change.before)} → {Math.round(change.after)} ({signed(change.delta)})</b>
            </div>
          ))}
        </div>
      )}

      {summary.sanitationSitesBefore !== summary.sanitationSitesAfter && (
        <div className="fast-forward-change-group">
          <h4>위생 환경</h4>
          <div className="fast-forward-change-row">
            <span>위생 장소</span>
            <b>{changeLabel(summary.sanitationSitesBefore, summary.sanitationSitesAfter)}</b>
          </div>
        </div>
      )}

      {summary.newResidents.length === 0
        && summary.newlyDeceased.length === 0
        && summary.facilityChanges.length === 0
        && summary.newDiscoveries.length === 0
        && summary.lifeEvents.length === 0
        && summary.resourceChanges.length === 0 && (
          <p className="hint">
            이 기간에는 현재 관찰 가능한 주요 변화가 없었습니다.
          </p>
      )}
    </div>
  );
}

export function FastForwardControl({
  state,
  disabled,
}: {
  state: FastForwardState;
  disabled: boolean;
}) {
  const [daysText, setDaysText] = useState('30');
  const [inputError, setInputError] = useState('');

  const requestedDays = Number(daysText);
  const running = state.status === 'running';
  const completedDays = state.completedMinutes / MINUTES_PER_DAY;
  const progressPercent = Math.round(state.progress01 * 100);

  const progressText = useMemo(() => {
    if (!running) return '';
    const whole = Math.floor(completedDays);
    const partial = Math.round((completedDays - whole) * 24);
    return partial > 0
      ? `${whole}일 ${partial}시간 / ${state.requestedDays}일 계산 중…`
      : `${whole}일 / ${state.requestedDays}일 계산 중…`;
  }, [completedDays, running, state.requestedDays]);

  const start = (): void => {
    const days = Math.floor(requestedDays);
    if (
      !Number.isFinite(days)
      || days < 1
      || days > MAX_FAST_FORWARD_DAYS
    ) {
      setInputError(`1일부터 ${MAX_FAST_FORWARD_DAYS}일까지 입력해 주세요.`);
      return;
    }
    setInputError('');
    void observerActions.fastForwardDays(days);
  };

  return (
    <div className="fast-forward-control">
      <div className="fast-forward-title">
        <strong>일수 건너뛰기</strong>
        <small>입력 기간을 실제 Core 규칙으로 고속 계산합니다.</small>
      </div>

      <div className="fast-forward-input-row">
        <label>
          <span className="sr-only">건너뛸 일수</span>
          <input
            type="number"
            inputMode="numeric"
            min={1}
            max={MAX_FAST_FORWARD_DAYS}
            step={1}
            value={daysText}
            disabled={disabled || running}
            onChange={(event) => {
              setDaysText(event.target.value);
              if (inputError) setInputError('');
            }}
            onKeyDown={(event) => {
              if (event.key === 'Enter' && !disabled && !running) start();
            }}
            aria-label="건너뛸 일수"
          />
        </label>
        <span>일</span>
        <button
          type="button"
          onClick={start}
          disabled={disabled || running}
        >
          {running ? '계산 중…' : '이만큼 진행'}
        </button>
      </div>

      {inputError && <p className="field-error">{inputError}</p>}
      {state.status === 'error' && (
        <p className="field-error">
          {state.errorMessage || '고속 진행 중 오류가 발생했습니다.'}
        </p>
      )}

      {running && (
        <div className="fast-forward-progress" aria-live="polite">
          <progress max={100} value={progressPercent} />
          <div>
            <span>{progressText}</span>
            <b>{progressPercent}%</b>
          </div>
          <small>화면 렌더링은 멈추고 세계의 실제 변화만 계산하고 있습니다.</small>
        </div>
      )}

      {state.status === 'complete' && state.summary && (
        <Summary
          summary={state.summary}
          onClose={() => observerActions.clearFastForwardResult()}
        />
      )}
    </div>
  );
}
