import {
  OBSERVER_CAMERA_CONTRACT,
  SIMULATION_TIME_CONTRACT,
} from '../runtime/lifelens-contract';
import type {
  DynamicEnvironment,
  Resident,
  TerrainWindow,
  WorldOverview,
  WorldPresentationSnapshot,
} from '../runtime/core-types';

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
  presentation: WorldPresentationSnapshot | null;
  camera: CameraState;
  runtime: RuntimeState;
  simulationSpeed: number;
  selectedResidentId: string | null;
  revision: number;
}

type Listener = () => void;

const INITIAL_STATE: ObserverSnapshot = {
  world: {},
  residents: [],
  terrain: null,
  environment: null,
  presentation: null,
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
    const nextResidents = patch.residents ?? this.snapshot.residents;
    const selectedResidentId = this.snapshot.selectedResidentId
      && nextResidents.some((resident) => resident.id === this.snapshot.selectedResidentId)
      ? this.snapshot.selectedResidentId
      : null;

    this.snapshot = {
      ...this.snapshot,
      ...patch,
      selectedResidentId,
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
    if (next === this.snapshot.selectedResidentId) return;

    this.snapshot = {
      ...this.snapshot,
      selectedResidentId: next,
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
      presentation: null,
      simulationSpeed: SIMULATION_TIME_CONTRACT.defaultSpeed,
      selectedResidentId: null,
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
