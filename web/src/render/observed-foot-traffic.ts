import type { Resident } from '../runtime/core-types';
import { WORLD_PRESENTATION } from './world-presentation-config';
import { REAL_MS_PER_SIMULATION_MINUTE_AT_1X, SIMULATION_TIME_CONTRACT, normalizeSimulationSpeed } from '../runtime/lifelens-contract';

export interface FootMark { x: number; y: number; angle: number; strength: number; minute: number }
interface Sample { x: number; y: number; minute: number }

/** Bounded, session-local visual memory of observed steps, never a Core road. */
export class ObservedFootTraffic {
  readonly marks = new Map<string, FootMark>();
  private samples = new Map<string, Sample>();
  private seed: string | undefined;
  private minute = -Infinity;
  private wallMs: number | undefined;
  private speed = SIMULATION_TIME_CONTRACT.defaultSpeed as number;

  setSpeed(speed: number): void {
    const next = normalizeSimulationSpeed(speed);
    if (next === this.speed) return;
    this.speed = next;
    this.breakContinuity();
  }

  /** Preserve visited cells, but never connect observations across a time jump. */
  breakContinuity(): void {
    this.samples.clear();
    this.wallMs = undefined;
  }

  observe(residents: Resident[], seed: string | undefined, minute: number, wallMs = performance.now()): void {
    if (!Number.isFinite(minute) || !Number.isFinite(wallMs)) return;
    if (seed !== this.seed || minute < this.minute) {
      this.marks.clear(); this.samples.clear();
      this.minute = -Infinity;
      this.wallMs = undefined;
    }
    this.seed = seed;
    if (minute === this.minute) return; // Pause and duplicate snapshots add no wear.
    const elapsedMs = this.wallMs === undefined ? Infinity : wallMs - this.wallMs;
    this.wallMs = wallMs;
    this.minute = minute;
    const config = WORLD_PRESENTATION.paths;
    // One normal observer window plus one tick of scheduling jitter. The integer
    // clock accumulator can straddle a tick, hence ceil; no fixed minute gap.
    const cadenceMs = SIMULATION_TIME_CONTRACT.refreshIntervalMs + SIMULATION_TIME_CONTRACT.tickIntervalMs;
    const maxMinutes = Math.ceil((elapsedMs + SIMULATION_TIME_CONTRACT.tickIntervalMs)
      / REAL_MS_PER_SIMULATION_MINUTE_AT_1X * this.speed);
    const continuous = this.speed > 0 && elapsedMs >= 0 && elapsedMs <= cadenceMs;
    for (const [key, mark] of this.marks) {
      if (minute - mark.minute >= config.fadeMinutes) this.marks.delete(key);
    }
    const next = new Map<string, Sample>();
    for (const resident of residents) {
      if (next.size >= config.maxSamples) break;
      if (resident.alive === false || !resident.hasPosition
        || !Number.isFinite(resident.gridX) || !Number.isFinite(resident.gridY)) continue;
      const x = Number(resident.gridX), y = Number(resident.gridY);
      next.set(resident.id, { x, y, minute });
      const previous = this.samples.get(resident.id);
      const deltaMinutes = previous ? minute - previous.minute : 0;
      if (!previous || !continuous || deltaMinutes > maxMinutes) continue;
      const dx = x - previous.x, dy = y - previous.y;
      // CoreNavigation advances cardinally, at most one grid cell per simulation
      // minute. Weather/health may slow it. Audit this authority bound on changes.
      const distance = Math.abs(dx) + Math.abs(dy);
      // Do not draw guessed paths across fast-forward gaps, teleports or reloads.
      if (distance <= 0 || distance > deltaMinutes) continue;
      const key = `${x}:${y}`;
      const old = this.marks.get(key);
      const strength = Math.min(config.maxOpacity,
        (old ? this.opacity(old, minute) : 0) + config.opacityPerVisit);
      this.marks.delete(key);
      this.marks.set(key, { x, y, angle: Math.atan2(dy, dx), strength, minute });
      if (this.marks.size > config.maxMarks) this.marks.delete(this.marks.keys().next().value!);
    }
    this.samples = next;
  }

  opacity(mark: FootMark, minute: number): number {
    return mark.strength * Math.max(0, 1 - Math.max(0, minute - mark.minute) / WORLD_PRESENTATION.paths.fadeMinutes);
  }
}
