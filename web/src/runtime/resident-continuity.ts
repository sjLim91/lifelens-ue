import type { Resident, ResidentsPayload } from './core-types';

export interface ResidentPosition {
  x: number;
  y: number;
}

interface CachedResident {
  resident: Resident;
  seenAt: number;
}

export class ResidentContinuity {
  private readonly residents = new Map<string, CachedResident>();
  private readonly positions = new Map<string, ResidentPosition>();

  constructor(private readonly graceMs = 10000) {}

  stabilize(payload: ResidentsPayload, expectedLiving: number): Resident[] {
    const now = performance.now();
    const incoming = Array.isArray(payload.residents) ? payload.residents : [];

    for (const resident of incoming) {
      this.residents.set(resident.id, { resident, seenAt: now });
      if (
        resident.hasPosition
        && resident.gridX !== undefined
        && resident.gridY !== undefined
      ) {
        this.positions.set(resident.id, {
          x: resident.gridX,
          y: resident.gridY,
        });
      }
    }

    for (const [id, cached] of this.residents) {
      if (now - cached.seenAt > this.graceMs) this.residents.delete(id);
    }

    if (expectedLiving <= 0) {
      this.reset();
      return [];
    }

    if (payload.available !== false && incoming.length >= expectedLiving) {
      return incoming;
    }

    const merged = new Map(
      incoming.map((resident) => [resident.id, resident] as const),
    );
    const cachedResidents = [...this.residents.values()]
      .sort((left, right) => right.seenAt - left.seenAt);

    for (const cached of cachedResidents) {
      if (merged.size >= expectedLiving) break;
      if (!merged.has(cached.resident.id)) {
        merged.set(cached.resident.id, cached.resident);
      }
    }

    return [...merged.values()];
  }

  positionFor(resident: Resident): ResidentPosition | undefined {
    if (
      resident.hasPosition
      && resident.gridX !== undefined
      && resident.gridY !== undefined
    ) {
      const position = { x: resident.gridX, y: resident.gridY };
      this.positions.set(resident.id, position);
      return position;
    }

    return this.positions.get(resident.id);
  }

  reset(): void {
    this.residents.clear();
    this.positions.clear();
  }
}
