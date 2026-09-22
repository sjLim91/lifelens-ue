import type { Resident } from '../runtime/core-types';
import type { ObserverSnapshot } from '../state/observer-store';
import { formatDay, formatPercent } from './observer-format';

export function RuntimeBadge({
  runtime,
}: {
  runtime: ObserverSnapshot['runtime'];
}) {
  const text = runtime.status === 'ready'
    ? 'Core WASM LIVE'
    : runtime.status === 'error'
      ? 'Core load failed'
      : 'Core loading…';

  const className = runtime.status === 'loading' ? 'pending' : runtime.status;
  return <div id="status" className={`status ${className}`}>{text}</div>;
}

export function WorldOverlay({
  snapshot,
}: {
  snapshot: ObserverSnapshot;
}) {
  const center = snapshot.terrain?.chunks.find(
    (chunk) => chunk.x === snapshot.camera.centerChunkX
      && chunk.y === snapshot.camera.centerChunkY,
  );
  const forest = Math.round(
    Math.max(0, Math.min(1, Number(center?.forestCoverage01) || 0)) * 100,
  );

  return (
    <div className="world-overlay">
      <span id="timeLabel" className="world-overlay-primary">
        {formatDay(snapshot.world.minute)}
      </span>
      <span id="biomeLabel">
        {center
          ? `${center.biome ?? 'Unknown'} · 숲 ${forest}%`
          : '환경 분석 중'}
      </span>
      <span id="livingOverlay">
        {snapshot.world.livingResidents !== undefined
          ? `인구 ${snapshot.world.livingResidents}`
          : '인구 —'}
      </span>
    </div>
  );
}

export function ObserverMetrics({
  snapshot,
}: {
  snapshot: ObserverSnapshot;
}) {
  return (
    <div className="metrics">
      <div><span>Living</span><b id="living">{snapshot.world.livingResidents ?? '—'}</b></div>
      <div><span>Households</span><b id="households">{snapshot.world.households ?? '—'}</b></div>
      <div><span>Couples</span><b id="couples">{snapshot.world.activeCouples ?? '—'}</b></div>
      <div><span>Life events</span><b id="events">{snapshot.world.majorLifeEvents ?? '—'}</b></div>
    </div>
  );
}

function ResidentNeedsGrid({ resident }: { resident: Resident }) {
  return (
    <div className="need-grid">
      <span>배고픔 <b>{formatPercent(resident.needs?.hunger)}</b></span>
      <span>갈증 <b>{formatPercent(resident.needs?.thirst)}</b></span>
      <span>수면 <b>{formatPercent(resident.needs?.sleep)}</b></span>
      <span>위생 <b>{formatPercent(resident.needs?.hygiene)}</b></span>
    </div>
  );
}

function ResidentCard({
  resident,
  selected,
  onSelect,
}: {
  resident: Resident;
  selected: boolean;
  onSelect: (residentId: string) => void;
}) {
  return (
    <button
      type="button"
      className={`resident-card resident-card-button ${selected ? 'selected' : ''}`}
      onClick={() => onSelect(resident.id)}
      aria-pressed={selected}
    >
      <div className="resident-title">
        <strong>{resident.name}</strong>
        <span>{resident.sex ?? '—'} · {resident.activityLabel ?? 'Idle'}</span>
      </div>
      <ResidentNeedsGrid resident={resident} />
      <small>
        {resident.hasPosition
          ? `Grid ${resident.gridX}, ${resident.gridY}`
          : 'Position unavailable'}
      </small>
    </button>
  );
}

export function SelectedResidentReadout({
  resident,
  onClear,
}: {
  resident: Resident | null;
  onClear: () => void;
}) {
  if (!resident) {
    return (
      <div className="focused-life-empty">
        월드의 사람을 터치하면 그 사람의 현재 삶을 자세히 관찰할 수 있습니다.
      </div>
    );
  }

  return (
    <article className="focused-life">
      <div className="focused-life-heading">
        <div>
          <span>FOCUSED LIFE</span>
          <strong>{resident.name}</strong>
        </div>
        <button type="button" onClick={onClear} aria-label="선택 해제">×</button>
      </div>
      <p>
        {resident.sex ?? '—'} · 현재 행동 <b>{resident.activityLabel ?? 'Idle'}</b>
      </p>
      <ResidentNeedsGrid resident={resident} />
      <small>
        {resident.hasPosition
          ? `현재 위치 Grid ${resident.gridX}, ${resident.gridY}`
          : '현재 위치를 확인하는 중'}
      </small>
    </article>
  );
}

export function ResidentReadout({
  residents,
  selectedResidentId,
  onSelect,
}: {
  residents: Resident[];
  selectedResidentId: string | null;
  onSelect: (residentId: string) => void;
}) {
  return (
    <>
      <div className="panel-heading">
        <h2>Residents</h2>
        <span id="residentCount">{residents.length}</span>
      </div>
      <div id="residentList" className="resident-list">
        {residents.length > 0
          ? residents.map((resident) => (
              <ResidentCard
                key={resident.id}
                resident={resident}
                selected={resident.id === selectedResidentId}
                onSelect={onSelect}
              />
            ))
          : <div className="empty">표시할 주민이 없습니다.</div>}
      </div>
    </>
  );
}
