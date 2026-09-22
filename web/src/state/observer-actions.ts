export interface ObserverActionHandlers {
  createWorld: (seed: string) => void;
  setSimulationSpeed: (speed: number) => void;
  moveObserver: (dx: number, dy: number) => void;
  recenterObserver: () => void;
  selectResident: (residentId: string | null) => void;
}

class ObserverActions {
  private handlers: ObserverActionHandlers | null = null;

  bind(handlers: ObserverActionHandlers): () => void {
    this.handlers = handlers;

    return () => {
      if (this.handlers === handlers) this.handlers = null;
    };
  }

  createWorld(seed: string): void {
    this.handlers?.createWorld(seed);
  }

  setSimulationSpeed(speed: number): void {
    this.handlers?.setSimulationSpeed(speed);
  }

  moveObserver(dx: number, dy: number): void {
    this.handlers?.moveObserver(dx, dy);
  }

  recenterObserver(): void {
    this.handlers?.recenterObserver();
  }

  selectResident(residentId: string | null): void {
    this.handlers?.selectResident(residentId);
  }

  get ready(): boolean {
    return this.handlers !== null;
  }
}

export const observerActions = new ObserverActions();
