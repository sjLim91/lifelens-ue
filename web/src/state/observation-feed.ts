import type {
  Resident,
  ResidentMemory,
  ResidentRelationship,
  WorldOverview,
} from '../runtime/core-types';

export type ObservationKind =
  | 'activity'
  | 'memory'
  | 'relationship'
  | 'family'
  | 'life';

export interface ObservationEvent {
  id: string;
  kind: ObservationKind;
  minute: number;
  residentId?: string;
  residentName?: string;
  summary: string;
  detail?: string;
  importance: 'high' | 'medium' | 'low';
}

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function percent(value: unknown): string {
  return `${Math.round(clamp01(value) * 100)}%`;
}

function memoryKey(memory: ResidentMemory): string {
  return [
    memory.minute ?? -1,
    memory.sourceCharacter ?? '',
    memory.who ?? '',
    memory.what ?? '',
    memory.where ?? '',
    memory.source ?? '',
  ].join('|');
}

function relationshipMap(
  resident: Resident,
): Map<string, ResidentRelationship> {
  return new Map(
    (resident.relationships ?? [])
      .filter((relation) => Boolean(relation.targetId))
      .map((relation) => [relation.targetId, relation]),
  );
}

function activitySignature(resident: Resident): string {
  const context = resident.actionContext;
  return [
    resident.activityKind ?? '',
    resident.activityLabel ?? '',
    resident.activityTargetId ?? '',
    resident.physicalGoal ?? '',
    resident.socialIntent ?? '',
    context?.active ? 'active' : '',
    context?.kind ?? '',
    context?.targetResidentId ?? '',
    context?.hasSpatialTarget ? 'spatial' : '',
    context?.targetGridX ?? '',
    context?.targetGridY ?? '',
  ].join('|');
}

function actionContextDetail(resident: Resident): string | undefined {
  const context = resident.actionContext;
  if (!context?.active) return undefined;

  const labels: Record<string, string> = {
    Social: '사회 상호작용',
    Civilization: '생활/작업',
    Parenting: '돌봄',
    KnowledgeTeaching: '가르침',
  };
  const label = context.kind
    ? labels[context.kind] ?? context.kind
    : '실행 맥락';

  if (
    context.hasSpatialTarget
    && Number.isFinite(context.targetGridX)
    && Number.isFinite(context.targetGridY)
  ) {
    return `${label} · 목표 위치 ${context.targetGridX}, ${context.targetGridY}`;
  }

  if (context.targetResidentId && context.targetResidentId !== '0') {
    return `${label} · 주민 대상 행동`;
  }

  return label;
}

function sameWorld(
  previous: WorldOverview,
  next: WorldOverview,
): boolean {
  if (
    previous.worldSeed === undefined
    || next.worldSeed === undefined
  ) {
    return false;
  }
  return String(previous.worldSeed) === String(next.worldSeed);
}

function relationshipChange(
  resident: Resident,
  previous: Resident,
  minute: number,
): ObservationEvent | null {
  const before = relationshipMap(previous);
  const dimensions = [
    ['socialBond', '유대'],
    ['trust', '신뢰'],
    ['conflict', '갈등'],
    ['romancePotential', '연애 감정'],
  ] as const;

  let strongest:
    | {
        target: ResidentRelationship;
        label: string;
        before: number;
        after: number;
        delta: number;
      }
    | undefined;

  for (const target of resident.relationships ?? []) {
    const previousTarget = before.get(target.targetId);
    if (!previousTarget) continue;

    for (const [key, label] of dimensions) {
      const beforeValue = clamp01(previousTarget[key]);
      const afterValue = clamp01(target[key]);
      const delta = afterValue - beforeValue;
      if (Math.abs(delta) < 0.12) continue;

      if (!strongest || Math.abs(delta) > Math.abs(strongest.delta)) {
        strongest = {
          target,
          label,
          before: beforeValue,
          after: afterValue,
          delta,
        };
      }
    }
  }

  if (!strongest) return null;
  const direction = strongest.delta > 0 ? '높아짐' : '낮아짐';
  const targetName = strongest.target.targetName
    || strongest.target.targetId;

  return {
    id: [
      'relationship',
      minute,
      resident.id,
      strongest.target.targetId,
      strongest.label,
      strongest.after.toFixed(3),
    ].join(':'),
    kind: 'relationship',
    minute,
    residentId: resident.id,
    residentName: resident.name,
    summary: `${resident.name} ↔ ${targetName}: ${strongest.label}가 ${direction}`,
    detail: `${percent(strongest.before)} → ${percent(strongest.after)}`,
    importance: strongest.label === '갈등'
      || strongest.label === '연애 감정'
      ? 'high'
      : 'medium',
  };
}

function newMemoryEvent(
  resident: Resident,
  previous: Resident,
  minute: number,
): ObservationEvent | null {
  const before = new Set(
    (previous.memories ?? []).map(memoryKey),
  );

  const candidate = (resident.memories ?? [])
    .filter((memory) => !before.has(memoryKey(memory)))
    .filter((memory) => (
      clamp01(memory.importance) >= 0.55
      || (
        memory.source === 'DirectWitness'
        && clamp01(memory.emotionIntensity) >= 0.45
      )
    ))
    .sort((a, b) => (
      clamp01(b.importance) - clamp01(a.importance)
    ))[0];

  if (!candidate) return null;

  return {
    id: `memory:${resident.id}:${memoryKey(candidate)}`,
    kind: 'memory',
    minute: Number(candidate.minute) || minute,
    residentId: resident.id,
    residentName: resident.name,
    summary: `${resident.name}에게 중요한 기억이 남음`,
    detail: [
      candidate.what || '새로운 기억',
      candidate.where || '',
    ].filter(Boolean).join(' · '),
    importance: clamp01(candidate.importance) >= 0.75
      ? 'high'
      : 'medium',
  };
}

