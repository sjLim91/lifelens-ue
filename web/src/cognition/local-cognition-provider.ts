import {
  cognitiveProposalJsonSchema,
  parseCognitiveProposal,
  type CognitiveProposalDto,
  type CognitiveRequestDto,
} from './cognitive-contract';
import { CognitiveProviderError, type CognitiveProvider } from './cognitive-provider';

export interface LocalCognitionProviderConfig {
  baseUrl: string;
  model: string;
}

type FetchLike = (
  input: RequestInfo | URL,
  init?: RequestInit,
) => Promise<Response>;

export function isLoopbackCognitionUrl(value: string): boolean {
  try {
    const url = new URL(value);
    if (url.protocol !== 'http:' && url.protocol !== 'https:') return false;
    if (url.username || url.password) return false;
    const host = url.hostname.toLowerCase();
    return host === 'localhost'
      || host === '127.0.0.1'
      || host === '[::1]'
      || host === '::1';
  } catch {
    return false;
  }
}


export async function discoverLocalCognitionModels(
  baseUrl: string,
  signal: AbortSignal,
  fetcher: FetchLike = fetch,
): Promise<string[]> {
  if (!isLoopbackCognitionUrl(baseUrl)) {
    throw new Error(
      'LifeLens local model discovery accepts loopback/local endpoints only',
    );
  }
  if (signal.aborted) return [];

  const normalized = baseUrl.endsWith('/') ? baseUrl : `${baseUrl}/`;
  const endpoint = new URL('v1/models', normalized).toString();
  const response = await fetcher(endpoint, {
    method: 'GET',
    redirect: 'error',
    headers: {
      'accept': 'application/json',
    },
    signal,
  });

  if (!response.ok) {
    throw new Error(`local cognition model discovery HTTP ${response.status}`);
  }

  const payload = await response.json() as {
    data?: Array<{ id?: unknown }>;
  };
  if (!Array.isArray(payload.data)) {
    throw new Error('local cognition model discovery response is malformed');
  }

  const unique = new Set<string>();
  for (const entry of payload.data) {
    if (
      typeof entry?.id === 'string'
      && entry.id.trim().length > 0
      && entry.id.trim().length <= 200
      && !/[\u0000-\u001f\u007f]/.test(entry.id)
    ) {
      unique.add(entry.id.trim());
    }
    if (unique.size >= 32) break;
  }
  return [...unique];
}

// Auto-discovery uses only local, credential-free endpoints. A browser cannot
// start a native model server; if none is available, the SAME Core resident
// remains alive and autonomous while on-device language reasoning is pending.
export const AUTOMATIC_LOCAL_COGNITION_ENDPOINTS = Object.freeze([
  'http://127.0.0.1:11434', // Ollama OpenAI-compatible API
  'http://127.0.0.1:1234',  // LM Studio
  'http://127.0.0.1:8080',  // llama.cpp
] as const);

export async function discoverAutomaticLocalCognition(
  signal: AbortSignal,
  fetcher: FetchLike = fetch,
): Promise<LocalCognitionProviderConfig | null> {
  for (const baseUrl of AUTOMATIC_LOCAL_COGNITION_ENDPOINTS) {
    if (signal.aborted) return null;
    try {
      const models = await discoverLocalCognitionModels(baseUrl, signal, fetcher);
      const model = models.find(name => name.length > 0 && name.length <= 200);
      if (model) return { baseUrl, model };
    } catch {
      // A missing local service, browser CORS restriction or malformed local
      // response never authorizes a cloud API or an imaginary model.
      if (signal.aborted) return null;
    }
  }
  return null;
}

function chatCompletionsUrl(baseUrl: string): string {
  const normalized = baseUrl.endsWith('/') ? baseUrl : `${baseUrl}/`;
  return new URL('v1/chat/completions', normalized).toString();
}

