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
