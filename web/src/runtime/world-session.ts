import { LifeLensCoreBridge } from './core-bridge';
import {
  OBSERVER_RUNTIME_CONTRACT,
  WORLD_GRID_CONTRACT,
} from './lifelens-contract';
import type {
  CivilizationWorldPayload,
  DynamicEnvironment,
  RecentSocialEventsPayload,
  Resident,
  TerrainWindow,
  WorldObjectsPayload,
  WorldOverview,
} from './core-types';
import { ResidentContinuity } from './resident-continuity';

export interface WorldSessionSnapshot {
  overview: WorldOverview;
  residents: Resident[];
  terrain: TerrainWindow;
  terrainStaticChanged: boolean;
  environment: DynamicEnvironment;
  socialEvents: RecentSocialEventsPayload;
  civilization: CivilizationWorldPayload;
  worldObjects: WorldObjectsPayload;
  centerX: number;
  centerY: number;
  followResidents: boolean;
}

export class WorldSession {
  private centerX = 0;
  private centerY = 0;
  private followResidents = false;
  private stableTerrain: TerrainWindow | null = null;
  private stableTerrainCenterKey: string | null = null;
  private recenterRequested = false;
  private residentDetailRefreshCountdown = 0;
  private residentDetailSnapshot: Resident[] = [];
  private worldActivityRefreshCountdown = 0;
  private worldActivityWindowKey: string | null = null;
  private civilizationSnapshot: CivilizationWorldPayload = {
    available: false,
    resources: [],
    storages: [],
    facilities: [],
    recentDiscoveries: [],
  };
  private worldObjectsSnapshot: WorldObjectsPayload = {
    available: false,
    smartObjects: [],
    sanitationSites: [],
  };

  constructor(
    private readonly core: LifeLensCoreBridge,
    private readonly continuity: ResidentContinuity,
  ) {}

  createWorld(seed: string): void {
    this.core.createWorld(seed, '', 3);
    this.centerX = 0;
    this.centerY = 0;
    this.followResidents = false;
    this.stableTerrain = null;
    this.stableTerrainCenterKey = null;
    this.recenterRequested = true;
    this.residentDetailRefreshCountdown = 0;
    this.residentDetailSnapshot = [];
    this.worldActivityRefreshCountdown = 0;
    this.worldActivityWindowKey = null;
    this.civilizationSnapshot = {
      available: false,
      resources: [],
      storages: [],
      facilities: [],
      recentDiscoveries: [],
    };
    this.worldObjectsSnapshot = {
      available: false,
      smartObjects: [],
      sanitationSites: [],
    };
    this.continuity.reset();
  }

  runMinutes(minutes: number): void {
    this.core.runMinutes(minutes);
  }

  forceWorldActivityRefresh(): void {
    this.worldActivityRefreshCountdown = 0;
  }

  moveObserver(dx: number, dy: number): void {
    this.followResidents = false;
    this.centerX += dx;
    this.centerY += dy;
  }

  recenterToResidents(): void {
    this.followResidents = false;
    this.recenterRequested = true;
  }

