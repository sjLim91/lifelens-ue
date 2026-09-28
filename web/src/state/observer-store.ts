import {
  OBSERVER_CAMERA_CONTRACT,
  SIMULATION_TIME_CONTRACT,
} from '../runtime/lifelens-contract';
import type {
  CivilizationWorldPayload,
  DynamicEnvironment,
  RecentSocialEventsPayload,
  Resident,
  TerrainWindow,
  WorldObjectsPayload,
  WorldOverview,
} from '../runtime/core-types';
import {
  deriveObservationEvents,
  mergeObservationEvents,
  type ObservationEvent,
} from './observation-feed';
import { visibleHumanTraces } from './human-traces';
import {
  INITIAL_FAST_FORWARD_STATE,
  type FastForwardState,
  type FastForwardSummary,
} from './fast-forward';

export type RuntimeStatus = 'loading' | 'ready' | 'error';

export interface CameraState {
  centerChunkX: number;
  centerChunkY: number;
  angle: number;
  elevation: number;
  zoom: number;
  followResidents: boolean;
}

export interface RuntimeState {
  status: RuntimeStatus;
  errorMessage: string | null;
}

export interface ObserverSnapshot {
  world: WorldOverview;
  residents: Resident[];
  terrain: TerrainWindow | null;
  environment: DynamicEnvironment | null;
  socialEvents: RecentSocialEventsPayload;
  civilization: CivilizationWorldPayload;
  worldObjects: WorldObjectsPayload;
  camera: CameraState;
  runtime: RuntimeState;
  simulationSpeed: number;
  selectedResidentId: string | null;
  selectedHumanTraceId: string | null;
  observations: ObservationEvent[];
  fastForward: FastForwardState;
  revision: number;
}

type Listener = () => void;

const INITIAL_STATE: ObserverSnapshot = {
  world: {},
  residents: [],
  terrain: null,
  environment: null,
  socialEvents: {
    available: false,
    count: 0,
    events: [],
  },
  civilization: {
    available: false,
    resources: [],
    storages: [],
    facilities: [],
    recentDiscoveries: [],
  },
  worldObjects: {
    available: false,
    smartObjects: [],
    sanitationSites: [],
  },
  camera: {
    centerChunkX: 0,
    centerChunkY: 0,
    angle: OBSERVER_CAMERA_CONTRACT.defaultAngleRadians,
    elevation: OBSERVER_CAMERA_CONTRACT.defaultElevationRadians,
    zoom: OBSERVER_CAMERA_CONTRACT.defaultDesktopZoom,
    followResidents: false,
  },
  runtime: {
    status: 'loading',
    errorMessage: null,
  },
  simulationSpeed: SIMULATION_TIME_CONTRACT.defaultSpeed,
  selectedResidentId: null,
  selectedHumanTraceId: null,
  observations: [],
  fastForward: INITIAL_FAST_FORWARD_STATE,
  revision: 0,
};

class ObserverStore {
  private snapshot: ObserverSnapshot = INITIAL_STATE;
  private readonly listeners = new Set<Listener>();

  getSnapshot = (): ObserverSnapshot => this.snapshot;

  subscribe = (listener: Listener): (() => void) => {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  };

