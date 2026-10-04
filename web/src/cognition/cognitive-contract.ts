export const COGNITIVE_TRIGGERS = [
  'RepeatedFailure',
  'ResourceScarcity',
  'SocialConflict',
  'MajorLifeEvent',
  'Discovery',
  'MigrationPressure',
  'LeadershipDecision',
  'Reflection',
] as const;

export type CognitiveTrigger = (typeof COGNITIVE_TRIGGERS)[number];

export const COGNITIVE_INTENTS = [
  'ImproveFoodSecurity',
  'ImproveWaterSecurity',
  'ImproveShelter',
  'ImproveSanitation',
  'AcquireMaterials',
  'CraftUsefulTools',
  'ExpandCultivation',
  'ExploreOpportunity',
  'CooperateWithResident',
  'ResolveConflict',
  'TeachKnowledge',
  'TradeWithResident',
  'CareForDependent',
  'MigrateHousehold',
] as const;

export type CognitiveIntent = (typeof COGNITIVE_INTENTS)[number];


export const MAX_COGNITIVE_CONTEXT_ENTRIES = 32 as const;

export interface CognitiveMemoryDto {
  who: string;
  minute: number;
  recallScore: number;
  confidence: number;
  importance: number;
  emotionValence: number;
  emotionIntensity: number;
  what: string;
  where: string;
  tags: string[];
}

export interface CognitiveBeliefDto {
  subject: string;
  proposition: string;
  stance: number;
  confidence: number;
  lastUpdatedMinute: number;
}

export interface CognitiveRelationshipDto {
  target: string;
  socialBond: number;
  affection: number;
  trust: number;
  respect: number;
  conflict: number;
  fear: number;
  grudge: number;
}

export interface CognitiveRequestDto {
  actor: string;
  minute: number;
  trigger: CognitiveTrigger;
  needs: Record<string, number>;
  personality: Record<string, number>;
  emotion: Record<string, number>;
  memories: CognitiveMemoryDto[];
  beliefs: CognitiveBeliefDto[];
  relationships: CognitiveRelationshipDto[];
  allowedIntents: CognitiveIntent[];
}

export interface CognitiveProposalDto {
  actor: string;
  intent: CognitiveIntent;
  priority: number;
  targetResident: string | null;
  rationale: string;
}

const RESIDENT_TARGET_INTENTS = new Set<CognitiveIntent>([
  'CooperateWithResident',
  'ResolveConflict',
  'TeachKnowledge',
  'TradeWithResident',
  'CareForDependent',
]);

function record(value: unknown): Record<string, unknown> {
  if (typeof value !== 'object' || value === null || Array.isArray(value)) {
    throw new Error('cognitive proposal must be an object');
  }
  return value as Record<string, unknown>;
}

function finiteNumericRecord(
  value: unknown,
  label: string,
): Record<string, number> {
  const candidate = record(value);
  const entries = Object.entries(candidate);
  if (entries.length > MAX_COGNITIVE_CONTEXT_ENTRIES) {
    throw new Error(`${label} is too large`);
  }
  for (const [, entry] of entries) {
    if (typeof entry !== 'number' || !Number.isFinite(entry)) {
      throw new Error(`${label} contains a non-finite value`);
    }
  }
  return candidate as Record<string, number>;
}

