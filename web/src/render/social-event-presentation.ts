import type { RecentSocialEvent, RecentSocialEventsPayload, SocialEventType } from '../runtime/core-types';
import { SOCIAL_EVENT_PRESENTATION_CONTRACT as C } from '../runtime/lifelens-contract';

export interface SocialEventAnchor { x: number; y: number; z: number }
export type SocialEventAnchorResolver = (id: string) => SocialEventAnchor | null;

export interface SocialEventVisual {
  kind: SocialEventType;
  color: number;
  scale: number;
  opacity: number;
  durationSeconds: number;
  level: number;
  importance: number;
  successful: boolean;
}

const levelRank = { Everyday: 0, Meaningful: 1, Important: 2 } as const;
const clamp01 = (value: number): number => Number.isFinite(value)
  ? Math.max(0, Math.min(1, value)) : 0;

// Exact Core enum mapping; unknown future types are skipped.
export function classifySocialEvent(event: RecentSocialEvent): SocialEventVisual | null {
  if (!Object.hasOwn(C.colors, event.type)) return null;
  const kind = event.type as SocialEventType;
  const level = levelRank[event.presentationLevel] ?? 0;
  const duration = [C.everydayDurationSeconds, C.meaningfulDurationSeconds,
    C.importantDurationSeconds][level];
  const intensity = clamp01(event.intensity);
  const importance = clamp01(event.importance);
  const successful = event.successful === true;
  return {
    kind, color: C.colors[kind], level, importance, successful,
    scale: C.minScale + (C.maxScale - C.minScale) * intensity,
    opacity: (C.minOpacity + (C.maxOpacity - C.minOpacity) * intensity)
      * (successful ? 1 : C.unsuccessfulOpacityMultiplier),
    durationSeconds: (duration + importance * C.importanceDurationSeconds)
      * (successful ? 1 : C.unsuccessfulDurationMultiplier),
  };
}

// Same identity contract as Observation Feed; this cache is presentation only.
export function socialEventPresentationKey(event: RecentSocialEvent): string {
  if (event.sequence && event.sequence !== '0') return `sequence:${event.sequence}`;
  return [event.minute, event.actorId, event.targetId, event.type, event.where].join('|');
}

export interface ActiveSocialEventCue {
  key: string;
  actorId: string;
  targetId: string;
  minute: number;
  sequence: bigint;
  visual: SocialEventVisual;
  actor: SocialEventAnchor;
  target: SocialEventAnchor;
  startedAtSeconds: number;
}

function sequenceNumber(sequence: string): bigint {
  return /^[1-9]\d*$/.test(sequence ?? '') ? BigInt(sequence) : 0n;
}
function validAnchor(anchor: SocialEventAnchor | null): anchor is SocialEventAnchor {
  return !!anchor && [anchor.x, anchor.y, anchor.z].every(Number.isFinite);
}
function priority(a: ActiveSocialEventCue, b: ActiveSocialEventCue): number {
  return b.visual.level - a.visual.level
    || b.visual.importance - a.visual.importance
    || b.minute - a.minute
    || (a.sequence < b.sequence ? 1 : a.sequence > b.sequence ? -1 : 0)
    || b.startedAtSeconds - a.startedAtSeconds;
}

export class SocialEventPresentation {
  private readonly seen = new Set<string>();
  private sequenceHighWater = 0n;
  private initialized = false;
  private cues: ActiveSocialEventCue[] = [];

  constructor(private readonly nowSeconds: () => number = () => performance.now() / 1000) {}

  get activeCues(): readonly ActiveSocialEventCue[] { return this.cues; }
  get seenCount(): number { return this.seen.size; }
  now(): number { return this.nowSeconds(); }

  reset(): void {
    this.seen.clear();
    this.sequenceHighWater = 0n;
    this.initialized = false;
    this.cues = [];
  }

  clearActive(): void { this.cues = []; }

  observe(payload: RecentSocialEventsPayload, minute: number, resolve: SocialEventAnchorResolver): void {
    this.update(resolve);
    if (payload.available === false || !Number.isFinite(minute)) return;
    const now = this.now();
    const previousHighWater = this.sequenceHighWater;
    const fresh: ActiveSocialEventCue[] = [];
    for (const event of payload.events ?? []) {
      const key = socialEventPresentationKey(event);
      const sequence = sequenceNumber(event.sequence);
      if (this.seen.has(key) || (sequence > 0n && sequence <= previousHighWater)) continue;
      this.seen.add(key);
      while (this.seen.size > C.maxSeenEvents) this.seen.delete(this.seen.values().next().value!);
      if (sequence > this.sequenceHighWater) this.sequenceHighWater = sequence;
      const age = minute - event.minute;
      if (!Number.isFinite(age) || age < 0 || age > C.replayWindowMinutes) continue;
      const visual = classifySocialEvent(event);
      if (!visual || event.actorId === event.targetId) continue;
      const actor = resolve(event.actorId), target = resolve(event.targetId);
      if (!validAnchor(actor) || !validAnchor(target)) continue;
      const distance = Math.hypot(target.x - actor.x, target.y - actor.y, target.z - actor.z);
      if (distance > C.pairMaxDistance || distance < C.pairMinDistance) continue;
      fresh.push({
        key, actorId: event.actorId, targetId: event.targetId,
        minute: event.minute, sequence, visual, actor: { ...actor }, target: { ...target },
        startedAtSeconds: now,
      });
      // Admission remains bounded even for malformed/oversized payloads.
      fresh.sort(priority);
      if (fresh.length > C.maxActiveCues) fresh.pop();
    }
    fresh.sort(priority);
    if (!this.initialized) fresh.splice(C.maxInitialCues);
    this.initialized = true;
    this.cues = [...this.cues, ...fresh].sort(priority).slice(0, C.maxActiveCues);
  }

  update(resolve: SocialEventAnchorResolver): void {
    const now = this.now();
    this.cues = this.cues.filter(cue =>
      now - cue.startedAtSeconds < cue.visual.durationSeconds
      && validAnchor(resolve(cue.actorId)) && validAnchor(resolve(cue.targetId)));
  }

  rebase(dx: number, dz: number): void {
    for (const cue of this.cues) {
      cue.actor.x += dx; cue.actor.z += dz;
      cue.target.x += dx; cue.target.z += dz;
    }
  }
}
