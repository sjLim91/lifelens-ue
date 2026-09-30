import type { ObservationEvent } from '../state/observation-feed';
import { formatDay } from './observer-format';

function kindLabel(kind: ObservationEvent['kind']): string {
  switch (kind) {
    case 'family': return '가족';
    case 'memory': return '기억';
    case 'relationship': return '관계';
    case 'social': return '사회';
    case 'civilization': return '문명';
    case 'facility': return '시설';
    case 'sanitation': return '위생';
    case 'life': return '생애';
    default: return '행동';
  }
}

function ObservationEventButton({
  event,
  onFocus,
  compact = false,
}: {
  event: ObservationEvent;
  onFocus: (event: ObservationEvent) => void;
  compact?: boolean;
}) {
  const content = (
    <>
      <div className="observation-event-heading">
        <span className={`observation-kind ${event.importance}`}>
          {kindLabel(event.kind)}
        </span>
        <small>{formatDay(event.minute)}</small>
      </div>
      <strong>{event.summary}</strong>
      {!compact && event.detail
        ? <span className="observation-event-detail">{event.detail}</span>
        : null}
    </>
  );

  const focusable = Boolean(event.residentId)
    || (
      event.focusGridX !== undefined
      && event.focusGridY !== undefined
    );
  if (!focusable) {
    return (
      <div className={`observation-event ${compact ? 'compact' : ''}`}>
        {content}
      </div>
    );
  }

  return (
    <button
      type="button"
      className={`observation-event observation-event-button ${compact ? 'compact' : ''}`}
      onClick={() => onFocus(event)}
      aria-label={`${event.summary} 사건 현장 보기`}
    >
      {content}
    </button>
  );
}

export function ObservationFeedOverlay({
  observations,
  onFocus,
}: {
  observations: ObservationEvent[];
  onFocus: (event: ObservationEvent) => void;
}) {
  const latest = observations.slice(0, 3);
  if (latest.length === 0) return null;

  return (
    <section className="observation-feed-overlay" aria-label="최근 관찰 포착">
      <div className="observation-feed-title">
        <span>관찰 포착</span>
        <b>{latest.length}</b>
      </div>
      <div className="observation-feed-items">
        {latest.map((event) => (
          <ObservationEventButton
            key={event.id}
            event={event}
            onFocus={onFocus}
            compact
          />
        ))}
      </div>
    </section>
  );
}

export function ObservationFeedPanel({
  observations,
  onFocus,
}: {
  observations: ObservationEvent[];
  onFocus: (event: ObservationEvent) => void;
}) {
  const recent = observations.slice(0, 8);

  return (
    <div className="observation-feed-panel">
      {recent.length > 0
        ? recent.map((event) => (
            <ObservationEventButton
              key={event.id}
              event={event}
              onFocus={onFocus}
            />
          ))
        : (
          <div className="focused-life-muted">
            아직 포착된 변화가 없습니다. 시뮬레이션이 진행되면 실제 활동·관계·기억·가족 변화가 여기에 기록됩니다.
          </div>
        )}
    </div>
  );
}


export function ObservationFocusBanner({
  event,
  onClear,
}: {
  event: ObservationEvent;
  onClear: () => void;
}) {
  return (
    <aside className={`observation-focus-banner ${event.importance}`} aria-live="polite">
      <div>
        <small>현장 관찰</small>
        <strong>{event.summary}</strong>
        {event.detail ? <span>{event.detail}</span> : null}
      </div>
      <button type="button" onClick={onClear} aria-label="현장 관찰 닫기">×</button>
    </aside>
  );
}
