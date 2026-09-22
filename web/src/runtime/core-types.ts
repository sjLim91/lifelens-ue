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
  bladder?: number;
  hygiene?: number;
}

export interface ResidentEmotion {
  joy?: number;
  sadness?: number;
  anger?: number;
  fear?: number;
  embarrassment?: number;
  pride?: number;
  jealousy?: number;
  affection?: number;
  anxiety?: number;
  relief?: number;
  grief?: number;
  valence?: number;
  arousal?: number;
  intensity?: number;
}

export interface ResidentPersonality {
  introversion?: number;
  conscientiousness?: number;
  openness?: number;
  agreeableness?: number;
  emotionalStability?: number;
  empathy?: number;
  impulsiveness?: number;
  riskTolerance?: number;
  ambition?: number;
  patience?: number;
  sociability?: number;
  curiosity?: number;
  orderliness?: number;
  adaptability?: number;
}

export interface ResidentTraits {
  resilience?: number;
  creativity?: number;
  discipline?: number;
  compassion?: number;
  adaptability?: number;
  boldness?: number;
  perseverance?: number;
  resourcefulness?: number;
}

export interface ResidentPreferences {
  socializing?: number;
  solitude?: number;
  exploration?: number;
  crafting?: number;
  gathering?: number;
  comfort?: number;
  novelty?: number;
  order?: number;
}

export interface ResidentRelationship {
  targetId: string;
  targetName: string;
  affection?: number;
  trust?: number;
  respect?: number;
  comfort?: number;
  familiarity?: number;
  attraction?: number;
  romanticInterest?: number;
  sexualAttraction?: number;
  commitment?: number;
  conflict?: number;
  jealousy?: number;
  fear?: number;
  grudge?: number;
  socialBond?: number;
  romancePotential?: number;
}

export interface ResidentFamilyMember {
  id: string;
  name: string;
  alive?: boolean;
  lifeStage?: string;
}

export interface ResidentFamily {
  householdId?: string;
  hasActivePartner?: boolean;
  partnerId?: string;
  partnerName?: string;
  partnerStage?: string;
  cohabitingWithPartner?: boolean;
  expectingChild?: boolean;
  pregnancyPartnerId?: string;
  pregnancyPartnerName?: string;
  parents?: ResidentFamilyMember[];
  children?: ResidentFamilyMember[];
  siblings?: ResidentFamilyMember[];
}

export interface ResidentMemory {
  who?: string;
  sourceCharacter?: string;
  what?: string;
  where?: string;
  minute?: number;
  emotionValence?: number;
  emotionIntensity?: number;
  importance?: number;
  confidence?: number;
  effectiveConfidence?: number;
  witnessed?: boolean;
  source?: 'DirectWitness' | 'ToldByOther' | 'Inferred';
}

export interface ResidentBelief {
  subject?: string;
  proposition?: string;
  stance?: number;
  confidence?: number;
  lastUpdatedMinute?: number;
}

export type ResidentPresentationKind =
  | 'None'
  | 'Physical'
  | 'Social'
  | 'Civilization'
  | 'Parenting'
  | 'KnowledgeTeaching';

export type ResidentPresentationPhase =
  | 'Idle'
  | 'Moving'
  | 'Interacting';

export interface ResidentPresentationDirective {
  active?: boolean;
  kind?: ResidentPresentationKind;
  phase?: ResidentPresentationPhase;
  physicalGoal?: string;
  socialIntent?: string;
  civilizationIntent?: string;
  parentingAction?: string;
  targetResidentId?: string;
  hasTargetGrid?: boolean;
  targetGridX?: number;
  targetGridY?: number;
  hasObjectTarget?: boolean;
  objectId?: string;
  objectKind?: string;
  emergencyFallback?: boolean;
  designatedSanitationSite?: boolean;
  sanitationSiteId?: string;
  contextActionToken?: string;
  durationTicks?: number;
}

export interface Resident {
  id: string;
  name: string;
  sex?: 'Male' | 'Female';
  alive?: boolean;
  lifeStage?: string;
  ageYears?: number;
  activityKind?: 'Idle' | 'Physical' | 'Social';
  activityLabel?: string;
  physicalGoal?: string;
  socialIntent?: string;
  activityTargetId?: string;
  activityTargetName?: string;
  presentation?: ResidentPresentationDirective;
  emotion?: ResidentEmotion;
  needs?: ResidentNeeds;
  personality?: ResidentPersonality;
  traits?: ResidentTraits;
  preferences?: ResidentPreferences;
  relationships?: ResidentRelationship[];
  family?: ResidentFamily;
  memories?: ResidentMemory[];
  beliefs?: ResidentBelief[];
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

export type WeatherSummary =
  | 'Clear'
  | 'Cloudy'
  | 'Rain'
  | 'Snow'
  | 'Fog'
  | 'Storm'
  | 'Heat'
  | 'Cold';

export interface DynamicEnvironment {
  available?: boolean;
  centerChunkX?: number;
  centerChunkY?: number;
  simulationMinute?: number;
  airTemperatureC?: number;
  precipitationIntensity01?: number;
  cloudCover01?: number;
  windIntensity01?: number;
  humidity01?: number;
  visibility01?: number;
  surfaceWetness01?: number;
  precipitationType?: 'None' | 'Rain' | 'Snow';
  summary?: WeatherSummary;
}

export interface RuntimeClient {
  newGame(worldSeed: string, populationSeed: string, generationVersion: number): boolean;
  runMinutes(minutes: number): void;
  worldOverviewJson(): string;
  residentsJson(): string;
  dynamicEnvironmentJson?: (x: number, y: number) => string;
  terrainWindowJson(x: number, y: number, radius: number): string;
  delete?: () => void;
}

export interface CoreModule {
  LifeLensWebClient: new () => RuntimeClient;
}
