import type {
  CognitiveProposalDto,
  CognitiveRequestDto,
} from './cognitive-contract';

export interface CognitiveProvider {
  readonly name: string;

  reason(
    request: CognitiveRequestDto,
    signal: AbortSignal,
  ): Promise<CognitiveProposalDto | null>;
}

export type CognitiveProviderFailure = 'invalid_json' | 'invalid_schema' | 'http_error' | 'malformed_response';

export class CognitiveProviderError extends Error {
  constructor(readonly reason: CognitiveProviderFailure, message: string) {
    super(message);
    this.name = 'CognitiveProviderError';
  }
}
