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
  gradientX?: number;
  gradientY?: number;
  waterKind: WaterKind;
  salinity?: 'Fresh' | 'Brackish' | 'Salt' | string;
  waterAvailability: number;
  flowPotential?: number;
  drainageAccumulationPotential?: number;
  hasDownstream?: boolean;
  downstreamChunkX?: number;
  downstreamChunkY?: number;
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
  radiusChunks?: number;
  worldSeed?: string;
  chunks: TerrainChunk[];
  humanTraces?: { total: number; entries: HumanTrace[] };
}

interface HumanTracePosition {
  id: string;
  gridX: number;
  gridY: number;
}

export type HumanTrace = HumanTracePosition & (
  | { kind: 'ResourceUse'; material: string; quantity: number;
      baselineQuantity: number; renewable: boolean }
  | { kind: 'Residue'; sourceResidentId: string; amount: number;
      intensity: number; radiusTiles: number }
  | { kind: 'Facility'; sourceResidentId: string; facilityKind: string;
      state: string; progress01: number; deliveredMaterialUnits: number;
      requiredMaterialUnits: number; active: boolean; lit: boolean;
      cropPlanted: boolean; cropGrowth01: number; cropMoisture01: number;
      cropCare01: number; cropHarvestUnits: number }
);

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

export type ResidentKinship =
  | 'Unrelated'
  | 'Self'
  | 'Parent'
  | 'Child'
  | 'Sibling'
  | 'HalfSibling'
  | 'Spouse'
  | 'Grandparent'
  | 'Grandchild'
  | 'InLaw';

export interface ResidentFamilyMember {
  id: string;
  name: string;
  alive?: boolean;
  lifeStage?: string;
  kinship?: ResidentKinship | string;
}

export interface ResidentFamily {
  subjectId?: string;
  householdId?: string;
  hasRomanceHistory?: boolean;
  hasActivePartner?: boolean;
  partnerId?: string;
  partnerName?: string;
  partnerStage?: string;
  cohabitingWithPartner?: boolean;
  isGestationalParent?: boolean;
  expectingChild?: boolean;
  pregnancyPartnerId?: string;
  pregnancyPartnerName?: string;
  parents?: ResidentFamilyMember[];
  children?: ResidentFamilyMember[];
  siblings?: ResidentFamilyMember[];
}

export interface ResidentHouseholdResponsibilities {
  cooking?: number;
  cleaning?: number;
  shopping?: number;
  maintenance?: number;
  caregiving?: number;
}

export interface ResidentHouseholdMember {
  id: string;
  name?: string;
  contributionWeight?: number;
  responsibilities?: ResidentHouseholdResponsibilities;
}

export interface ResidentHousehold {
  id: string;
  homeObjectId?: string;
  resources?: number;
  sharedMoney?: number;
  sharedObjectIds?: string[];
  members?: ResidentHouseholdMember[];
}

export type ResidentPregnancyStage =
  | 'FirstTrimester'
  | 'SecondTrimester'
  | 'ThirdTrimester'
  | 'Due'
  | 'Completed';

export interface ResidentPregnancy {
  role?: 'GestationalParent' | 'GeneticPartner';
  gestationalParentId?: string;
  geneticPartnerId?: string;
  stage?: ResidentPregnancyStage | string;
  conceptionMinute?: number;
  dueMinute?: number;
  lastUpdateMinute?: number;
  health?: number;
  fatigue?: number;
  stress?: number;
  nutrition?: number;
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
  recallScore?: number;
  decayPerDay?: number;
  witnessed?: boolean;
  source?: 'DirectWitness' | 'ToldByOther' | 'Inferred';
  tags?: string[];
}

export interface ResidentBelief {
  subject?: string;
  proposition?: string;
  stance?: number;
  confidence?: number;
  supportWeight?: number;
  contradictionWeight?: number;
  supportCount?: number;
  contradictionCount?: number;
  lastUpdatedMinute?: number;
}

export interface ResidentGenetics {
  faceShape?: number;
  eyePigment?: number;
  hairPigment?: number;
  skinTone?: number;
  heightPotential?: number;
  buildPotential?: number;
  healthPotential?: number;
  learningPotential?: number;
  temperamentSensitivity?: number;
}

export interface ResidentLifeCondition {
  physicalHealth?: number;
  energyCapacity?: number;
  movementCapacity?: number;
  reproductivePotential?: number;
  workCapacity?: number;
  appearanceAgeFactor?: number;
  lifeGoalFamilyFocus?: number;
  familyRoleSalience?: number;
}