  update(patch: Partial<Omit<ObserverSnapshot, 'revision'>>): void {
    const nextWorld = patch.world ?? this.snapshot.world;
    const nextResidents = patch.residents ?? this.snapshot.residents;
    const nextSocialEvents = patch.socialEvents ?? this.snapshot.socialEvents;
    const nextCivilization = patch.civilization ?? this.snapshot.civilization;
    const nextWorldObjects = patch.worldObjects ?? this.snapshot.worldObjects;
    const selectedResidentId = this.snapshot.selectedResidentId
      && nextResidents.some((resident) => resident.id === this.snapshot.selectedResidentId)
      ? this.snapshot.selectedResidentId
      : null;

    const worldChanged = (
      this.snapshot.world.worldSeed !== undefined
      && nextWorld.worldSeed !== undefined
      && String(this.snapshot.world.worldSeed)
        !== String(nextWorld.worldSeed)
    );

    const observations = worldChanged
      ? []
      : mergeObservationEvents(
          deriveObservationEvents(
            this.snapshot.world,
            this.snapshot.residents,
            nextWorld,
            nextResidents,
            this.snapshot.socialEvents,
            nextSocialEvents,
            this.snapshot.civilization,
            nextCivilization,
            this.snapshot.worldObjects,
            nextWorldObjects,
          ),
          patch.observations ?? this.snapshot.observations,
        );

    this.snapshot = {
      ...this.snapshot,
      ...patch,
      selectedResidentId,
      selectedHumanTraceId: !worldChanged && visibleHumanTraces(
        patch.terrain !== undefined ? patch.terrain : this.snapshot.terrain,
      ).some(trace => trace.id === this.snapshot.selectedHumanTraceId)
        ? this.snapshot.selectedHumanTraceId : null,
      observations,
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  updateCamera(patch: Partial<CameraState>): void {
    this.snapshot = {
      ...this.snapshot,
      camera: {
        ...this.snapshot.camera,
        ...patch,
      },
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  selectResident(residentId: string | null): void {
    const next = residentId
      && this.snapshot.residents.some((resident) => resident.id === residentId)
      ? residentId
      : null;
    if (next === this.snapshot.selectedResidentId && !this.snapshot.selectedHumanTraceId) return;

    this.snapshot = {
      ...this.snapshot,
      selectedResidentId: next,
      selectedHumanTraceId: null,
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  selectHumanTrace(id: string | null): void {
    const next = visibleHumanTraces(this.snapshot.terrain).some(trace => trace.id === id) ? id : null;
    this.snapshot = {
      ...this.snapshot,
      selectedHumanTraceId: next,
      selectedResidentId: null,
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  beginFastForward(requestedDays: number, totalMinutes: number): void {
    this.snapshot = {
      ...this.snapshot,
      fastForward: {
        status: 'running',
        requestedDays,
        completedMinutes: 0,
        totalMinutes,
        progress01: 0,
        summary: null,
        errorMessage: null,
      },
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  updateFastForwardProgress(completedMinutes: number): void {
    if (this.snapshot.fastForward.status !== 'running') return;
    const totalMinutes = Math.max(1, this.snapshot.fastForward.totalMinutes);
    const bounded = Math.max(0, Math.min(totalMinutes, completedMinutes));
    this.snapshot = {
      ...this.snapshot,
      fastForward: {
        ...this.snapshot.fastForward,
        completedMinutes: bounded,
        progress01: bounded / totalMinutes,
      },
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  completeFastForward(summary: FastForwardSummary): void {
    this.snapshot = {
      ...this.snapshot,
      fastForward: {
        status: 'complete',
        requestedDays: summary.requestedDays,
        completedMinutes: Math.max(0, summary.endMinute - summary.startMinute),
        totalMinutes: Math.max(0, summary.endMinute - summary.startMinute),
        progress01: 1,
        summary,
        errorMessage: null,
      },
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  failFastForward(message: string): void {
    this.snapshot = {
      ...this.snapshot,
      fastForward: {
        ...this.snapshot.fastForward,
        status: 'error',
        errorMessage: message,
      },
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  clearFastForwardResult(): void {
    if (this.snapshot.fastForward.status === 'running') return;
    this.snapshot = {
      ...this.snapshot,
      fastForward: INITIAL_FAST_FORWARD_STATE,
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  setRuntime(status: RuntimeStatus, errorMessage: string | null = null): void {
    this.snapshot = {
      ...this.snapshot,
      runtime: { status, errorMessage },
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  resetWorld(): void {
    this.snapshot = {
      ...this.snapshot,
      world: {},
      residents: [],
      terrain: null,
      environment: null,
      socialEvents: {
        available: false,
        count: 0,
        events: [],
      },
      civilization: {
        available: false,
        resources: [],
        storages: [],
        facilities: [],
        recentDiscoveries: [],
      },
      worldObjects: {
        available: false,
        smartObjects: [],
        sanitationSites: [],
      },
      simulationSpeed: SIMULATION_TIME_CONTRACT.defaultSpeed,
      selectedResidentId: null,
      selectedHumanTraceId: null,
      observations: [],
      fastForward: INITIAL_FAST_FORWARD_STATE,
      camera: {
        ...INITIAL_STATE.camera,
      },
      revision: this.snapshot.revision + 1,
    };
    this.emit();
  }

  private emit(): void {
    for (const listener of this.listeners) listener();
  }
}

export const observerStore = new ObserverStore();
