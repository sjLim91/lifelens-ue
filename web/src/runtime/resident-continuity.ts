import type { Resident, ResidentsPayload } from './core-types';

export interface ResidentPosition {
  x: number;
  y: number;
}

interface CachedResident {
  resident: Resident;
  seenAt: number;
}

interface CachedPosition {
  position: ResidentPosition;
  seenAt: number;
}

function currentPosition(resident: Resident): ResidentPosition | undefined {
  if (resident.hasPosition
    && typeof resident.gridX === 'number' && Number.isFinite(resident.gridX)
    && typeof resident.gridY === 'number' && Number.isFinite(resident.gridY)) {
    return { x: resident.gridX, y: resident.gridY };
  }
  return undefined;
}

export class ResidentContinuity {
  private readonly residents = new Map<string, CachedResident>();
  private readonly positions = new Map<string, CachedPosition>();

  constructor(
    private readonly graceMs = 10000,
    private readonly now: () => number = () => performance.now(),
  ) {}

  stabilize(payload: ResidentsPayload, expectedLiving: number): Resident[] {
    if (!Number.isSafeInteger(expectedLiving) || expectedLiving < 0) {
      throw new Error('Resident continuity requires an authoritative living count');
    }
    if (expectedLiving === 0) {
      this.reset();
      return [];
    }

    const now = this.now();
    this.expire(now);
    // An unavailable payload must not refresh the age of old observations.
    const observed = payload.available !== false && Array.isArray(payload.residents)
      ? payload.residents : [];
    const incoming = new Map<string, Resident>();
    for (const resident of observed) {
      if (resident.alive === false) {
        this.remove(resident.id);
        continue;
      }
      incoming.set(resident.id, resident);
      this.residents.set(resident.id, { resident, seenAt: now });
      const position = currentPosition(resident);
      if (position) this.positions.set(resident.id, { position, seenAt: now });
    }

    // A complete Core list supersedes every previous identity. Without pruning,
    // a later partial response could resurrect a resident already removed by Core.
    if (payload.available !== false && incoming.size === expectedLiving) {
      for (const id of this.residents.keys()) {
        if (!incoming.has(id)) this.remove(id);
      }
      return [...incoming.values()];
    }

    const merged = new Map(incoming);
    const cachedResidents = [...this.residents.values()]
      .sort((left, right) => right.seenAt - left.seenAt);
    for (const cached of cachedResidents) {
      if (merged.size >= expectedLiving) break;
      if (!merged.has(cached.resident.id)) merged.set(cached.resident.id, cached.resident);
    }
    return [...merged.values()];
  }

  positionFor(resident: Resident): ResidentPosition | undefined {
    if (resident.alive === false) return undefined;
    const position = currentPosition(resident);
    if (position) return position;
    const cached = this.positions.get(resident.id);
    if (!cached) return undefined;
    if (this.now() - cached.seenAt > this.graceMs) {
      this.positions.delete(resident.id);
      return undefined;
    }
    // Rendering reads must never extend a missing observation's grace period.
    return { ...cached.position };
  }

  private remove(id: string): void {
    this.residents.delete(id);
    this.positions.delete(id);
  }

  private expire(now: number): void {
    for (const [id, cached] of this.residents) {
      if (now - cached.seenAt > this.graceMs) this.remove(id);
    }
    for (const [id, cached] of this.positions) {
      if (now - cached.seenAt > this.graceMs) this.positions.delete(id);
    }
  }

  reset(): void {
    this.residents.clear();
    this.positions.clear();
  }
}
