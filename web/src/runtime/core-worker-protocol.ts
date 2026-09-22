import type {
  ResidentsPayload,
  TerrainWindow,
  WorldOverview,
} from './core-types';

export interface CoreWorkerSnapshot {
  sequence: number;
  simulationMinute: number;
  overview: WorldOverview;
  residents: ResidentsPayload;
  terrain: TerrainWindow;
}

export type CoreWorkerCommand =
  | {
      type: 'create-world';
      requestId: number;
      seed: string;
      generationVersion: number;
    }
  | {
      type: 'step';
      requestId: number;
      minutes: number;
    }
  | {
      type: 'snapshot';
      requestId: number;
      centerX: number;
      centerY: number;
      radius: number;
    }
  | {
      type: 'dispose';
      requestId: number;
    };

export type CoreWorkerEvent =
  | {
      type: 'ready';
    }
  | {
      type: 'ack';
      requestId: number;
    }
  | {
      type: 'snapshot';
      requestId: number;
      payload: CoreWorkerSnapshot;
    }
  | {
      type: 'error';
      requestId?: number;
      message: string;
    };

export function isCoreWorkerEvent(value: unknown): value is CoreWorkerEvent {
  if (!value || typeof value !== 'object') return false;
  const type = (value as { type?: unknown }).type;
  return type === 'ready'
    || type === 'ack'
    || type === 'snapshot'
    || type === 'error';
}