export function cognitiveSystemPrompt(): string {
  return [
    'You are the strategic reasoning layer for one LifeLens resident.',
    'Use only facts in the supplied request.',
    'localPerception, when present, is a CURRENT small-radius Core scan from this resident\'s actual position, not global resource knowledge. It proves only nearby availability, NOT a visible line of sight, safe path, permission to gather, exact quantity, or successful collection. Null means unknown, not zero resources.',
    'Treat memory.source DirectWitness as firsthand observation, ToldByOther as hearsay, and Inferred as an unverified inference.',
    'Never treat reported, inferred, or missing-origin memories as proven world truth. Missing source or witnessed means unverified. The witnessed flag also matters.',
    'DirectWitness memories tagged action_experience show only what this resident actually attempted and whether that attempt succeeded or failed. Use them and experienced-action-outcome beliefs as evidence when choosing the NEXT strategy.',
    'Repeated firsthand failures may justify another allowed approach; never infer from one failed attempt that every resource, person, tool, or location is unavailable.',
    'When the Core trigger is RepeatedFailure, reconsider the previous approach using real direct outcome evidence, compare allowed alternatives and prefer a plausible different strategy if available.',
    'When a DirectWitness action_experience memory names a Core-verified cause, distinguish source-exhausted (the observed source ran out), source-removed (the known source disappeared), container-missing (water exists but transport equipment is missing), and access-unavailable (this destination could not be reached).',
    'A container-missing failure is NOT evidence that water is absent; consider only allowed material/tool actions and remain uncertain about whether they will succeed.',
    'Source-exhausted or source-removed can justify a new search when exploration is allowed; access-unavailable can justify a different route or goal, but never assume traversal is possible.',
    'Do not treat a cause string in hearsay, Inferred memory, or model rationale as verified first-person evidence.',
    'An executed action is not automatically a completed life goal: a successful Explore action proves an area was explored, NOT that the requested food, water, or technology was found.',
    'If personal outcome evidence is missing or inconclusive, remain uncertain; do not invent an observed success, failure, private thought, or resource.',
    'Choose only one allowed intent.',
    'Never invent residents, resources, facilities, technologies, or outcomes.',
    'Return only JSON matching the schema.',
    'Do not provide hidden chain-of-thought.',
    'Keep rationale short and state only the decision-relevant reason.',
    'Write the short rationale in Korean.',
  ].join(' ');
}

export class LocalCognitionProvider implements CognitiveProvider {
  readonly name = 'local-openai-compatible';
  private readonly endpoint: string;

  constructor(
    private readonly config: LocalCognitionProviderConfig,
    private readonly fetcher: FetchLike = fetch,
  ) {
    if (!isLoopbackCognitionUrl(config.baseUrl)) {
      throw new Error(
        'LifeLens default cognition provider accepts loopback/local endpoints only',
      );
    }
    if (!config.model.trim() || config.model.length > 200
      || /[\u0000-\u001f\u007f]/.test(config.model)) {
      throw new Error('local cognition model name is required');
    }
    this.endpoint = chatCompletionsUrl(config.baseUrl);
  }

  async reason(
    request: CognitiveRequestDto,
    signal: AbortSignal,
  ): Promise<CognitiveProposalDto | null> {
    if (signal.aborted || request.allowedIntents.length === 0) return null;

    const response = await this.fetcher(this.endpoint, {
      method: 'POST',
      redirect: 'error',
      headers: {
        'content-type': 'application/json',
      },
      signal,
      body: JSON.stringify({
        model: this.config.model,
        temperature: 0,
        max_tokens: 180,
        messages: [
          {
            role: 'system',
            content: cognitiveSystemPrompt(),
          },
          {
            role: 'user',
            content: JSON.stringify(request),
          },
        ],
        response_format: {
          type: 'json_schema',
          json_schema: {
            name: 'lifelens_cognitive_proposal',
            strict: true,
            schema: cognitiveProposalJsonSchema(request),
          },
        },
      }),
    });

    if (!response.ok) {
      throw new CognitiveProviderError('http_error', `local cognition HTTP ${response.status}`);
    }

    let payload: {
      choices?: Array<{
        message?: {
          content?: unknown;
        };
      }>;
    };
    try { payload = await response.json(); }
    catch { throw new CognitiveProviderError('invalid_json', 'local cognition response is not valid JSON'); }
    const content = payload?.choices?.[0]?.message?.content;
    if (typeof content !== 'string' || content.length === 0 || content.length > 8192) {
      throw new CognitiveProviderError('malformed_response', 'local cognition response is missing JSON content');
    }

    let parsed: unknown;
    try {
      parsed = JSON.parse(content);
    } catch {
      throw new CognitiveProviderError('invalid_json', 'local cognition response is not valid JSON');
    }
    try { return parseCognitiveProposal(parsed, request); }
    catch { throw new CognitiveProviderError('invalid_schema', 'local cognition proposal violates the Core request contract'); }
  }
}
