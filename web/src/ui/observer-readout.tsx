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

  return <div id="status" className={`status ${runtime.status}`}>{text}</div>;
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
      <span id="seedLabel">
        {snapshot.world.worldSeed !== undefined
          ? `WorldSeed ${snapshot.world.worldSeed}`
          : 'WorldSeed —'}
      </span>
      <span id="timeLabel">{formatDay(snapshot.world.minute)}</span>
      <span id="chunkLabel">
        Chunk {snapshot.camera.centerChunkX}, {snapshot.camera.centerChunkY}
      </span>
      <span id="biomeLabel">
        {center ? `Biome ${center.biome ?? '—'} · Forest ${forest}%` : 'Biome —'}
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

function ResidentCard({ resident }: { resident: Resident }) {
  return (
    <article className="resident-card">
      <div className="resident-title">
        <strong>{resident.name}</strong>
        <span>{resident.sex ?? '—'} · {resident.activityLabel ?? 'Idle'}</span>
      </div>
      <div className="need-grid">
        <span>배고픔 <b>{formatPercent(resident.needs?.hunger)}</b></span>
        <span>갈증 <b>{formatPercent(resident.needs?.thirst)}</b></span>
        <span>수면 <b>{formatPercent(resident.needs?.sleep)}</b></span>
        <span>위생 <b>{formatPercent(resident.needs?.hygiene)}</b></span>
      </div>
      <small>
        {resident.hasPosition
          ? `Grid ${resident.gridX}, ${resident.gridY}`
          : 'Position unavailable'}
      </small>
    </article>
  );
}

export function ResidentReadout({
  residents,
}: {
  residents: Resident[];
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
              <ResidentCard key={resident.id} resident={resident} />
            ))
          : <div className="empty">표시할 주민이 없습니다.</div>}
      </div>
    </>
  );
}
