export interface RenderCounters {
  calls: number; triangles: number; geometries: number; textures: number; programs: number;
}
export interface RenderSurface {
  width: number; height: number; pixelRatio: number; mobile: boolean;
  viewportWidth?: number; viewportHeight?: number;
}
const CAPACITY = 300;
const emptyCounters = (): RenderCounters => ({ calls: 0, triangles: 0, geometries: 0, textures: 0, programs: 0 });
const safe = (value: number) => Number.isFinite(value) ? Math.max(0, value) : 0;
const percentile = (values: number[], fraction: number): number | null => {
  if (!values.length) return null;
  values.sort((a, b) => a - b);
  return values[Math.max(0, Math.ceil(values.length * fraction) - 1)];
};

/** Bounded presentation telemetry; contains no resident/world/device identifiers. */
export class RenderPerformanceRecorder {
  private readonly cpu = new Float64Array(CAPACITY);
  private readonly intervals = new Float64Array(CAPACITY);
  private count = 0;
  private cursor = 0;
  private previousFrame: number | null = null;
  private latest = emptyCounters();
  private peak = emptyCounters();
  private surface: RenderSurface | null = null;
  private active = false;

  begin(surface: RenderSurface): void {
    this.count = this.cursor = 0;
    this.previousFrame = null;
    this.latest = emptyCounters(); this.peak = emptyCounters();
    this.surface = { ...surface }; this.active = true;
  }
  setSurface(surface: RenderSurface): void {
    if (this.surface && (Object.keys(surface) as (keyof RenderSurface)[])
      .every(key => this.surface![key] === surface[key])) return;
    const active = this.active;
    this.begin(surface);
    this.active = active;
  }
  resume(): void { this.active = true; this.previousFrame = null; }
  stop(): void { this.active = false; this.previousFrame = null; }
  breakContinuity(): void { this.previousFrame = null; }
  record(frameTime: number, cpuMs: number, counters: RenderCounters): void {
    if (!this.active || !Number.isFinite(frameTime) || !Number.isFinite(cpuMs) || cpuMs < 0) return;
    const interval = this.previousFrame === null ? 0 : frameTime - this.previousFrame;
    this.previousFrame = frameTime;
    this.cpu[this.cursor] = cpuMs;
    // Slow foreground frames count, including multi-second stalls. Only an
    // explicit visibility/resize/restart boundary may omit a positive gap.
    this.intervals[this.cursor] = interval > 0 ? interval : 0;
    this.cursor = (this.cursor + 1) % CAPACITY;
    this.count = Math.min(CAPACITY, this.count + 1);
    this.latest = { calls: safe(counters.calls), triangles: safe(counters.triangles),
      geometries: safe(counters.geometries), textures: safe(counters.textures), programs: safe(counters.programs) };
    for (const key of Object.keys(this.peak) as (keyof RenderCounters)[]) this.peak[key] = Math.max(this.peak[key], this.latest[key]);
  }
  snapshot() {
    const cpu = Array.from(this.cpu.subarray(0, this.count));
    const intervals = Array.from(this.intervals.subarray(0, this.count)).filter(n => n > 0);
    return {
      schema: 'lifelens-render-performance-v1', active: this.active,
      sampleLimit: CAPACITY, sampledFrames: this.count,
      // CPU submission time includes scene update/reflections/render, not GPU timestamps.
      cpuFrameMs: { p50: percentile(cpu.slice(), .5), p95: percentile(cpu, .95) },
      deliveredFps: intervals.length ? 1000 * intervals.length / intervals.reduce((a, b) => a + b, 0) : null,
      frameIntervalP95Ms: percentile(intervals, .95),
      sampledSeconds: intervals.reduce((sum, n) => sum + n, 0) / 1000,
      presentationProfile: this.surface?.mobile ? 'mobile' : 'desktop',
      targetFrameBudgetMs: this.surface?.mobile ? 1000 / 30 : null,
      surface: this.surface ? { ...this.surface } : null,
      latest: { ...this.latest }, peak: { ...this.peak },
    };
  }
}
export const renderPerformance = new RenderPerformanceRecorder();