function familyEvents(
  resident: Resident,
  previous: Resident,
  minute: number,
): ObservationEvent[] {
  const events: ObservationEvent[] = [];
  const before = previous.family;
  const after = resident.family;

  if (!after) return events;

  if (
    after.hasActivePartner
    && after.partnerId
    && after.partnerId !== before?.partnerId
  ) {
    events.push({
      id: `family:partner:${minute}:${resident.id}:${after.partnerId}`,
      kind: 'family',
      minute,
      residentId: resident.id,
      residentName: resident.name,
      summary: `${resident.name}의 관계가 새로운 단계로 들어감`,
      detail: after.partnerName
        ? `파트너: ${after.partnerName}`
        : '새 파트너 관계',
      importance: 'high',
    });
  }

  if (after.expectingChild && !before?.expectingChild) {
    events.push({
      id: `family:pregnancy:${minute}:${resident.id}`,
      kind: 'family',
      minute,
      residentId: resident.id,
      residentName: resident.name,
      summary: `${resident.name}의 가족에 임신 변화가 생김`,
      detail: after.pregnancyPartnerName
        ? `함께하는 사람: ${after.pregnancyPartnerName}`
        : undefined,
      importance: 'high',
    });
  }

  const beforeChildren = before?.children?.length ?? 0;
  const afterChildren = after.children?.length ?? 0;
  if (afterChildren > beforeChildren) {
    events.push({
      id: `family:child:${minute}:${resident.id}:${afterChildren}`,
      kind: 'family',
      minute,
      residentId: resident.id,
      residentName: resident.name,
      summary: `${resident.name}의 가족에 새 아이가 생김`,
      detail: `자녀 ${beforeChildren} → ${afterChildren}`,
      importance: 'high',
    });
  }

  return events;
}

function activityEvent(
  resident: Resident,
  previous: Resident,
  minute: number,
): ObservationEvent | null {
  if (activitySignature(resident) === activitySignature(previous)) {
    return null;
  }

  const targetChanged =
    (resident.activityTargetId ?? '')
    !== (previous.activityTargetId ?? '');
  const kindChanged =
    (resident.activityKind ?? '')
    !== (previous.activityKind ?? '');

  if (
    resident.activityKind === 'Idle'
    && !targetChanged
    && !kindChanged
  ) {
    return null;
  }

  const label = resident.activityLabel?.trim();
  if (!label) return null;

  const target = resident.activityTargetName?.trim();
  const contextDetail = actionContextDetail(resident);
  return {
    id: [
      'activity',
      minute,
      resident.id,
      resident.activityKind ?? '',
      label,
      resident.activityTargetId ?? '',
      resident.actionContext?.kind ?? '',
      resident.actionContext?.targetGridX ?? '',
      resident.actionContext?.targetGridY ?? '',
    ].join(':'),
    kind: 'activity',
    minute,
    residentId: resident.id,
    residentName: resident.name,
    summary: target
      ? `${resident.name}: ${label} → ${target}`
      : `${resident.name}: ${label}`,
    detail: contextDetail,
    importance: resident.activityKind === 'Social'
      || resident.actionContext?.active
      ? 'medium'
      : 'low',
  };
}

function importanceRank(
  importance: ObservationEvent['importance'],
): number {
  switch (importance) {
    case 'high': return 3;
    case 'medium': return 2;
    default: return 1;
  }
}

export function deriveObservationEvents(
  previousWorld: WorldOverview,
  previousResidents: Resident[],
  nextWorld: WorldOverview,
  nextResidents: Resident[],
): ObservationEvent[] {
  if (!sameWorld(previousWorld, nextWorld)) {
    return [];
  }

  const minute = Number(nextWorld.minute) || 0;
  const previousById = new Map(
    previousResidents.map((resident) => [resident.id, resident]),
  );
  const events: ObservationEvent[] = [];

  const previousMajorEvents = Number(previousWorld.majorLifeEvents) || 0;
  const nextMajorEvents = Number(nextWorld.majorLifeEvents) || 0;
  if (nextMajorEvents > previousMajorEvents) {
    events.push({
      id: `life:${minute}:${nextMajorEvents}`,
      kind: 'life',
      minute,
      summary: nextMajorEvents - previousMajorEvents === 1
        ? '새로운 주요 생애사건이 발생함'
        : `주요 생애사건이 ${nextMajorEvents - previousMajorEvents}건 늘어남`,
      detail: `누적 ${nextMajorEvents}건`,
      importance: 'high',
    });
  }

  for (const resident of nextResidents) {
    const previous = previousById.get(resident.id);
    if (!previous) continue;

    events.push(...familyEvents(resident, previous, minute));

    const memory = newMemoryEvent(resident, previous, minute);
    if (memory) events.push(memory);

    const relationship = relationshipChange(
      resident,
      previous,
      minute,
    );
    if (relationship) events.push(relationship);

    const activity = activityEvent(resident, previous, minute);
    if (activity) events.push(activity);
  }

  return events
    .sort((a, b) => (
      importanceRank(b.importance) - importanceRank(a.importance)
    ))
    .slice(0, 6);
}

export function mergeObservationEvents(
  incoming: ObservationEvent[],
  existing: ObservationEvent[],
  limit = 12,
): ObservationEvent[] {
  const seen = new Set<string>();
  const merged: ObservationEvent[] = [];

  for (const event of [...incoming, ...existing]) {
    if (seen.has(event.id)) continue;
    seen.add(event.id);
    merged.push(event);
    if (merged.length >= limit) break;
  }

  return merged;
}
