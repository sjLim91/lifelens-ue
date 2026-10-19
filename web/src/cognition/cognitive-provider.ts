import type {
  CognitiveProposalDto,
  CognitiveRequestDto,
} from './cognitive-contract';

export interface CognitiveProvider {
  readonly name: string;
  // Optional owner teardown: in-browser GPU engines must release model memory
  // when replaced; Core/world resets alone never unload the shared mind.
  dispose?(): void | Promise<void>;

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
