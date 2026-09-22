import type {
  DynamicEnvironment,
  Resident,
  TerrainWindow,
  WorldOverview,
} from '../runtime/core-types';

export type RuntimeStatus = 'loading' | 'ready' | 'error';

export interface CameraState {
  centerChunkX: number;
  centerChunkY: number;
  angle: number;
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
  camera: CameraState;
  runtime: RuntimeState;
  selectedResidentId: string | null;
  revision: number;
}

type Listener = () => void;

const INITIAL_STATE: ObserverSnapshot = {
  world: {},
  residents: [],
  terrain: null,
  environment: null,
  camera: {
    centerChunkX: 0,
    centerChunkY: 0,
    angle: -0.68,
    zoom: 1,
    followResidents: true,
  },
  runtime: {
    status: 'loading',
    errorMessage: null,
  },
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
