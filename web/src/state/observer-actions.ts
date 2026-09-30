import type { ObservationEvent } from './observation-feed';

export interface ObserverActionHandlers {
  createWorld: (seed: string) => void;
  setSimulationSpeed: (speed: number) => void;
  fastForwardDays: (days: number) => Promise<void>;
  clearFastForwardResult: () => void;
  moveObserver: (dx: number, dy: number) => void;
  recenterObserver: () => void;
  selectResident: (residentId: string | null) => void;
  selectHumanTrace: (id: string | null, focus?: boolean) => void;
  focusObservation: (event: ObservationEvent) => void;
  clearObservationFocus: () => void;
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

  async fastForwardDays(days: number): Promise<void> {
    await this.handlers?.fastForwardDays(days);
  }

  clearFastForwardResult(): void {
    this.handlers?.clearFastForwardResult();
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

  selectHumanTrace(id: string | null, focus = false): void {
    this.handlers?.selectHumanTrace(id, focus);
  }

  focusObservation(event: ObservationEvent): void {
    this.handlers?.focusObservation(event);
  }

  clearObservationFocus(): void {
    this.handlers?.clearObservationFocus();
  }

  get ready(): boolean {
    return this.handlers !== null;
  }
}

export const observerActions = new ObserverActions();
