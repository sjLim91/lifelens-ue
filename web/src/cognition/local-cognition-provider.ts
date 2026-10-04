import {
  cognitiveProposalJsonSchema,
  parseCognitiveProposal,
  type CognitiveProposalDto,
  type CognitiveRequestDto,
} from './cognitive-contract';
import type { CognitiveProvider } from './cognitive-provider';

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
    ) {
      unique.add(entry.id.trim());
    }
    if (unique.size >= 32) break;
  }
  return [...unique];
}

function chatCompletionsUrl(baseUrl: string): string {
  const normalized = baseUrl.endsWith('/') ? baseUrl : `${baseUrl}/`;
  return new URL('v1/chat/completions', normalized).toString();
}

export function cognitiveSystemPrompt(): string {
  return [
    'You are the strategic reasoning layer for one LifeLens resident.',
    'Use only facts in the supplied request.',
    'Choose only one allowed intent.',
    'Never invent residents, resources, facilities, technologies, or outcomes.',
    'Return only JSON matching the schema.',
    'Do not provide hidden chain-of-thought.',
    'Keep rationale short and state only the decision-relevant reason.',
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
    if (!config.model.trim()) {
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
      throw new Error(`local cognition HTTP ${response.status}`);
    }

    const payload = await response.json() as {
      choices?: Array<{
        message?: {
          content?: unknown;
        };
      }>;
    };
    const content = payload.choices?.[0]?.message?.content;
    if (typeof content !== 'string' || content.length === 0) {
      throw new Error('local cognition response is missing JSON content');
    }

    let parsed: unknown;
    try {
      parsed = JSON.parse(content);
    } catch {
      throw new Error('local cognition response is not valid JSON');
    }
    return parseCognitiveProposal(parsed, request);
  }
}
