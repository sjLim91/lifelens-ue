export interface SimulationClockOptions {
  realMsPerSimulationMinute?: number;
  tickIntervalMs?: number;
  refreshIntervalMs?: number;
  maxCatchupMs?: number;
  onAdvance: (minutes: number) => void;
  onRefresh: () => void;
  onError?: (phase: 'advance' | 'refresh', error: unknown) => void;
}

export class SimulationClock {
  private readonly realMsPerSimulationMinute: number;
  private readonly tickIntervalMs: number;
  private readonly refreshIntervalMs: number;
  private readonly maxCatchupMs: number;
  private readonly onAdvance: (minutes: number) => void;
  private readonly onRefresh: () => void;
  private readonly onError?: (phase: 'advance' | 'refresh', error: unknown) => void;

  private tickTimer: number | null = null;
  private refreshTimer: number | null = null;
  private lastWallMs = Date.now();
  private accumulatorMs = 0;

  constructor(options: SimulationClockOptions) {
    this.realMsPerSimulationMinute =
      options.realMsPerSimulationMinute ?? 1000;
    this.tickIntervalMs = options.tickIntervalMs ?? 250;
    this.refreshIntervalMs = options.refreshIntervalMs ?? 1000;
    this.maxCatchupMs = options.maxCatchupMs ?? 5 * 60 * 1000;
    this.onAdvance = options.onAdvance;
    this.onRefresh = options.onRefresh;
    this.onError = options.onError;
  }

  start(): void {
    this.stop();
    this.resetAccumulator();

    this.tickTimer = window.setInterval(
      () => this.tick(),
      this.tickIntervalMs,
    );
    this.refreshTimer = window.setInterval(
      () => this.refresh(),
      this.refreshIntervalMs,
    );
    document.addEventListener('visibilitychange', this.handleVisibilityChange);
  }

  stop(): void {
    if (this.tickTimer !== null) window.clearInterval(this.tickTimer);
    if (this.refreshTimer !== null) window.clearInterval(this.refreshTimer);
    this.tickTimer = null;
    this.refreshTimer = null;
    document.removeEventListener('visibilitychange', this.handleVisibilityChange);
  }

  resetAccumulator(): void {
    this.lastWallMs = Date.now();
    this.accumulatorMs = 0;
  }

  private readonly handleVisibilityChange = (): void => {
    if (document.visibilityState !== 'visible') return;

    // Browser timers are heavily throttled in background tabs. Catch up from
    // wall time immediately on resume, then repaint without waiting for the
    // next interval so the observer never appears frozen.
    this.tick();
    this.refresh();
  };

  private tick(): void {
    const now = Date.now();
    const elapsed = Math.max(0, now - this.lastWallMs);
    this.lastWallMs = now;
    this.accumulatorMs = Math.min(
      this.maxCatchupMs,
      this.accumulatorMs + elapsed,
    );

    const minutes = Math.floor(
      this.accumulatorMs / this.realMsPerSimulationMinute,
    );
    if (minutes <= 0) return;

    try {
      this.onAdvance(minutes);
      this.accumulatorMs -= minutes * this.realMsPerSimulationMinute;
    } catch (error) {
      this.onError?.('advance', error);
    }
  }

  private refresh(): void {
    try {
      this.onRefresh();
    } catch (error) {
      this.onError?.('refresh', error);
    }
  }
}
