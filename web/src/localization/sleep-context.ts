import type { ResidentPresentationDirective } from '../runtime/core-types';

const sleepContextLabels: Record<string, string> = {
  Protected: '수면 중',
  Exposed: '야외 취침 중',
  ExposedEmergency: '악천후 속 임시 취침 중',
};

// Only Core-authored interaction context changes the label. No weather inference.
export function sleepContextActionText(
  presentation: ResidentPresentationDirective | undefined,
): string | null {
  if (!presentation?.active || presentation.kind !== 'Physical'
      || presentation.physicalGoal !== 'Sleep' || presentation.phase !== 'Interacting') return null;
  return sleepContextLabels[presentation.sleepContext ?? ''] ?? null;
}