export function parseCognitiveRequest(
  value: unknown,
): CognitiveRequestDto {
  const candidate = record(value);
  if (typeof candidate.actor !== 'string' || candidate.actor.length === 0) {
    throw new Error('cognitive request actor is invalid');
  }
  if (
    typeof candidate.minute !== 'number'
    || !Number.isFinite(candidate.minute)
    || candidate.minute < 0
  ) {
    throw new Error('cognitive request minute is invalid');
  }
  if (
    typeof candidate.trigger !== 'string'
    || !COGNITIVE_TRIGGERS.includes(candidate.trigger as CognitiveTrigger)
  ) {
    throw new Error('cognitive request trigger is invalid');
  }

  const needs = finiteNumericRecord(candidate.needs, 'cognitive request needs');
  const personality = finiteNumericRecord(
    candidate.personality,
    'cognitive request personality',
  );
  const emotion = finiteNumericRecord(
    candidate.emotion,
    'cognitive request emotion',
  );

  const boundedArray = (entry: unknown, label: string): unknown[] => {
    if (!Array.isArray(entry) || entry.length > MAX_COGNITIVE_CONTEXT_ENTRIES) {
      throw new Error(`${label} is invalid or too large`);
    }
    for (const item of entry) record(item);
    return entry;
  };

  const memories = boundedArray(
    candidate.memories,
    'cognitive request memories',
  ) as unknown as CognitiveMemoryDto[];
  const beliefs = boundedArray(
    candidate.beliefs,
    'cognitive request beliefs',
  ) as unknown as CognitiveBeliefDto[];
  const relationships = boundedArray(
    candidate.relationships,
    'cognitive request relationships',
  ) as unknown as CognitiveRelationshipDto[];

  if (
    !Array.isArray(candidate.allowedIntents)
    || candidate.allowedIntents.length > COGNITIVE_INTENTS.length
    || candidate.allowedIntents.some(
      (intent) => (
        typeof intent !== 'string'
        || !COGNITIVE_INTENTS.includes(intent as CognitiveIntent)
      ),
    )
  ) {
    throw new Error('cognitive request allowed intents are invalid');
  }

  return {
    actor: candidate.actor,
    minute: candidate.minute,
    trigger: candidate.trigger as CognitiveTrigger,
    needs,
    personality,
    emotion,
    memories,
    beliefs,
    relationships,
    allowedIntents: candidate.allowedIntents as CognitiveIntent[],
  };
}

export function cognitiveIntentTargetsResident(
  intent: CognitiveIntent,
): boolean {
  return RESIDENT_TARGET_INTENTS.has(intent);
}

export function parseCognitiveProposal(
  value: unknown,
  request: CognitiveRequestDto,
): CognitiveProposalDto {
  const candidate = record(value);

  if (candidate.actor !== request.actor) {
    throw new Error('cognitive proposal actor mismatch');
  }
  if (
    typeof candidate.intent !== 'string'
    || !request.allowedIntents.includes(candidate.intent as CognitiveIntent)
  ) {
    throw new Error('cognitive proposal intent is not allowed');
  }
  const intent = candidate.intent as CognitiveIntent;

  if (
    typeof candidate.priority !== 'number'
    || !Number.isFinite(candidate.priority)
    || candidate.priority < 0
    || candidate.priority > 1
  ) {
    throw new Error('cognitive proposal priority is invalid');
  }

  const target = candidate.targetResident;
  if (!(target === null || typeof target === 'string')) {
    throw new Error('cognitive proposal target is invalid');
  }
  if (cognitiveIntentTargetsResident(intent)) {
    if (typeof target !== 'string' || target.length === 0 || target === request.actor) {
      throw new Error('cognitive proposal requires another resident target');
    }
  } else if (target !== null) {
    throw new Error('cognitive proposal has an unexpected resident target');
  }

  if (typeof candidate.rationale !== 'string' || candidate.rationale.length > 240) {
    throw new Error('cognitive proposal rationale is invalid');
  }

  return {
    actor: request.actor,
    intent,
    priority: candidate.priority,
    targetResident: target,
    rationale: candidate.rationale,
  };
}

export function cognitiveProposalJsonSchema(
  request: CognitiveRequestDto,
): Record<string, unknown> {
  return {
    type: 'object',
    additionalProperties: false,
    required: [
      'actor',
      'intent',
      'priority',
      'targetResident',
      'rationale',
    ],
    properties: {
      actor: { type: 'string', const: request.actor },
      intent: {
        type: 'string',
        enum: [...request.allowedIntents],
      },
      priority: {
        type: 'number',
        minimum: 0,
        maximum: 1,
      },
      targetResident: {
        type: ['string', 'null'],
      },
      rationale: {
        type: 'string',
        maxLength: 240,
      },
    },
  };
}
