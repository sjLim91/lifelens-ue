export type WaterKind =
  | 'None'
  | 'Spring'
  | 'Stream'
  | 'River'
  | 'Lake'
  | 'Wetland'
  | 'Coast'
  | 'Ocean';

export interface TerrainChunk {
  x: number;
  y: number;
  elevation01: number;
  waterKind: WaterKind;
  waterAvailability: number;
  biome?: string;
  moisture01?: number;
  temperature01?: number;
  forestCoverage01?: number;
  grassCoverage01?: number;
  shrubCoverage01?: number;
  rockCoverage01?: number;
  wetlandCoverage01?: number;
}

export interface TerrainWindow {
  available: boolean;
  centerChunkX: number;
  centerChunkY: number;
  worldSeed?: string;
  chunks: TerrainChunk[];
}

export interface ResidentNeeds {
  hunger?: number;
  thirst?: number;
  sleep?: number;
  hygiene?: number;
}

export interface Resident {
  id: string;
  name: string;
  sex?: 'Male' | 'Female';
  activityLabel?: string;
  needs?: ResidentNeeds;
  hasPosition?: boolean;
  gridX?: number;
  gridY?: number;
}

export interface ResidentsPayload {
  available?: boolean;
  residents?: Resident[];
}

export interface WorldOverview {
  worldSeed?: string | number;
  minute?: number;
  livingResidents?: number;
  households?: number;
  activeCouples?: number;
  majorLifeEvents?: number;
  [key: string]: unknown;
}

export interface RuntimeClient {
  newGame(worldSeed: string, populationSeed: string, generationVersion: number): boolean;
  runMinutes(minutes: number): void;
  worldOverviewJson(): string;
  residentsJson(): string;
  terrainWindowJson(x: number, y: number, radius: number): string;
  delete?: () => void;
}

export interface CoreModule {
  LifeLensWebClient: new () => RuntimeClient;
}
