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
import {
  formatActivity,
  formatFacilityKind,
  formatFacilityState,
  formatLocationText,
  formatMaterial,
  formatMemoryText,
  formatParentingAction,
  formatSocialIntent,
  formatTechnique,
} from '../localization/korean';

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
  focusGridX?: number;
  focusGridY?: number;
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

function residentFocus(
  resident: Resident | undefined,
): Pick<ObservationEvent, 'focusGridX' | 'focusGridY'> {
  if (
    !resident?.hasPosition
    || resident.gridX === undefined
    || resident.gridY === undefined
  ) {
    return {};
  }
  return {
    focusGridX: resident.gridX,
    focusGridY: resident.gridY,
  };
}

function presentationFocus(
  resident: Resident,
): Pick<ObservationEvent, 'focusGridX' | 'focusGridY'> {
  const presentation = resident.presentation;
  if (
    presentation?.hasTargetGrid
    && presentation.targetGridX !== undefined
    && presentation.targetGridY !== undefined
  ) {
    return {
      focusGridX: presentation.targetGridX,
      focusGridY: presentation.targetGridY,
    };
  }
  return residentFocus(resident);
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
    ...residentFocus(resident),
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
    ...residentFocus(resident),
    summary: `${resident.name}에게 중요한 기억이 남음`,
    detail: [
      formatMemoryText(candidate.what),
      formatLocationText(candidate.where),
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
      ...residentFocus(resident),
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
      ...residentFocus(resident),
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
      ...residentFocus(resident),
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

const SOCIAL_RELATIONSHIP_RESULT_DIMENSIONS = [
  ['socialBond', '유대'],
  ['trust', '신뢰'],
  ['affection', '애정'],
  ['comfort', '편안함'],
  ['respect', '존중'],
  ['familiarity', '친숙도'],
  ['conflict', '갈등'],
  ['grudge', '원한'],
  ['fear', '두려움'],
  ['jealousy', '질투'],
  ['romancePotential', '연애 감정'],
  ['commitment', '헌신'],
  ['attraction', '끌림'],
  ['romanticInterest', '연애 관심'],
  ['sexualAttraction', '성적 끌림'],
] as const satisfies ReadonlyArray<
  readonly [keyof ResidentRelationship, string]
>;

function signedPercentagePoint(delta: number): string {
  const points = delta * 100;
  const rounded = Math.round(points * 10) / 10;
  return `${rounded > 0 ? '+' : ''}${rounded.toFixed(1)}%p`;
}

function directionalRelationshipResult(
  event: RecentSocialEvent,
  previousResidents: Resident[],
  nextResidents: Resident[],
): string | null {
  // Core relationship state is directional. A social event changes how the
  // recipient currently feels about the actor, not an invented symmetric pair.
  const previousRecipient = previousResidents.find(
    (resident) => resident.id === event.targetId,
  );
  const nextRecipient = nextResidents.find(
    (resident) => resident.id === event.targetId,
  );
  if (!previousRecipient || !nextRecipient) return null;

  const before = relationshipMap(previousRecipient).get(event.actorId);
  const after = relationshipMap(nextRecipient).get(event.actorId);
  if (!before || !after) return null;

  const changes = SOCIAL_RELATIONSHIP_RESULT_DIMENSIONS
    .map(([key, label]) => ({
      label,
      delta: clamp01(after[key]) - clamp01(before[key]),
    }))
    .filter((change) => Math.abs(change.delta) >= 0.005)
    .sort((a, b) => Math.abs(b.delta) - Math.abs(a.delta))
    .slice(0, 3);

  if (changes.length === 0) return null;

  const recipientName = nextRecipient.name || event.targetId;
  const actorName = nextResidents.find(
    (resident) => resident.id === event.actorId,
  )?.name || event.actorId;

  return [
    `${recipientName}→${actorName}`,
    changes
      .map((change) => (
        `${change.label} ${signedPercentagePoint(change.delta)}`
      ))
      .join(' · '),
  ].join(': ');
}

function exactSocialEvents(
  previous: RecentSocialEventsPayload | undefined,
  next: RecentSocialEventsPayload | undefined,
  previousResidents: Resident[],
  nextResidents: Resident[],
): ObservationEvent[] {
  if (
    previous?.available !== true
    || next?.available !== true
  ) {
    return [];
  }

  const names = residentNames(nextResidents);
  const before = new Set(
    (previous.events ?? []).map(socialEventKey),
  );
  const newEvents = (next.events ?? []).filter(
    (event) => !before.has(socialEventKey(event)),
  );

  // Heavy relationship detail and social event history share the same observer
  // refresh cadence. Attribution is safe only when one new event exists for
  // the exact recipient->actor directional pair in this refresh window.
  const directionalPairCounts = new Map<string, number>();
  for (const event of newEvents) {
    const pair = `${event.targetId}->${event.actorId}`;
    directionalPairCounts.set(
      pair,
      (directionalPairCounts.get(pair) ?? 0) + 1,
    );
  }

  const result: ObservationEvent[] = [];
  for (const event of newEvents) {
    const key = socialEventKey(event);
    const actorName = names.get(event.actorId) || event.actorId;
    const targetName = names.get(event.targetId) || event.targetId;
    const actorResident = nextResidents.find(
      (resident) => resident.id === event.actorId,
    );
    const targetResident = nextResidents.find(
      (resident) => resident.id === event.targetId,
    );
    const focus = Object.keys(residentFocus(targetResident)).length > 0
      ? residentFocus(targetResident)
      : residentFocus(actorResident);
    const pair = `${event.targetId}->${event.actorId}`;
    const relationshipResult = directionalPairCounts.get(pair) === 1
      ? directionalRelationshipResult(
          event,
          previousResidents,
          nextResidents,
        )
      : null;
    const detail = [
      formatLocationText(event.where),
      `강도 ${percent(event.intensity)}`,
      relationshipResult,
    ].filter(Boolean).join(' · ');

    result.push({
      id: `social:${key}`,
      kind: 'social',
      minute: Number(event.minute) || 0,
      residentId: event.actorId || undefined,
      residentName: actorName,
      targetResidentId: event.targetId || undefined,
      ...focus,
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
        ...residentFocus(resident),
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
    const discoverer = residents.find(
      (resident) => resident.id === discovery.discovererId,
    );
    events.push({
      id: `civilization:discovery:${key}`,
      kind: 'civilization',
      minute: Number(discovery.minute) || minute,
      residentId: discovery.discovererId || undefined,
      residentName: discovererName,
      ...residentFocus(discoverer),
      summary: `${discovererName}가 ${formatTechnique(discovery.technique)} 지식을 발견함`,
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
        focusGridX: facility.gridX,
        focusGridY: facility.gridY,
        summary: `${formatFacilityKind(facility.kind)} 작업이 시작됨`,
        detail: formatFacilityState(facility.state),
        importance: facility.state === 'Operational' ? 'high' : 'medium',
      });
      continue;
    }
    if (before.state !== facility.state) {
      const workerId = facility.lastWorkedBy || facility.initiatedBy;
      const operational = facility.state === 'Operational';
      const ruined = facility.state === 'Ruined';
      const restored = before.state === 'Ruined' && operational;
      events.push({
        id: `facility:state:${facility.id}:${facility.state}:${minute}`,
        kind: 'facility',
        minute,
        residentId: workerId || undefined,
        residentName: workerId ? names.get(workerId) : undefined,
        focusGridX: facility.gridX,
        focusGridY: facility.gridY,
        summary: restored
          ? `${formatFacilityKind(facility.kind)}이 복구되어 다시 사용 가능해짐`
          : operational
            ? `${formatFacilityKind(facility.kind)}이 완성되어 가동을 시작함`
            : ruined
              ? `${formatFacilityKind(facility.kind)}이 파손됨`
              : `${formatFacilityKind(facility.kind)} 상태가 ${formatFacilityState(facility.state)}(으)로 바뀜`,
        detail: restored
          ? `내구도 ${percent(before.durability)} → ${percent(facility.durability)}`
          : undefined,
        importance: operational || ruined ? 'high' : 'medium',
      });
    } else {
      const durabilityBefore = clamp01(before.durability);
      const durabilityAfter = clamp01(facility.durability);
      if (
        facility.state === 'Operational'
        && durabilityAfter - durabilityBefore >= 0.05
      ) {
        const workerId = facility.lastWorkedBy || facility.initiatedBy;
        events.push({
          id: `facility:repair:${facility.id}:${minute}:${durabilityAfter.toFixed(3)}`,
          kind: 'facility',
          minute,
          residentId: workerId || undefined,
          residentName: workerId ? names.get(workerId) : undefined,
          focusGridX: facility.gridX,
          focusGridY: facility.gridY,
          summary: `${formatFacilityKind(facility.kind)} 내구도가 회복됨`,
          detail: `${percent(durabilityBefore)} → ${percent(durabilityAfter)}`,
          importance: 'medium',
        });
      }
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
        focusGridX: resource.gridX,
        focusGridY: resource.gridY,
        summary: `${formatMaterial(resource.material)} 자원이 고갈됨`,
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
        focusGridX: site.gridX,
        focusGridY: site.gridY,
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
        focusGridX: site.gridX,
        focusGridY: site.gridY,
        summary: '위생 장소가 개선됨',
        detail: `이용 누적 ${site.useCount}회`,
        importance: 'medium',
      });
    }
  }

  return events;
}

function explorationEvent(
  resident: Resident,
  previous: Resident,
  minute: number,
): ObservationEvent | null {
  const next = resident.presentation;
  if (
    !next?.active
    || next.kind !== 'Civilization'
    || next.civilizationIntent !== 'Explore'
    || next.phase === 'Idle'
  ) {
    return null;
  }

  const material = next.civilizationMaterial?.trim();
  if (!material || material === 'Unknown') return null;

  const before = previous.presentation;
  const sameContext = Boolean(
    before?.active
    && before.kind === 'Civilization'
    && before.civilizationIntent === 'Explore'
    && before.civilizationMaterial === next.civilizationMaterial
    && before.contextActionToken === next.contextActionToken
    && before.targetGridX === next.targetGridX
    && before.targetGridY === next.targetGridY
  );
  if (sameContext) return null;

  const token = next.contextActionToken?.trim()
    || String(next.issuedMinute ?? minute);
  const detail = next.hasTargetGrid
    ? `탐색 목표 좌표 ${next.targetGridX ?? 0}, ${next.targetGridY ?? 0}`
    : undefined;

  return {
    id: `civilization:explore:${resident.id}:${token}`,
    kind: 'civilization',
    minute: Number(next.issuedMinute) || minute,
    residentId: resident.id,
    residentName: resident.name,
    ...(next.hasTargetGrid
      ? {
          focusGridX: next.targetGridX,
          focusGridY: next.targetGridY,
        }
      : residentFocus(resident)),
    summary: `${resident.name}: ${formatMaterial(material)} 자원 탐색 시작`,
    detail,
    importance: 'medium',
  };
}

function civilizationPresentationSignature(
  resident: Resident,
): string {
  const presentation = resident.presentation;
  if (
    !presentation?.active
    || presentation.kind !== 'Civilization'
    || (
      presentation.phase !== 'Moving'
      && presentation.phase !== 'Interacting'
    )
  ) {
    return '';
  }

  return [
    presentation.civilizationIntent ?? '',
    presentation.phase ?? '',
    presentation.civilizationMaterial ?? '',
    presentation.civilizationQuantity ?? '',
    presentation.civilizationStorage ?? '',
    presentation.facilityAction ?? '',
    presentation.facilityId ?? '',
    presentation.facilityKind ?? '',
    presentation.contextActionToken ?? '',
    presentation.targetGridX ?? '',
    presentation.targetGridY ?? '',
  ].join('|');
}

function civilizationLogisticsMilestoneEvent(
  resident: Resident,
  previous: Resident,
  minute: number,
): ObservationEvent | null {
  const presentation = resident.presentation;
  if (
    !presentation?.active
    || presentation.kind !== 'Civilization'
    || (
      presentation.phase !== 'Moving'
      && presentation.phase !== 'Interacting'
    )
  ) {
    return null;
  }

  const signature = civilizationPresentationSignature(resident);
  if (
    !signature
    || signature === civilizationPresentationSignature(previous)
  ) {
    return null;
  }

  const intent = presentation.civilizationIntent?.trim() ?? '';
  if (intent === 'Explore') return null;

  const moving = presentation.phase === 'Moving';
  const material = presentation.civilizationMaterial?.trim();
  const materialLabel = material && material !== 'Unknown'
    ? formatMaterial(material)
    : '';
  const quantity = Math.max(
    0,
    Number(presentation.civilizationQuantity) || 0,
  );
  const facilityKind = presentation.facilityKind?.trim();
  const facilityLabel = facilityKind && facilityKind !== 'Unknown'
    ? formatFacilityKind(facilityKind)
    : '시설';
  const detailParts: string[] = [];
  if (quantity > 0) detailParts.push(`수량 ${quantity}`);
  if (materialLabel) detailParts.push(`자재 ${materialLabel}`);

  let summary = '';
  let importance: ObservationEvent['importance'] = 'low';

  if (intent === 'Retrieve' && materialLabel) {
    summary = moving
      ? `${resident.name}: 공동 저장소에서 ${materialLabel}을 가져오러 이동 중`
      : `${resident.name}: 공동 저장소에서 ${materialLabel}을 꺼내는 중`;
    importance = 'medium';
  } else if (intent === 'Store' && materialLabel) {
    summary = moving
      ? `${resident.name}: 공동 저장소에 ${materialLabel}을 보관하러 이동 중`
      : `${resident.name}: 공동 저장소에 ${materialLabel}을 보관하는 중`;
  } else if (intent === 'Craft') {
    switch (presentation.facilityAction) {
      case 'Plan':
        summary = moving
          ? `${resident.name}: ${facilityLabel} 부지를 정하러 이동 중`
          : `${resident.name}: ${facilityLabel} 건설을 계획하는 중`;
        importance = 'medium';
        break;
      case 'DeliverMaterial':
        if (!materialLabel) return null;
        summary = moving
          ? `${resident.name}: ${materialLabel} 자재를 ${facilityLabel} 작업지로 운반 중`
          : `${resident.name}: ${facilityLabel}에 ${materialLabel} 자재를 전달하는 중`;
        importance = 'medium';
        break;
      case 'Work':
        summary = moving
          ? `${resident.name}: ${facilityLabel} 작업지로 이동 중`
          : `${resident.name}: ${facilityLabel} 건설 작업 중`;
        importance = 'medium';
        break;
      case 'Repair':
        summary = moving
          ? `${resident.name}: ${facilityLabel}을 수리·복구하러 이동 중`
          : `${resident.name}: ${facilityLabel} 수리·복구 작업 중`;
        importance = 'medium';
        break;
      default:
        return null;
    }
  } else {
    return null;
  }

  return {
    id: [
      'civilization-logistics',
      minute,
      resident.id,
      intent,
      presentation.facilityAction ?? '',
      presentation.contextActionToken ?? '',
      presentation.phase,
    ].join(':'),
    kind: 'civilization',
    minute,
    residentId: resident.id,
    residentName: resident.name,
    ...presentationFocus(resident),
    summary,
    detail: detailParts.length > 0
      ? detailParts.join(' · ')
      : undefined,
    importance,
  };
}

function physicalPresentationSignature(resident: Resident): string {
  const presentation = resident.presentation;
  if (
    !presentation?.active
    || presentation.kind !== 'Physical'
    || presentation.phase === 'Idle'
  ) {
    return '';
  }

  return [
    presentation.physicalGoal ?? '',
    presentation.phase ?? '',
    presentation.directNaturalWaterSource ? 'natural-water' : '',
    presentation.designatedSanitationSite ? 'designated-sanitation' : '',
    presentation.emergencyFallback ? 'fallback' : '',
    presentation.hasTargetGrid ? presentation.targetGridX ?? '' : '',
    presentation.hasTargetGrid ? presentation.targetGridY ?? '' : '',
    presentation.objectId ?? '',
    presentation.sanitationSiteId ?? '',
  ].join('|');
}

function physicalNeedContext(
  resident: Resident,
  goal: string,
): { label: string; value: number } | null {
  const needs = resident.needs;
  if (!needs) return null;

  switch (goal) {
    case 'Eat':
      return { label: '허기', value: clamp01(needs.hunger) };
    case 'Drink':
      return { label: '갈증', value: clamp01(needs.thirst) };
    case 'Sleep':
      return { label: '피로', value: clamp01(needs.sleep) };
    case 'UseToilet':
      return { label: '배뇨 욕구', value: clamp01(needs.bladder) };
    case 'Wash':
      return { label: '위생 필요', value: clamp01(needs.hygiene) };
    default:
      return null;
  }
}

function physicalMilestoneSummary(
  resident: Resident,
  goal: string,
  phase: 'Moving' | 'Interacting',
): string {
  const presentation = resident.presentation;
  const moving = phase === 'Moving';

  switch (goal) {
    case 'Eat':
      return moving
        ? `${resident.name}: 음식을 먹으러 이동 중`
        : `${resident.name}: 식사를 시작함`;
    case 'Drink':
      if (presentation?.directNaturalWaterSource) {
        return moving
          ? `${resident.name}: 갈증을 해결하려 물가로 이동 중`
          : `${resident.name}: 물가에 도착해 물을 마시기 시작함`;
      }
      return moving
        ? `${resident.name}: 물을 마실 장소로 이동 중`
        : `${resident.name}: 소지한 물을 마시기 시작함`;
    case 'Sleep':
      if (moving) {
        return `${resident.name}: 피로를 풀기 위해 잠자리로 이동 중`;
      }
      return presentation?.hasTargetGrid
        ? `${resident.name}: 잠자리에 도착해 수면을 시작함`
        : `${resident.name}: 피로를 풀기 위해 야외 수면을 시작함`;
    case 'UseToilet':
      if (presentation?.designatedSanitationSite) {
        return moving
          ? `${resident.name}: 위생 장소로 이동 중`
          : `${resident.name}: 위생 장소 이용을 시작함`;
      }
      return moving
        ? `${resident.name}: 야외 용변 장소로 이동 중`
        : `${resident.name}: 야외에서 용변을 시작함`;
    case 'Wash':
      if (presentation?.directNaturalWaterSource) {
        return moving
          ? `${resident.name}: 씻기 위해 물가로 이동 중`
          : `${resident.name}: 물가에 도착해 씻기 시작함`;
      }
      return moving
        ? `${resident.name}: 씻을 장소로 이동 중`
        : `${resident.name}: 소지한 물로 씻기 시작함`;
    default:
      return `${resident.name}: 생활 행동 ${moving ? '이동' : '진행'} 중`;
  }
}

function physicalNeedMilestoneEvent(
  resident: Resident,
  previous: Resident,
  minute: number,
): ObservationEvent | null {
  const presentation = resident.presentation;
  if (
    !presentation?.active
    || presentation.kind !== 'Physical'
    || (
      presentation.phase !== 'Moving'
      && presentation.phase !== 'Interacting'
    )
  ) {
    return null;
  }

  const signature = physicalPresentationSignature(resident);
  if (
    signature.length === 0
    || signature === physicalPresentationSignature(previous)
  ) {
    return null;
  }

  const goal = presentation.physicalGoal?.trim();
  if (!goal || goal === 'Idle') return null;

  const need = physicalNeedContext(resident, goal);
  const detailParts: string[] = [];
  if (need) {
    detailParts.push(`${need.label} ${percent(need.value)}`);
  }
  if (
    (goal === 'Drink' || goal === 'Wash')
    && presentation.directNaturalWaterSource
  ) {
    detailParts.push('자연수 직접 사용');
  } else if (goal === 'Drink' || goal === 'Wash') {
    detailParts.push('소지한 물 사용');
  }
  if (goal === 'UseToilet' && presentation.designatedSanitationSite) {
    detailParts.push('지정 위생 장소');
  }

  const importance: ObservationEvent['importance'] = !need
    ? 'low'
    : need.value >= 0.84
      ? 'high'
      : need.value >= 0.60
        ? 'medium'
        : 'low';

  return {
    id: [
      'physical',
      minute,
      resident.id,
      goal,
      presentation.phase,
      presentation.contextActionToken ?? '',
      presentation.objectId ?? '',
      presentation.sanitationSiteId ?? '',
      presentation.targetGridX ?? '',
      presentation.targetGridY ?? '',
    ].join(':'),
    kind: 'activity',
    minute,
    residentId: resident.id,
    residentName: resident.name,
    ...presentationFocus(resident),
    summary: physicalMilestoneSummary(
      resident,
      goal,
      presentation.phase,
    ),
    detail: detailParts.length > 0
      ? detailParts.join(' · ')
      : undefined,
    importance,
  };
}

function socialPresentationSignature(resident: Resident): string {
  const presentation = resident.presentation;
  if (
    !presentation?.active
    || (
      presentation.kind !== 'Social'
      && presentation.kind !== 'KnowledgeTeaching'
      && presentation.kind !== 'Parenting'
    )
    || presentation.phase === 'Idle'
  ) {
    return '';
  }

  return [
    presentation.kind ?? '',
    presentation.phase ?? '',
    presentation.socialIntent ?? '',
    presentation.parentingAction ?? '',
    presentation.knowledgeTeachingTechnique ?? '',
    presentation.targetResidentId ?? '',
    presentation.contextActionToken ?? '',
    presentation.targetGridX ?? '',
    presentation.targetGridY ?? '',
  ].join('|');
}

function socialMilestoneSummary(
  resident: Resident,
  targetName: string,
): string {
  const presentation = resident.presentation!;
  const moving = presentation.phase === 'Moving';

  if (presentation.kind === 'Social') {
    switch (presentation.socialIntent) {
      case 'Comfort':
        return moving
          ? `${resident.name}: ${targetName}을 위로하러 이동 중`
          : `${resident.name}: ${targetName}을 위로하는 중`;
      case 'Repair':
        return moving
          ? `${resident.name}: ${targetName}와 관계를 회복하려 이동 중`
          : `${resident.name}: ${targetName}와 관계 회복을 시도하는 중`;
      case 'Avoid':
        return `${resident.name}: ${targetName}와 거리를 두는 중`;
      case 'Approach':
        return moving
          ? `${resident.name}: ${targetName}에게 다가가는 중`
          : `${resident.name}: ${targetName}와 교류를 시작함`;
      default:
        return `${resident.name}: ${targetName}와 ${formatSocialIntent(
          presentation.socialIntent,
        )} ${moving ? '위치로 이동 중' : '진행 중'}`;
    }
  }

  if (presentation.kind === 'KnowledgeTeaching') {
    const technique = presentation.knowledgeTeachingTechnique?.trim();
    const techniqueLabel = technique && technique !== 'None'
      ? formatTechnique(technique)
      : '알고 있는 지식';
    return moving
      ? `${resident.name}: ${targetName}에게 ${techniqueLabel}을 가르치러 이동 중`
      : `${resident.name}: ${targetName}에게 ${techniqueLabel}을 가르치는 중`;
  }

  const parenting = formatParentingAction(
    presentation.parentingAction,
  );
  return moving
    ? `${resident.name}: ${targetName}를 돌보러 이동 중 · ${parenting}`
    : `${resident.name}: ${targetName}에게 ${parenting} 진행 중`;
}

function socialInteractionMilestoneEvent(
  resident: Resident,
  previous: Resident,
  residents: Resident[],
  minute: number,
): ObservationEvent | null {
  const presentation = resident.presentation;
  if (
    !presentation?.active
    || (
      presentation.kind !== 'Social'
      && presentation.kind !== 'KnowledgeTeaching'
      && presentation.kind !== 'Parenting'
    )
    || (
      presentation.phase !== 'Moving'
      && presentation.phase !== 'Interacting'
    )
  ) {
    return null;
  }

  const signature = socialPresentationSignature(resident);
  if (
    !signature
    || signature === socialPresentationSignature(previous)
  ) {
    return null;
  }

  const targetId = presentation.targetResidentId?.trim() ?? '';
  if (!targetId || targetId === '0' || targetId === resident.id) {
    return null;
  }
  const target = residents.find((candidate) => candidate.id === targetId);
  if (!target) return null;
  const presentationTarget = presentationFocus(resident);
  const focus = Object.keys(presentationTarget).length > 0
    ? presentationTarget
    : residentFocus(target);

  return {
    id: [
      'social-action',
      minute,
      resident.id,
      targetId,
      presentation.kind,
      presentation.phase,
      presentation.socialIntent ?? '',
      presentation.parentingAction ?? '',
      presentation.knowledgeTeachingTechnique ?? '',
      presentation.contextActionToken ?? '',
    ].join(':'),
    kind: 'activity',
    minute,
    residentId: resident.id,
    residentName: resident.name,
    targetResidentId: targetId,
    ...focus,
    summary: socialMilestoneSummary(resident, target.name),
    importance:
      presentation.kind === 'Social'
      && presentation.socialIntent === 'Approach'
        ? 'low'
        : 'medium',
  };
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
  const localizedLabel = formatActivity(label);

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
    ...residentFocus(resident),
    summary: target
      ? `${resident.name}: ${localizedLabel} → ${target}`
      : `${resident.name}: ${localizedLabel}`,
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
    previousResidents,
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

    const exploration = explorationEvent(resident, previous, minute);
    if (exploration) events.push(exploration);

    const civilizationMilestone = civilizationLogisticsMilestoneEvent(
      resident,
      previous,
      minute,
    );
    if (civilizationMilestone) events.push(civilizationMilestone);

    const physicalMilestone = physicalNeedMilestoneEvent(
      resident,
      previous,
      minute,
    );
    if (physicalMilestone) events.push(physicalMilestone);

    const socialMilestone = socialParticipants.has(resident.id)
      ? null
      : socialInteractionMilestoneEvent(
          resident,
          previous,
          nextResidents,
          minute,
        );
    if (socialMilestone) events.push(socialMilestone);

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
      if (
        activity
        && !civilizationMilestone
        && !physicalMilestone
        && !socialMilestone
      ) {
        events.push(activity);
      }
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
