export interface DiagnosticsSnapshot {
  lastCoreQueryMs: number;
  lastRenderMs: number;
  lastResidentCount: number;
  lastChunkCount: number;
  coreQueryFailures: number;
  renderFailures: number;
}

const INITIAL: DiagnosticsSnapshot = {
  lastCoreQueryMs: 0,
  lastRenderMs: 0,
  lastResidentCount: 0,
  lastChunkCount: 0,
  coreQueryFailures: 0,
  renderFailures: 0,
};

class RuntimeDiagnostics {
  private state: DiagnosticsSnapshot = { ...INITIAL };

  snapshot(): DiagnosticsSnapshot {
    return { ...this.state };
  }

  recordCoreQuery(durationMs: number, residents: number, chunks: number): void {
    this.state.lastCoreQueryMs = durationMs;
    this.state.lastResidentCount = residents;
    this.state.lastChunkCount = chunks;
  }

  recordRender(durationMs: number): void {
    this.state.lastRenderMs = durationMs;
  }

  recordCoreFailure(): void {
    this.state.coreQueryFailures += 1;
  }

  recordRenderFailure(): void {
    this.state.renderFailures += 1;
  }

  reset(): void {
    this.state = { ...INITIAL };
  }
}

export const runtimeDiagnostics = new RuntimeDiagnostics();
