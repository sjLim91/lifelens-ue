import type { Resident } from '../runtime/core-types';
import { residentActionCue } from '../render/resident-action-context';
import { formatActivity } from './observer-format';

export function residentStatusText(
  resident: Resident,
  residents: Resident[],
): string {
  const cue = residentActionCue(resident, residents);
  return cue?.text ?? formatActivity(resident.activityLabel);
}