export interface ResidentDevelopment {
  attachment?: number;
  confidence?: number;
  stress?: number;
  socialSkill?: number;
  emotionalSecurity?: number;
  disciplineInternalization?: number;
  learningSupport?: number;
  health?: number;
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
  civilizationMaterial?: string;
  civilizationItem?: string;
  civilizationTechnique?: string;
  civilizationQuantity?: number;
  civilizationResourceNode?: string;
  civilizationStorage?: string;
  facilityAction?: string;
  facilityId?: string;
  facilityKind?: string;
  parentingAction?: string;
  knowledgeTeachingTechnique?: string;
  issuedMinute?: number;
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

export interface ResidentCivilizationItem {
  item?: string;
  material?: string;
  quantity?: number;
  quality?: number;
  durability?: number;
}

export interface ResidentCivilizationTechnique {
  technique?: string;
  level?: string;
  confidence?: number;
  successfulUses?: number;
  hasProvenance?: boolean;
  factId?: string;
  originResidentId?: string;
  immediateSourceId?: string;
  source?: 'Unknown' | 'SelfDiscovery' | 'DirectWitness' | 'Teaching';
  learnedMinute?: number;
  hopCount?: number;
}

export interface ResidentCivilization {
  totalInventoryUnits?: number;
  gatheringSkill?: number;
  craftingSkill?: number;
  learningSkill?: number;
  knownTechniqueCount?: number;
  reproducibleTechniqueCount?: number;
  latestKnowledgeMinute?: number;
  latestTechnique?: string;
  inventory?: ResidentCivilizationItem[];
  techniques?: ResidentCivilizationTechnique[];
}

export interface ResidentLifeEvent {
  type?: string;
  minute?: number;
  relatedCharacterIds?: string[];
  value?: number;
}

export interface Resident {
  id: string;
  name: string;
  sex?: 'Male' | 'Female';
  alive?: boolean;
  lifeStage?: string;
  ageYears?: number;
  hasBirthMinute?: boolean;
  birthMinute?: number;
  deathMinute?: number;
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
  genetics?: ResidentGenetics;
  lifeCondition?: ResidentLifeCondition;
  development?: ResidentDevelopment;
  traits?: ResidentTraits;
  preferences?: ResidentPreferences;
  civilization?: ResidentCivilization;
  relationships?: ResidentRelationship[];
  family?: ResidentFamily;
  household?: ResidentHousehold | null;
  pregnancy?: ResidentPregnancy | null;
  lifeHistory?: ResidentLifeEvent[];
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

export interface WorldLifeStageCounts {
  baby?: number;
  toddler?: number;
  child?: number;
  teen?: number;
  youngAdult?: number;
  adult?: number;
  middleAge?: number;
  elderly?: number;
}

export interface WorldOverview {
  available?: boolean;
  worldSeed?: string | number;
  populationSeed?: string | number;
  generationVersion?: number;
  minute?: number;
  totalResidents?: number;
  livingResidents?: number;
  deceasedResidents?: number;
  lifeStages?: WorldLifeStageCounts;
  households?: number;
  activeCouples?: number;
  datingCouples?: number;
  engagedCouples?: number;
  marriedCouples?: number;
  separatedCouples?: number;
  activePregnancies?: number;
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

export interface SimulationCalendar {
  minuteOfDay?: number;
  hourOfDay?: number;
  minuteOfHour?: number;
  dayIndex?: number;
  dayOfYear?: number;
  yearIndex?: number;
  annualPhase?: number;
  season?: 'Spring' | 'Summer' | 'Autumn' | 'Winter';
  isDay?: boolean;
  isNight?: boolean;
  daylight01?: number;
}

export interface DynamicEnvironment {
  available?: boolean;
  centerChunkX?: number;
  centerChunkY?: number;
  simulationMinute?: number;
  baselineTemperature01?: number;
  baselineMoisture01?: number;
  airTemperatureC?: number;
  seasonalTemperatureModifierC?: number;
  dailyTemperatureModifierC?: number;
  precipitationIntensity01?: number;
  cloudCover01?: number;
  windIntensity01?: number;
  humidity01?: number;
  visibility01?: number;
  surfaceWetness01?: number;
  precipitationType?: 'None' | 'Rain' | 'Snow';
  summary?: WeatherSummary;
  calendar?: SimulationCalendar;
}

export type SocialEventType =
  | 'PositiveInteraction'
  | 'Help'
  | 'Comfort'
  | 'Conflict'
  | 'Betrayal'
  | 'Rejection'
  | 'Apology'
  | 'Intimacy'
  | 'Commitment';

export interface RecentSocialEvent {
  sequence: string;
  actorId: string;
  targetId: string;
  type: SocialEventType | string;
  intensity: number;
  importance: number;
  minute: number;
  where: string;
  presentationLevel: 'Everyday' | 'Meaningful' | 'Important';
  successful: boolean;
}

export interface RecentSocialEventsPayload {
  available?: boolean;
  count?: number;
  events?: RecentSocialEvent[];
}

export interface CivilizationWorldResource {
  id: string;
  gridX: number;
  gridY: number;
  material: string;
  quantity: number;
  maxQuantity: number;
  renewable: boolean;
  regenerationPerDay: number;
}

export interface CivilizationWorldStorage {
  id: string;
  gridX: number;
  gridY: number;
  totalUnits: number;
  inventory: ResidentCivilizationItem[];
}

export interface CivilizationFacilityRequirement {
  material: string;
  required: number;
  delivered: number;
}

export interface CivilizationWorldFacility {
  id: string;
  kind: string;
  state: string;
  gridX: number;
  gridY: number;
  initiatedBy: string;
  lastWorkedBy: string;
  startedMinute: number;
  completedMinute: number;
  constructionWork: number;
  requiredWork: number;
  workProgress: number;
  durability: number;
  active: boolean;
  linkedStorage: string;
  requiredMaterialUnits: number;
  deliveredMaterialUnits: number;
  fuelUnits: number;
  charcoalUnits: number;
  oreUnits: number;
  metalUnits: number;
  heatLevel: number;
  lit: boolean;
  burnMinutesRemaining: number;
  lastFireMinute: number;
  cropPlanted: boolean;
  cropPlantedMinute: number;
  cropGrowth01: number;
  cropMoisture01: number;
  cropCare01: number;
  cropHarvestUnits: number;
  lastCultivationMinute: number;
  requirements: CivilizationFacilityRequirement[];
}

export interface CivilizationDiscovery {
  factId: string;
  technique: string;
  discovererId: string;
  discovererName: string;
  minute: number;
  recipientCount: number;
  livingKnowerCount: number;
}

export interface CivilizationWorldPayload {
  available?: boolean;
  minute?: number;
  resourceNodeCount?: number;
  depletedResourceNodeCount?: number;
  totalResourceUnits?: number;
  storageSiteCount?: number;
  totalStoredUnits?: number;
  facilityCount?: number;
  plannedFacilityCount?: number;
  underConstructionFacilityCount?: number;
  operationalFacilityCount?: number;
  techniqueFactCount?: number;
  transmissionReceiptCount?: number;
  uniqueKnownTechniqueTypes?: number;
  uniqueReproducibleTechniqueTypes?: number;
  knownTechniqueOwners?: number;
  reproducibleTechniqueOwners?: number;
  resources?: CivilizationWorldResource[];
  storages?: CivilizationWorldStorage[];
  facilities?: CivilizationWorldFacility[];
  recentDiscoveries?: CivilizationDiscovery[];
}

export interface WorldSmartObject {
  id: string;
  kind: string;
  gridX: number;
  gridY: number;
  reservedById: string;
  useDurationTicks: number;
  effectPerTick: ResidentNeeds;
}

export interface WorldSanitationSite {
  id: string;
  kind: 'DesignatedArea' | 'DugPit' | string;
  gridX: number;
  gridY: number;
  establishedBy: string;
  establishedMinute: number;
  active: boolean;
  useCount: number;
  improvementWork: number;
  improvedBy: string;
  improvedMinute: number;
}

export interface WorldObjectsPayload {
  available?: boolean;
  smartObjects?: WorldSmartObject[];
  sanitationSites?: WorldSanitationSite[];
}

export interface RuntimeClient {
  newGame(worldSeed: string, populationSeed: string, generationVersion: number): boolean;
  runMinutes(minutes: number): void;
  worldOverviewJson(): string;
  residentsJson(): string;
  dynamicEnvironmentJson?: (x: number, y: number) => string;
  recentSocialEventsJson?: (maxEvents: number) => string;
  civilizationWorldJson?: (maxRecentDiscoveries: number) => string;
  worldObjectsJson?: () => string;
  terrainWindowJson(x: number, y: number, radius: number): string;
  delete?: () => void;
}

export interface CoreModule {
  LifeLensWebClient: new () => RuntimeClient;
}
