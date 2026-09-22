export interface SimulationClockOptions {
  realMsPerSimulationMinute?: number;
  tickIntervalMs?: number;
  refreshIntervalMs?: number;
  maxCatchupMs?: number;
  maxAdvanceMinutesPerTick?: number;
  initialSpeed?: number;
  onAdvance: (minutes: number) => void;
  onRefresh: () => void;
  onError?: (phase: 'advance' | 'refresh', error: unknown) => void;
}

export class SimulationClock {
  private readonly realMsPerSimulationMinute: number;
  private readonly tickIntervalMs: number;
  private readonly refreshIntervalMs: number;
  private readonly maxCatchupMs: number;
  private readonly maxAdvanceMinutesPerTick: number;
  private readonly onAdvance: (minutes: number) => void;
  private readonly onRefresh: () => void;
  private readonly onError?: (phase: 'advance' | 'refresh', error: unknown) => void;

  private tickTimer: number | null = null;
  private refreshTimer: number | null = null;
  private lastWallMs = Date.now();
  private accumulatorSimulationMinutes = 0;
  private speed = 1;

  constructor(options: SimulationClockOptions) {
    // Canonical LifeLens time contract:
    // 1x = 8 real minutes per 1 simulation day.
    // 480 real seconds / 1440 simulation minutes = 1/3 second per sim minute.
    this.realMsPerSimulationMinute =
      options.realMsPerSimulationMinute ?? (1000 / 3);
    this.tickIntervalMs = options.tickIntervalMs ?? 125;
    this.refreshIntervalMs = options.refreshIntervalMs ?? 500;
    this.maxCatchupMs = options.maxCatchupMs ?? 10 * 1000;
    this.maxAdvanceMinutesPerTick = options.maxAdvanceMinutesPerTick ?? 240;
    this.speed = this.normalizeSpeed(options.initialSpeed ?? 1);
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

  setSpeed(speed: number): void {
    this.tick();
    this.speed = this.normalizeSpeed(speed);
    this.resetAccumulator();
  }

  getSpeed(): number {
    return this.speed;
  }

  resetAccumulator(): void {
    this.lastWallMs = Date.now();
    this.accumulatorSimulationMinutes = 0;
  }

  private normalizeSpeed(speed: number): number {
    return [0, 1, 4, 16, 64].includes(speed) ? speed : 1;
  }

  private readonly handleVisibilityChange = (): void => {
    if (document.visibilityState !== 'visible') return;

    // Browser throttling must never become an unbounded history jump.
    // We catch up only inside the canonical simulation work budget.
    this.tick();
    this.refresh();
  };

  private tick(): void {
    const now = Date.now();
    const elapsed = Math.max(
      0,
      Math.min(this.maxCatchupMs, now - this.lastWallMs),
    );
    this.lastWallMs = now;

    if (this.speed <= 0) {
      this.accumulatorSimulationMinutes = 0;
      return;
    }

    this.accumulatorSimulationMinutes += (
      elapsed / this.realMsPerSimulationMinute
    ) * this.speed;

    const availableMinutes = Math.floor(this.accumulatorSimulationMinutes);
    const minutes = Math.min(
      availableMinutes,
      this.maxAdvanceMinutesPerTick,
    );
    if (minutes <= 0) return;

    try {
      this.onAdvance(minutes);
      this.accumulatorSimulationMinutes -= minutes;

      // Preserve causality while bounding one-frame work. Excessive wall-clock
      // backlog is capped rather than replayed without limit.
      if (availableMinutes > this.maxAdvanceMinutesPerTick) {
        this.accumulatorSimulationMinutes = Math.min(
          this.accumulatorSimulationMinutes,
          this.maxAdvanceMinutesPerTick,
        );
      }
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
