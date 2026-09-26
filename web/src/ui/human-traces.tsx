import type { HumanTrace, Resident, TerrainWindow } from '../runtime/core-types';
import { describeHumanTrace, visibleHumanTraces } from '../state/human-traces';

export function HumanTraceDetail({ trace, residents, onClose }: {
  trace: HumanTrace; residents: Resident[]; onClose: () => void;
}) {
  const description = describeHumanTrace(trace, residents);
  return (
    <div className="human-trace-detail">
      <div className="human-trace-heading">
        <strong>{description.title}</strong>
        <button type="button" className="human-trace-close" onClick={onClose} aria-label="장소 선택 해제">×</button>
      </div>
      <span className={`human-trace-status trace-${trace.kind}`}>{description.status}</span>
      <p>{description.detail}</p>
      <small>{description.note}</small>
      {trace.kind === 'Facility' && <small>지면의 고리는 시설 위치를 알려주는 관찰 표식입니다.</small>}
    </div>
  );
}

export function HumanTracePanel({ terrain, selectedId, onFocus }: {
  terrain: TerrainWindow | null; selectedId: string | null; onFocus: (id: string) => void;
}) {
  const traces = visibleHumanTraces(terrain);
  if (!terrain?.available || !terrain.humanTraces) {
    return <p className="focused-life-muted">생활 흔적 정보를 사용할 수 없습니다.</p>;
  }
  if (!traces.length) {
    return <p className="focused-life-muted">주민이 자원을 채집하고 생활하면 이 주변에 남긴 흔적을 살펴볼 수 있습니다.</p>;
  }
  const renderTrace = (trace: HumanTrace) => {
    const description = describeHumanTrace(trace);
    return (
      <button type="button" key={trace.id}
        className={`human-trace-item ${trace.id === selectedId ? 'selected' : ''}`}
        onClick={() => onFocus(trace.id)}
        aria-label={`${description.title}, ${description.status}, 이 장소 보기`}
        aria-pressed={trace.id === selectedId}>
        <span><strong>{description.title}</strong><small>{description.status}</small></span>
        <span aria-hidden="true">보기 ↗</span>
      </button>
    );
  };
  return (
    <div className="human-trace-list">
      <p className="hint">주변의 실제 생활 흔적 {terrain.humanTraces.total}곳 · 선택하면 그 장소로 이동합니다.</p>
      {traces.slice(0, 6).map(renderTrace)}
      {traces.length > 6 && <details>
        <summary>주변 흔적 {traces.length - 6}곳 더 보기</summary>
        {traces.slice(6).map(renderTrace)}
      </details>}
      {terrain.humanTraces.total > traces.length && <p className="hint">가까운 {traces.length}곳을 표시 중입니다. 다른 곳으로 이동하면 주변 흔적이 바뀝니다.</p>}
    </div>
  );
}
