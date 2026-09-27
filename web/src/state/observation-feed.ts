import type {
  CivilizationDiscovery,
  CivilizationWorldPayload,
  RecentSocialEvent,
  RecentSocialEventsPayload,
  Resident,
  ResidentLifeEvent,
  ResidentMemory,
  ResidentRelationship,
  WorldObjectsPayload,
  WorldOverview,
} from '../runtime/core-types';

export type ObservationKind =
  | 'activity'
  | 'memory'
  | 'relationship'
  | 'family'
  | 'life'
  | 'social'
  | 'civilization'
  | 'facility'
  | 'sanitation';

export interface ObservationEvent {
  id: string;
  kind: ObservationKind;
  minute: number;
  residentId?: string;
  residentName?: string;
  targetResidentId?: string;
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
  return [
    resident.activityKind ?? '',
    resident.activityLabel ?? '',
    resident.activityTargetId ?? '',
    resident.physicalGoal ?? '',
    resident.socialIntent ?? '',
  ].join('|');
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

function residentNames(residents: Resident[]): Map<string, string> {
  return new Map(
    residents.map((resident) => [resident.id, resident.name]),
  );
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
    targetResidentId: strongest.target.targetId,
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
      targetResidentId: after.partnerId,
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

function socialEventKey(event: RecentSocialEvent): string {
  if (event.sequence && event.sequence !== '0') {
    return `sequence:${event.sequence}`;
  }
  return [
    event.minute,
    event.actorId,
    event.targetId,
    event.type,
    event.where,
  ].join('|');
}

function socialImportance(
  event: RecentSocialEvent,
): ObservationEvent['importance'] {
  if (
    event.presentationLevel === 'Important'
    || clamp01(event.importance) >= 0.75
  ) {
    return 'high';
  }
  if (
    event.presentationLevel === 'Meaningful'
    || clamp01(event.importance) >= 0.45
  ) {
    return 'medium';
  }
  return 'low';
}

function socialSummary(
  event: RecentSocialEvent,
  actorName: string,
  targetName: string,
): string {
  switch (event.type) {
    case 'Help': return `${actorName}가 ${targetName}을 도움`;
    case 'Comfort': return `${actorName}가 ${targetName}을 위로함`;
    case 'Conflict': return `${actorName}와 ${targetName} 사이에 갈등이 생김`;
    case 'Betrayal': return `${actorName}가 ${targetName}을 배신함`;
    case 'Rejection': return `${targetName}이(가) ${actorName}의 접근을 거절함`;
    case 'Apology': return `${actorName}가 ${targetName}에게 사과함`;
    case 'Intimacy': return `${actorName}와 ${targetName}의 관계가 가까워짐`;
    case 'Commitment': return `${actorName}와 ${targetName}가 관계를 약속함`;
    case 'PositiveInteraction':
    default:
      return `${actorName}와 ${targetName}가 좋은 상호작용을 나눔`;
  }
}

function exactSocialEvents(
  previous: RecentSocialEventsPayload | undefined,
  next: RecentSocialEventsPayload | undefined,
  residents: Resident[],
): ObservationEvent[] {
  if (
    previous?.available !== true
    || next?.available !== true
  ) {
    return [];
  }

  const names = residentNames(residents);
  const before = new Set(
    (previous.events ?? []).map(socialEventKey),
  );
  const result: ObservationEvent[] = [];

  for (const event of next.events ?? []) {
    const key = socialEventKey(event);
    if (before.has(key)) continue;

    const actorName = names.get(event.actorId) || event.actorId;
    const targetName = names.get(event.targetId) || event.targetId;
    const detail = [
      event.where?.trim() || '',
      `강도 ${percent(event.intensity)}`,
    ].filter(Boolean).join(' · ');

    result.push({
      id: `social:${key}`,
      kind: 'social',
      minute: Number(event.minute) || 0,
      residentId: event.actorId || undefined,
      residentName: actorName,
      targetResidentId: event.targetId || undefined,
      summary: socialSummary(event, actorName, targetName),
      detail,
      importance: socialImportance(event),
    });
  }

  return result;
}

function lifeEventCanonicalKey(
  subjectId: string,
  event: ResidentLifeEvent,
): string {
  const participants = Array.from(new Set([
    subjectId,
    ...(event.relatedCharacterIds ?? []),
  ].filter(Boolean))).sort();
  return [
    event.type ?? '',
    Number(event.minute) || 0,
    participants.join(','),
    Number(event.value) || 0,
  ].join('|');
}

function lifeImportance(
  type: string | undefined,
): ObservationEvent['importance'] {
  switch (type) {
    case 'Birth':
    case 'DatingStarted':
    case 'Engaged':
    case 'Married':
    case 'CohabitationStarted':
    case 'PregnancyStarted':
    case 'ChildBorn':
    case 'Separated':
    case 'Divorced':
    case 'PartnerWidowed':
    case 'Death':
    case 'Bereavement':
      return 'high';
    case 'LifeStageChanged':
    case 'ParentingMilestone':
    case 'HouseholdChanged':
      return 'medium';
    default:
      return 'low';
  }
}

function lifeSummary(
  resident: Resident,
  event: ResidentLifeEvent,
): string {
  switch (event.type) {
    case 'Birth': return `${resident.name}가 태어남`;
    case 'LifeStageChanged': return `${resident.name}가 새로운 생애단계에 들어감`;
    case 'DatingStarted': return `${resident.name}의 연애가 시작됨`;
    case 'Engaged': return `${resident.name}가 약혼함`;
    case 'Married': return `${resident.name}가 결혼함`;
    case 'CohabitationStarted': return `${resident.name}가 동거를 시작함`;
    case 'PregnancyStarted': return `${resident.name}의 가족에 임신이 시작됨`;
    case 'ChildBorn': return `${resident.name}의 가족에 아이가 태어남`;
    case 'ParentingMilestone': return `${resident.name}의 육아에 새로운 변화가 생김`;
    case 'Separated': return `${resident.name}의 관계가 별거 단계로 바뀜`;
    case 'Divorced': return `${resident.name}가 이혼함`;
    case 'PartnerWidowed': return `${resident.name}가 배우자를 잃음`;
    case 'HouseholdChanged': return `${resident.name}의 가구 구성이 바뀜`;
    case 'Death': return `${resident.name}가 사망함`;
    case 'Bereavement': return `${resident.name}가 가까운 사람의 죽음을 겪음`;
    default: return `${resident.name}에게 생애 변화가 생김`;
  }
}

function exactLifeEvents(
  previousResidents: Resident[],
  nextResidents: Resident[],
): ObservationEvent[] {
  const previousSupportsHistory = previousResidents.some(
    resident => Array.isArray(resident.lifeHistory),
  );
  const nextSupportsHistory = nextResidents.some(
    resident => Array.isArray(resident.lifeHistory),
  );
  if (!previousSupportsHistory || !nextSupportsHistory) return [];

  const names = residentNames(nextResidents);
  const before = new Set<string>();
  for (const resident of previousResidents) {
    for (const event of resident.lifeHistory ?? []) {
      before.add(lifeEventCanonicalKey(resident.id, event));
    }
  }

  const emitted = new Set<string>();
  const result: ObservationEvent[] = [];
  for (const resident of nextResidents) {
    for (const event of resident.lifeHistory ?? []) {
      const key = lifeEventCanonicalKey(resident.id, event);
      if (before.has(key) || emitted.has(key)) continue;
      emitted.add(key);

      const relatedNames = (event.relatedCharacterIds ?? [])
        .map(id => names.get(id) || '')
        .filter(Boolean);

      result.push({
        id: `life:${key}`,
        kind: 'life',
        minute: Number(event.minute) || 0,
        residentId: resident.id,
        residentName: resident.name,
        targetResidentId: event.relatedCharacterIds?.[0],
        summary: lifeSummary(resident, event),
        detail: relatedNames.length > 0
          ? `관련: ${relatedNames.join(', ')}`
          : undefined,
        importance: lifeImportance(event.type),
      });
    }
  }
  return result;
}

function discoveryKey(discovery: CivilizationDiscovery): string {
  return discovery.factId || [
    discovery.minute,
    discovery.discovererId,
    discovery.technique,
  ].join('|');
}

function exactCivilizationEvents(
  previous: CivilizationWorldPayload | undefined,
  next: CivilizationWorldPayload | undefined,
  residents: Resident[],
): ObservationEvent[] {
  if (previous?.available !== true || next?.available !== true) return [];

  const minute = Number(next.minute) || 0;
  const names = residentNames(residents);
  const events: ObservationEvent[] = [];

  const previousDiscoveries = new Set(
    (previous.recentDiscoveries ?? []).map(discoveryKey),
  );
  for (const discovery of next.recentDiscoveries ?? []) {
    const key = discoveryKey(discovery);
    if (previousDiscoveries.has(key)) continue;
    const discovererName = discovery.discovererName
      || names.get(discovery.discovererId)
      || discovery.discovererId
      || '누군가';
    events.push({
      id: `civilization:discovery:${key}`,
      kind: 'civilization',
      minute: Number(discovery.minute) || minute,
      residentId: discovery.discovererId || undefined,
      residentName: discovererName,
      summary: `${discovererName}가 ${discovery.technique || '새 기술'} 지식을 발견함`,
      detail: discovery.livingKnowerCount > 1
        ? `현재 ${discovery.livingKnowerCount}명이 알고 있음`
        : '아직 개인 지식에 가까움',
      importance: discovery.livingKnowerCount >= 3 ? 'high' : 'medium',
    });
  }

  const previousFacilities = new Map(
    (previous.facilities ?? []).map(facility => [facility.id, facility]),
  );
  for (const facility of next.facilities ?? []) {
    const before = previousFacilities.get(facility.id);
    if (!before) {
      const workerId = facility.initiatedBy || facility.lastWorkedBy;
      events.push({
        id: `facility:new:${facility.id}:${facility.startedMinute}`,
        kind: 'facility',
        minute: Number(facility.startedMinute) || minute,
        residentId: workerId || undefined,
        residentName: workerId ? names.get(workerId) : undefined,
        summary: `${facility.kind || '시설'} 작업이 시작됨`,
        detail: facility.state || undefined,
        importance: facility.state === 'Operational' ? 'high' : 'medium',
      });
      continue;
    }
    if (before.state !== facility.state) {
      const workerId = facility.lastWorkedBy || facility.initiatedBy;
      const operational = facility.state === 'Operational';
      const ruined = facility.state === 'Ruined';
      events.push({
        id: `facility:state:${facility.id}:${facility.state}:${minute}`,
        kind: 'facility',
        minute,
        residentId: workerId || undefined,
        residentName: workerId ? names.get(workerId) : undefined,
        summary: operational
          ? `${facility.kind || '시설'}이 완성되어 가동을 시작함`
          : ruined
            ? `${facility.kind || '시설'}이 파손됨`
            : `${facility.kind || '시설'} 상태가 ${facility.state}(으)로 바뀜`,
        importance: operational || ruined ? 'high' : 'medium',
      });
    }
  }

  const previousResources = new Map(
    (previous.resources ?? []).map(resource => [resource.id, resource]),
  );
  for (const resource of next.resources ?? []) {
    const before = previousResources.get(resource.id);
    if (!before) continue;
    if (Number(before.quantity) > 0 && Number(resource.quantity) <= 0) {
      events.push({
        id: `civilization:depleted:${resource.id}:${minute}`,
        kind: 'civilization',
        minute,
        summary: `${resource.material || '자원'} 노드가 고갈됨`,
        detail: `좌표 ${resource.gridX}, ${resource.gridY}`,
        importance: 'medium',
      });
    }
  }

  return events;
}

function exactWorldObjectEvents(
  previous: WorldObjectsPayload | undefined,
  next: WorldObjectsPayload | undefined,
  minute: number,
  residents: Resident[],
): ObservationEvent[] {
  if (previous?.available !== true || next?.available !== true) return [];

  const names = residentNames(residents);
  const beforeSites = new Map(
    (previous.sanitationSites ?? []).map(site => [site.id, site]),
  );
  const events: ObservationEvent[] = [];

  for (const site of next.sanitationSites ?? []) {
    const before = beforeSites.get(site.id);
    if (!before) {
      const residentId = site.establishedBy || undefined;
      events.push({
        id: `sanitation:new:${site.id}:${site.establishedMinute}`,
        kind: 'sanitation',
        minute: Number(site.establishedMinute) || minute,
        residentId,
        residentName: residentId ? names.get(residentId) : undefined,
        summary: site.kind === 'DugPit'
          ? '새 위생 구덩이가 마련됨'
          : '새 위생 구역이 지정됨',
        detail: `좌표 ${site.gridX}, ${site.gridY}`,
        importance: 'medium',
      });
      continue;
    }

    if (
      Number(site.improvedMinute) > 0
      && Number(site.improvedMinute) !== Number(before.improvedMinute)
    ) {
      const residentId = site.improvedBy || undefined;
      events.push({
        id: `sanitation:improved:${site.id}:${site.improvedMinute}`,
        kind: 'sanitation',
        minute: Number(site.improvedMinute) || minute,
        residentId,
        residentName: residentId ? names.get(residentId) : undefined,
        summary: '위생 장소가 개선됨',
        detail: `이용 누적 ${site.useCount}회`,
        importance: 'medium',
      });
    }
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
  return {
    id: [
      'activity',
      minute,
      resident.id,
      resident.activityKind ?? '',
      label,
      resident.activityTargetId ?? '',
    ].join(':'),
    kind: 'activity',
    minute,
    residentId: resident.id,
    residentName: resident.name,
    targetResidentId: resident.activityTargetId || undefined,
    summary: target
      ? `${resident.name}: ${label} → ${target}`
      : `${resident.name}: ${label}`,
    importance: resident.activityKind === 'Social'
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
  previousSocialEvents?: RecentSocialEventsPayload,
  nextSocialEvents?: RecentSocialEventsPayload,
  previousCivilization?: CivilizationWorldPayload,
  nextCivilization?: CivilizationWorldPayload,
  previousWorldObjects?: WorldObjectsPayload,
  nextWorldObjects?: WorldObjectsPayload,
): ObservationEvent[] {
  if (!sameWorld(previousWorld, nextWorld)) {
    return [];
  }

  const minute = Number(nextWorld.minute) || 0;
  const previousById = new Map(
    previousResidents.map((resident) => [resident.id, resident]),
  );
  const events: ObservationEvent[] = [];

  const exactSocial = exactSocialEvents(
    previousSocialEvents,
    nextSocialEvents,
    nextResidents,
  );
  events.push(...exactSocial);
  events.push(...exactCivilizationEvents(
    previousCivilization,
    nextCivilization,
    nextResidents,
  ));
  events.push(...exactWorldObjectEvents(
    previousWorldObjects,
    nextWorldObjects,
    minute,
    nextResidents,
  ));

  const socialParticipants = new Set<string>();
  for (const event of exactSocial) {
    if (event.residentId) socialParticipants.add(event.residentId);
    if (event.targetResidentId) socialParticipants.add(event.targetResidentId);
  }

  const historySupported = previousResidents.some(
    resident => Array.isArray(resident.lifeHistory),
  ) && nextResidents.some(
    resident => Array.isArray(resident.lifeHistory),
  );

  if (historySupported) {
    events.push(...exactLifeEvents(previousResidents, nextResidents));
  } else {
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
  }

  for (const resident of nextResidents) {
    const previous = previousById.get(resident.id);
    if (!previous) continue;

    if (!historySupported) {
      events.push(...familyEvents(resident, previous, minute));
    }

    const memory = newMemoryEvent(resident, previous, minute);
    if (memory) events.push(memory);

    if (!socialParticipants.has(resident.id)) {
      const relationship = relationshipChange(
        resident,
        previous,
        minute,
      );
      if (relationship) events.push(relationship);
    }

    if (
      !socialParticipants.has(resident.id)
      || resident.activityKind !== 'Social'
    ) {
      const activity = activityEvent(resident, previous, minute);
      if (activity) events.push(activity);
    }
  }

  return events
    .sort((a, b) => {
      const importance = importanceRank(b.importance)
        - importanceRank(a.importance);
      return importance !== 0 ? importance : b.minute - a.minute;
    })
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
