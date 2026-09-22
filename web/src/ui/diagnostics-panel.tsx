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
  lastThreeFrameMs: 0,
  lastDrawCalls: 0,
  lastTriangles: 0,
  lastGeometries: 0,
  lastTextures: 0,
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
      <h2>Diagnostics</h2>
      <div><span>Core query</span><b>{snapshot.lastCoreQueryMs.toFixed(1)} ms</b></div>
      <div><span>Legacy render</span><b>{snapshot.lastRenderMs.toFixed(1)} ms</b></div>
      <div><span>Residents</span><b>{snapshot.lastResidentCount}</b></div>
      <div><span>Chunks</span><b>{snapshot.lastChunkCount}</b></div>
      <div><span>Three frame</span><b>{snapshot.lastThreeFrameMs.toFixed(1)} ms</b></div>
      <div><span>Draw calls</span><b>{snapshot.lastDrawCalls}</b></div>
      <div><span>Triangles</span><b>{snapshot.lastTriangles.toLocaleString()}</b></div>
      <div><span>Geometries</span><b>{snapshot.lastGeometries}</b></div>
      <div><span>Textures</span><b>{snapshot.lastTextures}</b></div>
      <div><span>Core failures</span><b>{snapshot.coreQueryFailures}</b></div>
      <div><span>Render failures</span><b>{snapshot.renderFailures}</b></div>
    </section>
  );
}
