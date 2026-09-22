import {
  REAL_MS_PER_SIMULATION_MINUTE_AT_1X,
  SIMULATION_TIME_CONTRACT,
  normalizeSimulationSpeed,
} from './lifelens-contract';

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
    this.realMsPerSimulationMinute =
      options.realMsPerSimulationMinute
      ?? REAL_MS_PER_SIMULATION_MINUTE_AT_1X;
    this.tickIntervalMs =
      options.tickIntervalMs
      ?? SIMULATION_TIME_CONTRACT.tickIntervalMs;
    this.refreshIntervalMs =
      options.refreshIntervalMs
      ?? SIMULATION_TIME_CONTRACT.refreshIntervalMs;
    this.maxCatchupMs =
      options.maxCatchupMs
      ?? SIMULATION_TIME_CONTRACT.maxCatchupMs;
    this.maxAdvanceMinutesPerTick =
      options.maxAdvanceMinutesPerTick
      ?? SIMULATION_TIME_CONTRACT.maxAdvanceMinutesPerTick;
    this.speed = normalizeSimulationSpeed(
      options.initialSpeed ?? SIMULATION_TIME_CONTRACT.defaultSpeed,
    );
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
    this.speed = normalizeSimulationSpeed(speed);
    this.resetAccumulator();
  }

  getSpeed(): number {
    return this.speed;
  }

  resetAccumulator(): void {
    this.lastWallMs = Date.now();
    this.accumulatorSimulationMinutes = 0;
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
