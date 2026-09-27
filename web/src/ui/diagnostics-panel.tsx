import { useEffect, useState } from 'react';
import {
  runtimeDiagnostics,
  type DiagnosticsSnapshot,
} from '../runtime/runtime-diagnostics';

const EMPTY: DiagnosticsSnapshot = {
  lastCoreQueryMs: 0,
  lastRenderMs: 0,
  lastResidentCount: 0,
  lastChunkCount: 0,
  coreQueryFailures: 0,
  renderFailures: 0,
};

export function DiagnosticsPanel() {
  const [snapshot, setSnapshot] = useState<DiagnosticsSnapshot>(EMPTY);

  useEffect(() => {
    const timer = window.setInterval(() => {
      setSnapshot(runtimeDiagnostics.snapshot());
    }, 1000);

    return () => window.clearInterval(timer);
  }, []);

  if (!import.meta.env.DEV) return null;

  return (
    <section className="panel runtime">
      <h2>진단 정보</h2>
      <div><span>코어 조회</span><b>{snapshot.lastCoreQueryMs.toFixed(1)} 밀리초</b></div>
      <div><span>비상 렌더링</span><b>{snapshot.lastRenderMs.toFixed(1)} ms</b></div>
      <div><span>주민 수</span><b>{snapshot.lastResidentCount}</b></div>
      <div><span>지형 구역 수</span><b>{snapshot.lastChunkCount}</b></div>
      <div><span>코어 조회 실패</span><b>{snapshot.coreQueryFailures}</b></div>
      <div><span>렌더링 실패</span><b>{snapshot.renderFailures}</b></div>
    </section>
  );
}
