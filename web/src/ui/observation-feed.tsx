import type { ObservationEvent } from '../state/observation-feed';
import { formatDay } from './observer-format';

function kindLabel(kind: ObservationEvent['kind']): string {
  switch (kind) {
    case 'family': return '가족';
    case 'memory': return '기억';
    case 'relationship': return '관계';
    case 'life': return '생애';
    default: return '행동';
  }
}

function ObservationEventButton({
  event,
  onSelect,
  compact = false,
}: {
  event: ObservationEvent;
  onSelect: (residentId: string) => void;
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

  if (!event.residentId) {
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
      onClick={() => onSelect(event.residentId!)}
      aria-label={`${event.summary} 주민 집중 관찰`}
    >
      {content}
    </button>
  );
}

export function ObservationFeedOverlay({
  observations,
  onSelect,
}: {
  observations: ObservationEvent[];
  onSelect: (residentId: string) => void;
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
            onSelect={onSelect}
            compact
          />
        ))}
      </div>
    </section>
  );
}

export function ObservationFeedPanel({
  observations,
  onSelect,
}: {
  observations: ObservationEvent[];
  onSelect: (residentId: string) => void;
}) {
  const recent = observations.slice(0, 8);

  return (
    <div className="observation-feed-panel">
      {recent.length > 0
        ? recent.map((event) => (
            <ObservationEventButton
              key={event.id}
              event={event}
              onSelect={onSelect}
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
