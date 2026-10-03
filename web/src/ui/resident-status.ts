import type { Resident } from '../runtime/core-types';
import { residentActionCue } from '../render/resident-action-context';
import { formatActivity } from './observer-format';

export function residentStatusText(
  resident: Resident,
  residents: Resident[],
): string {
  const presentation = resident.presentation;
  if (presentation?.active && presentation.phase === 'Interacting'
      && presentation.physicalGoal === 'Sleep') {
    if (presentation.sleepContext === 'ExposedEmergency') return '악천후 속 임시 취침 중';
    if (presentation.sleepContext === 'Exposed') return '야외 취침 중';
    if (presentation.sleepContext === 'Protected') return '수면 중';
  }
  const cue = residentActionCue(resident, residents);
  return cue?.text ?? formatActivity(resident.activityLabel);
}