  refresh(): WorldSessionSnapshot {
    const overview = this.core.worldOverview();
    let residentRuntimePayload = this.core.residentRuntime();
    if (residentRuntimePayload.available === false) {
      residentRuntimePayload = this.core.residents();
    }

    const runtimeResidents = residentRuntimePayload.residents ?? [];
    const detailIds = new Set(
      this.residentDetailSnapshot.map((resident) => resident.id),
    );
    const runtimeIds = new Set(
      runtimeResidents.map((resident) => resident.id),
    );
    const identityChanged = runtimeResidents.some(
      (resident) => !detailIds.has(resident.id),
    ) || this.residentDetailSnapshot.some(
      (resident) => !runtimeIds.has(resident.id),
    );

    if (
      residentRuntimePayload.available !== false
      && (
        this.residentDetailSnapshot.length === 0
        || this.residentDetailRefreshCountdown <= 0
        || identityChanged
      )
    ) {
      const detailPayload = this.core.residents();
      if (
        detailPayload.available !== false
        && Array.isArray(detailPayload.residents)
      ) {
        this.residentDetailSnapshot = detailPayload.residents;
        this.residentDetailRefreshCountdown = Math.max(
          0,
          OBSERVER_RUNTIME_CONTRACT.residentDetailRefreshEverySnapshots - 1,
        );
      }
    } else if (this.residentDetailRefreshCountdown > 0) {
      this.residentDetailRefreshCountdown -= 1;
    }

    const detailsById = new Map(
      this.residentDetailSnapshot.map(
        (resident) => [resident.id, resident] as const,
      ),
    );
    const mergedResidents = runtimeResidents.map((runtimeResident) => {
      const detail = detailsById.get(runtimeResident.id);
      if (!detail) return runtimeResident;
      return {
        ...detail,
        ...runtimeResident,
        emotion: {
          ...detail.emotion,
          ...runtimeResident.emotion,
        },
        needs: {
          ...detail.needs,
          ...runtimeResident.needs,
        },
        presentation:
          runtimeResident.presentation ?? detail.presentation,
      };
    });
    const residentPayload = {
      available: residentRuntimePayload.available,
      residents: mergedResidents,
    };

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

    if (this.recenterRequested && visiblePositions.length > 0) {
      const chunkXs = visiblePositions.map(
        (position) => Math.floor(position.x / WORLD_GRID_CONTRACT.gridCellsPerChunk),
      );
      const chunkYs = visiblePositions.map(
        (position) => Math.floor(position.y / WORLD_GRID_CONTRACT.gridCellsPerChunk),
      );

      this.centerX = Math.round(
        (Math.min(...chunkXs) + Math.max(...chunkXs)) * 0.5,
      );
      this.centerY = Math.round(
        (Math.min(...chunkYs) + Math.max(...chunkYs)) * 0.5,
      );
      this.recenterRequested = false;
    }

    const residentRadius = visiblePositions.reduce((radius, position) => {
      const chunkX = Math.floor(position.x / WORLD_GRID_CONTRACT.gridCellsPerChunk);
      const chunkY = Math.floor(position.y / WORLD_GRID_CONTRACT.gridCellsPerChunk);
      return Math.max(
        radius,
        Math.abs(chunkX - this.centerX),
        Math.abs(chunkY - this.centerY),
      );
    }, 8);

    const queryRadius = Math.max(8, Math.min(16, residentRadius + 2));
    const centerKey =
      `${this.centerX}:${this.centerY}:${queryRadius}`;
    let terrainStaticChanged = false;
    let terrain: TerrainWindow;

    if (
      this.stableTerrain === null
      || this.stableTerrainCenterKey !== centerKey
    ) {
      const candidate = this.core.terrainWindow(
        this.centerX,
        this.centerY,
        queryRadius,
      );
      const candidateValid = candidate.available === true
        && Array.isArray(candidate.chunks)
        && candidate.chunks.length > 0;

      if (candidateValid) {
        this.stableTerrain = candidate;
        this.stableTerrainCenterKey = centerKey;
        terrainStaticChanged = true;
        terrain = candidate;
      } else if (
        this.stableTerrain
        && this.stableTerrainCenterKey === centerKey
      ) {
        terrain = this.stableTerrain;
      } else {
        terrain = {
          ...candidate,
          available: false,
          chunks: Array.isArray(candidate.chunks)
            ? candidate.chunks
            : [],
        };
      }
    } else {
      terrain = this.stableTerrain;
    }

    // Terrain/elevation/ecology is deterministic for a fixed window. Only the
    // small human-trace tail is dynamic, so never regenerate hundreds of
    // procedural chunks on every 500 ms observer refresh.
    if (!terrainStaticChanged && terrain.available === true) {
      const traces = this.core.humanTracesWindow(
        this.centerX,
        this.centerY,
        queryRadius,
      );
      if (
        traces.available !== false
        && traces.humanTraces
      ) {
        terrain = {
          ...terrain,
          humanTraces: traces.humanTraces,
        };
      }
    }

    const environment = this.core.dynamicEnvironment(
      this.centerX,
      this.centerY,
    );
    const socialEvents = this.core.recentSocialEvents(32);

    // Civilization/object payloads are materially larger than the normal
    // resident/environment snapshot. Keep them off the 500 ms hot path while
    // still refreshing often enough for observation UI. A viewport move must
    // refresh immediately so a windowed resource payload never lags behind the
    // terrain currently on screen.
    const worldActivityWindowKey =
      `${this.centerX}:${this.centerY}:${queryRadius}`;
    if (
      this.worldActivityRefreshCountdown <= 0
      || this.worldActivityWindowKey !== worldActivityWindowKey
    ) {
      this.civilizationSnapshot = this.core.civilizationWorldWindow(
        16,
        this.centerX,
        this.centerY,
        queryRadius,
      );
      this.worldObjectsSnapshot = this.core.worldObjects();
      this.worldActivityWindowKey = worldActivityWindowKey;
      this.worldActivityRefreshCountdown = Math.max(
        0,
        OBSERVER_RUNTIME_CONTRACT.worldActivityRefreshEverySnapshots - 1,
      );
    } else {
      this.worldActivityRefreshCountdown -= 1;
    }

    return {
      overview,
      residents,
      terrain,
      terrainStaticChanged,
      environment,
      socialEvents,
      civilization: this.civilizationSnapshot,
      worldObjects: this.worldObjectsSnapshot,
      centerX: this.centerX,
      centerY: this.centerY,
      followResidents: this.followResidents,
    };
  }
}
