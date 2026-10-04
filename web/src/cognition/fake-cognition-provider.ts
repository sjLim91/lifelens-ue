import {
  cognitiveIntentTargetsResident,
  type CognitiveProposalDto,
  type CognitiveRequestDto,
} from './cognitive-contract';
import type { CognitiveProvider } from './cognitive-provider';

export class DeterministicFakeCognitionProvider implements CognitiveProvider {
  readonly name = 'deterministic-fake';

  async reason(
    request: CognitiveRequestDto,
    signal: AbortSignal,
  ): Promise<CognitiveProposalDto | null> {
    if (signal.aborted) return null;

    for (const intent of request.allowedIntents) {
      if (cognitiveIntentTargetsResident(intent)) {
        const target = request.relationships
          .map((relationship) => relationship.target)
          .find((id) => id.length > 0 && id !== request.actor);
        if (!target) continue;
        return {
          actor: request.actor,
          intent,
          priority: 0.5,
          targetResident: target,
          rationale: 'Deterministic fixture proposal.',
        };
      }

      return {
        actor: request.actor,
        intent,
        priority: 0.5,
        targetResident: null,
        rationale: 'Deterministic fixture proposal.',
      };
    }

    return null;
  }
}
