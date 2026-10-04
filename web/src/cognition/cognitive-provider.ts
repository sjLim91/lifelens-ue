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
