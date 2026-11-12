/** Limit presentation work without changing the authoritative simulation clock. */
export class RenderFramePacer {
  private readonly intervalMs: number;
  private nextFrameAt: number;
  private lastFrameAt: number;

  constructor(maxFramesPerSecond: number | null, startedAt: number) {
    this.intervalMs = maxFramesPerSecond === null ? 0 : 1000 / maxFramesPerSecond;
    this.nextFrameAt = startedAt + this.intervalMs;
    this.lastFrameAt = startedAt;
  }

  advance(frameTime: number): number | null {
    // Fractional RAF timestamps can fall a few microseconds before the deadline.
    if (this.intervalMs > 0 && frameTime + 0.01 < this.nextFrameAt) return null;
    const elapsed = Math.max(0, frameTime - this.lastFrameAt);
    this.lastFrameAt = frameTime;
    if (this.intervalMs > 0) {
      // Retain the cadence after a late frame, without catch-up render bursts.
      const overdue = Math.max(0, frameTime - this.nextFrameAt + 0.01);
      this.nextFrameAt += (Math.floor(overdue / this.intervalMs) + 1) * this.intervalMs;
    }
    return Math.min(0.05, elapsed / 1000);
  }
}
