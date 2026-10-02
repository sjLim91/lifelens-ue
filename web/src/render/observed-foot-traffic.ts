import type { Resident } from '../runtime/core-types';
import { WORLD_PRESENTATION } from './world-presentation-config';

export interface FootMark { x: number; y: number; angle: number; strength: number; minute: number }
interface Sample { x: number; y: number; minute: number }

/** Bounded, session-local visual memory of observed steps, never a Core road. */
export class ObservedFootTraffic {
  readonly marks = new Map<string, FootMark>();
  private samples = new Map<string, Sample>();
  private seed: string | undefined;
  private minute = -Infinity;

  observe(residents: Resident[], seed: string | undefined, minute: number): void {
    if (!Number.isFinite(minute)) return;
    if (seed !== this.seed || minute < this.minute) {
      this.marks.clear(); this.samples.clear();
      this.minute = -Infinity;
    }
    this.seed = seed;
    if (minute === this.minute) return; // Pause and duplicate snapshots add no wear.
    this.minute = minute;
    const config = WORLD_PRESENTATION.paths;
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
      if (!previous || minute - previous.minute > config.maxGapMinutes) continue;
      const dx = x - previous.x, dy = y - previous.y;
      const distance = Math.hypot(dx, dy);
      // Do not draw guessed paths across fast-forward gaps, teleports or reloads.
      if (distance <= 0 || distance > config.maxStepGrid) continue;
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
