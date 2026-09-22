import { LifeLensCoreBridge } from './core-bridge';
import type {
  DynamicEnvironment,
  Resident,
  TerrainWindow,
  WorldOverview,
} from './core-types';
import { ResidentContinuity } from './resident-continuity';

export interface WorldSessionSnapshot {
  overview: WorldOverview;
  residents: Resident[];
  terrain: TerrainWindow;
  environment: DynamicEnvironment;
  centerX: number;
  centerY: number;
  followResidents: boolean;
}

export class WorldSession {
  private centerX = 0;
  private centerY = 0;
  private followResidents = true;
  private stableTerrain: TerrainWindow | null = null;
  private stableTerrainCenterKey: string | null = null;
  private recenterRequested = false;

  constructor(
    private readonly core: LifeLensCoreBridge,
    private readonly continuity: ResidentContinuity,
  ) {}

  createWorld(seed: string): void {
    this.core.createWorld(seed, '', 3);
    this.centerX = 0;
    this.centerY = 0;
    this.followResidents = true;
    this.stableTerrain = null;
    this.stableTerrainCenterKey = null;
    this.recenterRequested = false;
    this.continuity.reset();
  }

  runMinutes(minutes: number): void {
    this.core.runMinutes(minutes);
  }

  moveObserver(dx: number, dy: number): void {
    this.followResidents = false;
    this.centerX += dx;
    this.centerY += dy;
  }

  resetFollow(): void {
    this.followResidents = true;
    this.recenterRequested = true;
  }

  refresh(): WorldSessionSnapshot {
    const overview = this.core.worldOverview();
    const residentPayload = this.core.residents();
    const expectedLiving = Math.max(
      0,
      Number(overview.livingResidents) || 0,
    );
    const residents = this.continuity.stabilize(
      residentPayload,
      expectedLiving,
    );

    const visiblePositions = residents
      .map((resident) => this.continuity.positionFor(resident))
      .filter((position): position is { x: number; y: number } => (
        position !== undefined
      ));

    if (this.followResidents && visiblePositions.length > 0) {
      const chunkXs = visiblePositions.map(
        (position) => Math.floor(position.x / 32),
      );
      const chunkYs = visiblePositions.map(
        (position) => Math.floor(position.y / 32),
      );

      const outsideTrackingEnvelope = chunkXs.some(
        (x) => Math.abs(x - this.centerX) > 6,
      ) || chunkYs.some(
        (y) => Math.abs(y - this.centerY) > 6,
      );

      if (this.recenterRequested || outsideTrackingEnvelope) {
        this.centerX = Math.round(
          (Math.min(...chunkXs) + Math.max(...chunkXs)) * 0.5,
        );
        this.centerY = Math.round(
          (Math.min(...chunkYs) + Math.max(...chunkYs)) * 0.5,
        );
      }
      this.recenterRequested = false;
    }

    const residentRadius = visiblePositions.reduce((radius, position) => {
      const chunkX = Math.floor(position.x / 32);
      const chunkY = Math.floor(position.y / 32);
      return Math.max(
        radius,
        Math.abs(chunkX - this.centerX),
        Math.abs(chunkY - this.centerY),
      );
    }, 8);

    const queryRadius = Math.max(8, Math.min(16, residentRadius + 2));
    const candidate = this.core.terrainWindow(
      this.centerX,
      this.centerY,
      queryRadius,
    );
    const centerKey = `${this.centerX}:${this.centerY}`;
    const candidateValid = candidate.available === true
      && Array.isArray(candidate.chunks)
      && candidate.chunks.length > 0;

    let terrain: TerrainWindow;
    if (candidateValid) {
      terrain = candidate;
      this.stableTerrain = candidate;
      this.stableTerrainCenterKey = centerKey;
    } else if (
      this.stableTerrain
      && this.stableTerrainCenterKey === centerKey
    ) {
      terrain = this.stableTerrain;
    } else {
      terrain = {
        ...candidate,
        available: false,
        chunks: Array.isArray(candidate.chunks) ? candidate.chunks : [],
      };
    }

    const environment = this.core.dynamicEnvironment(
      this.centerX,
      this.centerY,
    );

    return {
      overview,
      residents,
      terrain,
      environment,
      centerX: this.centerX,
      centerY: this.centerY,
      followResidents: this.followResidents,
    };
  }
}
