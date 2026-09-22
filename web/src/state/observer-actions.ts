export interface ObserverActionHandlers {
  createWorld: (seed: string) => void;
  stepMinutes: (minutes: number) => void;
  moveObserver: (dx: number, dy: number) => void;
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

  stepMinutes(minutes: number): void {
    this.handlers?.stepMinutes(minutes);
  }

  moveObserver(dx: number, dy: number): void {
    this.handlers?.moveObserver(dx, dy);
  }

  selectResident(residentId: string | null): void {
    this.handlers?.selectResident(residentId);
  }

  get ready(): boolean {
    return this.handlers !== null;
  }
}

export const observerActions = new ObserverActions();
