import { useEffect, useState } from 'react';
import { renderPerformance } from '../render/render-performance';

function LiveRenderPerformance() {
  const [snapshot, setSnapshot] = useState(() => renderPerformance.snapshot());
  useEffect(() => {
    const timer = window.setInterval(() => setSnapshot(renderPerformance.snapshot()), 1000);
    return () => window.clearInterval(timer);
  }, []);
  if (!snapshot.surface) return <p className="hint">3D 화면에서 성능 기록을 수집합니다. 현재는 측정 기록이 없습니다.</p>;
  const ms = (value: number | null) => value === null ? '측정 중' : `${value.toFixed(1)} ms`;
  return <div className="render-performance-readout">
    <dl>
      <dt>화면 갱신</dt><dd>{snapshot.active ? snapshot.deliveredFps === null ? '측정 중' : `${snapshot.deliveredFps.toFixed(1)} FPS` : '3D 측정 중단'}</dd>
      <dt>프레임 처리 · 중앙값</dt><dd>{ms(snapshot.cpuFrameMs.p50)}</dd>
      <dt>프레임 처리 · 상위 5% 경계</dt><dd>{ms(snapshot.cpuFrameMs.p95)}</dd>
      <dt>프레임 간격 · 상위 5% 경계</dt><dd>{ms(snapshot.frameIntervalP95Ms)}</dd>
      <dt>최근 그리기 수</dt><dd>{snapshot.latest.calls.toLocaleString()}회</dd>
      <dt>최근 삼각형 수</dt><dd>{snapshot.latest.triangles.toLocaleString()}개</dd>
      <dt>화면 버퍼</dt><dd>{snapshot.surface.width} × {snapshot.surface.height}</dd>
      <dt>그래픽 객체 · 텍스처</dt><dd>{snapshot.latest.geometries} · {snapshot.latest.textures}</dd>
    </dl>
    <p className="hint">최근 최대 {snapshot.sampleLimit}프레임을 측정합니다. 프레임 처리는 장면 갱신과 화면 제출에 걸린 CPU 시간이며, 반사 그리기도 포함합니다.</p>
    <button type="button" onClick={() => {
      const report = renderPerformance.snapshot();
      const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json;charset=utf-8' });
      const url = URL.createObjectURL(blob), a = document.createElement('a');
      a.href = url; a.download = 'lifelens-render-performance.json';
      document.body.appendChild(a); a.click(); a.remove();
      window.setTimeout(() => URL.revokeObjectURL(url), 1000);
    }}>성능 기록 저장</button>
  </div>;
}
export function RenderPerformancePanel() {
  const [open, setOpen] = useState(false);
  return <details className="render-performance-panel mobile-settings-disclosure" onToggle={event => setOpen(event.currentTarget.open)}>
    <summary>화면 성능 확인</summary>
    {open && <LiveRenderPerformance />}
  </details>;
}
