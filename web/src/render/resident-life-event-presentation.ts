import type { Resident, ResidentLifeEvent } from '../runtime/core-types';
import type { SocialEventAnchor, SocialEventAnchorResolver } from './social-event-presentation';
import { WORLD_PRESENTATION } from './world-presentation-config';
import { lifeEventPresentationKey, lifePairKey } from './resident-life-presentation';
const C = WORLD_PRESENTATION.lifeEvents;
const kinds: Record<string, keyof typeof C.colors> = {
  Birth: 'birth', ChildBorn: 'birth', LifeStageChanged: 'growth', PregnancyStarted: 'pregnancy',
  DatingStarted: 'relationship', Engaged: 'relationship', Married: 'relationship',
  CohabitationStarted: 'relationship', ParentingMilestone: 'growth', HouseholdChanged: 'household',
  Separated: 'separation', Divorced: 'separation', PartnerWidowed: 'loss', Death: 'loss', Bereavement: 'loss',
};
export interface LifeCue {
  key: string; type: string; kind: keyof typeof C.colors; minute: number;
  actorId: string; targetId?: string; actor: SocialEventAnchor; target?: SocialEventAnchor;
  start: number; duration: number; priority: number;
}
interface HistoryCursor { length: number; first: string; last: string; minute: number }
const valid = (p: SocialEventAnchor | null): p is SocialEventAnchor => !!p && [p.x, p.y, p.z].every(Number.isFinite);
const distance = (a: SocialEventAnchor, b: SocialEventAnchor) => Math.hypot(a.x-b.x, a.y-b.y, a.z-b.z);
export class ResidentLifeEventPresentation {
  private initialized = false;
  private readonly cursors = new Map<string, HistoryCursor>();
  private readonly seen = new Set<string>();
  private readonly departures = new Map<string, SocialEventAnchor>();
  private cues: LifeCue[] = [];
  constructor(private readonly resolve: SocialEventAnchorResolver,
    private readonly clock: () => number = () => performance.now()/1000) {}
  get activeCues(): readonly LifeCue[] { return this.cues; }
  get seenCount(): number { return this.seen.size; }
  get cursorCount(): number { return this.cursors.size; }
  now(): number { return this.clock(); }
  get majorPairs(): ReadonlyMap<string, number> {
    return new Map(this.cues.filter(c => c.kind === 'relationship' && c.targetId)
      .map(c => [lifePairKey(c.actorId, c.targetId!), c.minute]));
  }
  captureDepartures(residents: Resident[]): void {
    for (const r of residents) if (r.alive === false && r.lifeHistory?.some(e => e.type === 'Death')) {
      const anchor = this.resolve(r.id);
      if (valid(anchor)) this.departures.set(r.id, {...anchor});
    }
    while (this.departures.size > C.maxActive) this.departures.delete(this.departures.keys().next().value!);
  }
  private remember(key: string): void {
    this.seen.add(key);
    while (this.seen.size > C.maxSeen) this.seen.delete(this.seen.values().next().value!);
  }
  observe(residents: Resident[], minute: number): void {
    this.update();
    if (!Number.isFinite(minute) || !residents.some(r => Array.isArray(r.lifeHistory))) return;
    const baseline = !this.initialized, byId = new Map(residents.map(r => [r.id,r]));
    const fresh: LifeCue[] = [];
    for (const r of residents) {
      if (!Array.isArray(r.lifeHistory)) continue;
      const history = r.lifeHistory, previous = this.cursors.get(r.id);
      const first = history.length ? lifeEventPresentationKey(r.id,history[0]) : '';
      const last = history.length ? lifeEventPresentationKey(r.id,history[history.length-1]) : '';
      if (previous?.length === history.length && previous.first === first && previous.last === last) continue;
      const append = previous && previous.length <= history.length && previous.first === first
        && (!previous.length || lifeEventPresentationKey(r.id,history[previous.length-1]) === previous.last);
      const entries = append ? history.slice(previous.length) : history;
      let latest = previous?.minute ?? -1;
      for (const e of entries) {
        const key = lifeEventPresentationKey(r.id,e), time = Number(e.minute);
        if (Number.isFinite(time)) latest = Math.max(latest,time);
        if (this.seen.has(key)) continue;
        this.remember(key);
        // Sliding/truncated history fails closed to events strictly newer than
        // its prior watermark. Full unchanged histories use O(1) cursors.
        if (baseline || (!append && previous && time <= previous.minute)
          || !Number.isFinite(time) || time > minute || minute-time > C.replayWindowMinutes) continue;
        const cue = this.classify(r,e,byId);
        if (cue) fresh.push(cue);
      }
      this.cursors.delete(r.id);
      this.cursors.set(r.id,{length:history.length,first,last,minute:latest});
    }
    while (this.cursors.size > C.maxResidentCursors) this.cursors.delete(this.cursors.keys().next().value!);
    this.initialized = true;
    const unique = new Map(this.cues.map(c => [c.key,c]));
    for (const cue of fresh) if (!unique.has(cue.key)) unique.set(cue.key,cue);
    this.cues = [...unique.values()].sort((a,b) => b.priority-a.priority || b.minute-a.minute || b.start-a.start).slice(0,C.maxActive);
    this.departures.clear();
  }
  private classify(source: Resident, e: ResidentLifeEvent, byId: ReadonlyMap<string,Resident>): LifeCue | null {
    if (!Object.prototype.hasOwnProperty.call(kinds,e.type ?? '')) return null;
    const type = e.type!, kind = kinds[type];
    let actor = source, target: Resident | undefined;
    if (kind === 'birth') {
      if (type === 'ChildBorn') actor = (e.relatedCharacterIds ?? []).map(id => byId.get(id)).find(child =>
        child?.lifeStage === 'Baby' && (source.family?.children?.some(c => c.id === child.id)
          || child.family?.parents?.some(p => p.id === source.id)))!;
      if (!actor || actor.lifeStage !== 'Baby' || actor.alive === false || !actor.hasPosition) return null;
      target = (actor.family?.parents ?? []).map(p => byId.get(p.id)).find(p => p?.alive !== false && p?.hasPosition);
    } else if (kind === 'relationship' || kind === 'separation' || kind === 'pregnancy') {
      target = (e.relatedCharacterIds ?? []).map(id => byId.get(id)).find(r => r && r.id !== actor.id && r.alive !== false && r.hasPosition);
    }
    if (type !== 'Death' && (actor.alive === false || !actor.hasPosition)) return null;
    const anchor = type === 'Death' ? this.departures.get(actor.id) ?? null : this.resolve(actor.id);
    if (!valid(anchor)) return null;
    let targetAnchor = target ? this.resolve(target.id) : null;
    if (!valid(targetAnchor) || distance(anchor,targetAnchor) > C.pairMaxDistance) { target = undefined; targetAnchor = null; }
    const pair = target ? lifePairKey(actor.id,target.id) : actor.id;
    const key = kind === 'birth' ? `birth:${actor.id}:${e.minute}` : `${type}:${e.minute}:${pair}`;
    const priority = kind === 'birth' ? 4 : type === 'Married' || type === 'Death' ? 3
      : kind === 'relationship' || kind === 'pregnancy' || kind === 'separation' || kind === 'loss' ? 2 : 1;
    return {key,type,kind,minute:Number(e.minute),actorId:actor.id,targetId:target?.id,
      actor:{...anchor},target:valid(targetAnchor)?{...targetAnchor}:undefined,start:this.now(),
      duration:priority >= 2 ? C.durationSeconds : C.minorDurationSeconds,priority};
  }
  update(): void {
    const now = this.now();
    this.cues = this.cues.filter(c => now-c.start < c.duration
      && (c.type === 'Death' || valid(this.resolve(c.actorId)))
      && (!c.targetId || valid(this.resolve(c.targetId))));
  }
  rebase(dx: number, dz: number): void {
    for (const c of this.cues) for (const p of [c.actor,c.target]) if (p) {p.x+=dx;p.z+=dz;}
  }
  clearActive(): void {this.cues=[];this.departures.clear();}
  reset(): void {this.clearActive();this.seen.clear();this.cursors.clear();this.initialized=false;}
}
