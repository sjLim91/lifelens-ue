import type {
  CivilizationDiscovery,
  CivilizationWorldFacility,
  CivilizationWorldPayload,
  Resident,
  WorldObjectsPayload,
} from '../runtime/core-types';
import {
  KOREAN_COLLECTIVE_RECORD_STAGE_LABELS,
  KOREAN_SOCIETY_INSTITUTION_LABELS,
  KOREAN_SOCIETY_ROLE_LABELS,
} from '../localization/korean';
import {
  formatFacilityKind,
  formatFacilityState,
  formatMaterial,
  formatPercent,
  formatSanitationSiteKind,
  formatTechnique,
} from './observer-format';

interface TechnologySummary {
  technique: string;
  firstFactId: string;
  firstDiscovererId: string;
  firstDiscovererName: string;
  firstMinute: number;
  latestMinute: number;
  livingKnowerCount: number;
}

function groupTechnologyDiscoveries(
  discoveries: CivilizationDiscovery[],
): TechnologySummary[] {
  const grouped = new Map<string, TechnologySummary>();

  for (const discovery of discoveries) {
    const technique = discovery.technique || discovery.factId;
    const minute = Number(discovery.minute) || 0;
    const knowerCount = Math.max(0, Number(discovery.livingKnowerCount) || 0);
    const existing = grouped.get(technique);

    if (!existing) {
      grouped.set(technique, {
        technique,
        firstFactId: discovery.factId,
        firstDiscovererId: discovery.discovererId,
        firstDiscovererName: discovery.discovererName,
        firstMinute: minute,
        latestMinute: minute,
        livingKnowerCount: knowerCount,
      });
      continue;
    }

    if (minute < existing.firstMinute) {
      existing.firstFactId = discovery.factId;
      existing.firstDiscovererId = discovery.discovererId;
      existing.firstDiscovererName = discovery.discovererName;
      existing.firstMinute = minute;
    }

    existing.latestMinute = Math.max(existing.latestMinute, minute);
    existing.livingKnowerCount = Math.max(
      existing.livingKnowerCount,
      knowerCount,
    );
  }

  return [...grouped.values()]
    .sort((a, b) => b.firstMinute - a.firstMinute);
}

function facilitySort(a: CivilizationWorldFacility, b: CivilizationWorldFacility): number {
  const priority = (facility: CivilizationWorldFacility): number => {
    if (facility.state === 'UnderConstruction') return 3;
    if (facility.state === 'Planned') return 2;
    if (facility.state === 'Operational') return 1;
    return 0;
  };
  const byPriority = priority(b) - priority(a);
  if (byPriority !== 0) return byPriority;
  return (Number(b.startedMinute) || 0) - (Number(a.startedMinute) || 0);
}

export function WorldActivityPanel({
  civilization,
  worldObjects,
  residents,
  onFocusGrid,
}: {
  civilization: CivilizationWorldPayload;
  worldObjects: WorldObjectsPayload;
  residents: Resident[];
  onFocusGrid: (gridX: number, gridY: number) => void;
}) {
  const names = new Map(residents.map(resident => [resident.id, resident.name]));
  const facilities = [...(civilization.facilities ?? [])]
    .sort(facilitySort)
    .slice(0, 4);
  const technologySummaries = groupTechnologyDiscoveries(
    civilization.recentDiscoveries ?? [],
  ).slice(0, 6);
  const livingResidentCount = residents.filter(
    resident => resident.alive !== false,
  ).length;
  const pressuredResources = [...(civilization.resources ?? [])]
    .filter(resource => (
      Number(resource.maxQuantity) > 0
      && Number(resource.quantity) / Number(resource.maxQuantity) <= 0.35
    ))
    .sort((a, b) => (
      (Number(a.quantity) / Math.max(1, Number(a.maxQuantity)))
      - (Number(b.quantity) / Math.max(1, Number(b.maxQuantity)))
    ))
    .slice(0, 3);
  const sanitationSites = worldObjects.sanitationSites ?? [];
  const activeSanitation = sanitationSites.filter(site => site.active);
  const smartObjects = worldObjects.smartObjects ?? [];
  const reservedObjects = smartObjects.filter(object => (
    object.reservedById
    && object.reservedById !== '0'
  )).length;
  const society = civilization.society;
  const activeInstitutions = (society?.institutions ?? [])
    .filter(institution => institution.active)
    .sort((a, b) => Number(b.strength01) - Number(a.strength01));
  const pressuredDemands = (society?.demands ?? [])
    .filter(demand => Number(demand.demand01) >= 0.25)
    .sort((a, b) => Number(b.demand01) - Number(a.demand01))
    .slice(0, 4);
  const roleCounts = new Map<string, number>();
  for (const resident of society?.residents ?? []) {
    roleCounts.set(resident.role, (roleCounts.get(resident.role) ?? 0) + 1);
  }
  const roleSummary = [...roleCounts.entries()]
    .filter(([role]) => role !== 'Generalist')
    .sort((a, b) => b[1] - a[1])
    .slice(0, 4);

  if (
    civilization.available !== true
    && worldObjects.available !== true
  ) {
    return (
      <div className="focused-life-muted">
        월드 활동 정보를 불러오는 중입니다.
      </div>
    );
  }

  return (
    <div className="observation-feed-panel">
      <div className="focused-life-chips">
        <span>자원 노드 <b>{civilization.resourceNodeCount ?? 0}</b></span>
        <span>저장량 <b>{civilization.totalStoredUnits ?? 0}</b></span>
        <span>시설 <b>{civilization.facilityCount ?? 0}</b></span>
        <span>재현 가능 기술 <b>{civilization.uniqueReproducibleTechniqueTypes ?? 0}</b></span>
      </div>

      {society ? (
        <div className="focused-life-section">
          <h3>사회·경제</h3>
          <div className="focused-life-chips">
            <span>전문화 <b>{society.specializedResidentCount}/{society.livingResidentCount}명</b></span>
            <span>최근 교육 전수 <b>{society.recentTeachingReceipts}</b></span>
            <span>교환 기록 <b>{society.exchangeFactCount}</b></span>
            <span>활성 조직 <b>{society.activeInstitutionCount}</b></span>
            <span>
              전승 단계 <b>{KOREAN_COLLECTIVE_RECORD_STAGE_LABELS[society.recordStage] ?? society.recordStage}</b>
            </span>
          </div>
          {roleSummary.length > 0 ? (
            <div className="focused-life-inline">
              {roleSummary.map(([role, count]) => (
                <span key={role}>
                  {KOREAN_SOCIETY_ROLE_LABELS[role] ?? role} <b>{count}명</b>
                </span>
              ))}
            </div>
          ) : null}
          {activeInstitutions.length > 0 ? (
            <div className="focused-life-chips">
              {activeInstitutions.map(institution => (
                <span key={institution.kind}>
                  {KOREAN_SOCIETY_INSTITUTION_LABELS[institution.kind] ?? institution.kind}
                  {' '}
                  <b>{formatPercent(institution.strength01)}</b>
                </span>
              ))}
            </div>
          ) : null}
          {pressuredDemands.length > 0 ? (
            <div className="focused-life-inline">
              {pressuredDemands.map(demand => (
                <span key={demand.material}>
                  {formatMaterial(demand.material)} 부족
                  {' '}
                  <b>{demand.deficitUnits}</b>
                  {' · '}
                  수요 {formatPercent(demand.demand01)}
                </span>
              ))}
            </div>
          ) : null}
        </div>
      ) : null}

      {facilities.length > 0 ? (
        <div className="focused-life-section">
          <h3>시설 현황</h3>
          {facilities.map(facility => {
            const workerId = facility.lastWorkedBy || facility.initiatedBy;
            const workerName = names.get(workerId) || '';
            return (
              <button
                type="button"
                className="observation-event observation-event-button"
                key={facility.id}
                onClick={() => onFocusGrid(facility.gridX, facility.gridY)}
              >
                <strong>
                  {formatFacilityKind(facility.kind)} · {formatFacilityState(facility.state)}
                </strong>
                <span className="observation-event-detail">
                  진행 {formatPercent(facility.workProgress)}
                  {facility.requiredMaterialUnits > 0
                    ? ` · 재료 ${facility.deliveredMaterialUnits}/${facility.requiredMaterialUnits}`
                    : ''}
                  {workerName ? ` · 최근 작업 ${workerName}` : ''}
                  {' · 위치 보기'}
                </span>
              </button>
            );
          })}
        </div>
      ) : null}

      {pressuredResources.length > 0 ? (
        <div className="focused-life-section">
          <h3>자원 압박</h3>
          {pressuredResources.map(resource => (
            <button
              type="button"
              className="observation-event observation-event-button"
              key={resource.id}
              onClick={() => onFocusGrid(resource.gridX, resource.gridY)}
            >
              <strong>{formatMaterial(resource.material)} · 잔량 {resource.quantity}/{resource.maxQuantity}</strong>
              <span className="observation-event-detail">
                {resource.renewable ? '재생 가능' : '비재생'} · 위치 보기
              </span>
            </button>
          ))}
        </div>
      ) : null}

      {technologySummaries.length > 0 ? (
        <div className="focused-life-section">
          <h3>기술 발전</h3>
          {technologySummaries.map(technology => {
            const discovererName = technology.firstDiscovererName
              || names.get(technology.firstDiscovererId)
              || '알 수 없음';
            const knownCount = Math.min(
              Math.max(0, technology.livingKnowerCount),
              Math.max(livingResidentCount, technology.livingKnowerCount),
            );
            const diffusionState = livingResidentCount > 0
              && knownCount >= livingResidentCount
              ? '전체 전파 완료'
              : knownCount > 1
                ? '전파 중'
                : '개인 지식';

            return (
              <div
                className="observation-event"
                key={technology.technique}
              >
                <strong>{formatTechnique(technology.technique)}</strong>
                <span className="observation-event-detail">
                  최초 발견 {discovererName}
                  {' · '}
                  현재 보유 {knownCount}
                  {livingResidentCount > 0 ? `/${livingResidentCount}명` : '명'}
                  {' · '}
                  {diffusionState}
                </span>
              </div>
            );
          })}
        </div>
      ) : null}

      <div className="focused-life-section">
        <h3>생활 인프라</h3>
        <div className="focused-life-inline">
          <span>
            위생 장소 <b>{activeSanitation.length}/{sanitationSites.length}</b>
          </span>
          <span>
            위생 이용 <b>{sanitationSites.reduce((sum, site) => sum + (Number(site.useCount) || 0), 0)}</b>
          </span>
          <span>
            생활 오브젝트 사용 중 <b>{reservedObjects}/{smartObjects.length}</b>
          </span>
        </div>
        {activeSanitation.slice(0, 3).map(site => (
          <button
            type="button"
            className="observation-event observation-event-button"
            key={site.id}
            onClick={() => onFocusGrid(site.gridX, site.gridY)}
          >
            <strong>
              {formatSanitationSiteKind(site.kind)}
            </strong>
            <span className="observation-event-detail">
              이용 {site.useCount}회 · 위치 보기
            </span>
          </button>
        ))}
      </div>

      {(civilization.depletedResourceNodeCount ?? 0) > 0 ? (
        <div className="life-event-chip">
          고갈된 자원 노드 {civilization.depletedResourceNodeCount}곳
        </div>
      ) : null}
    </div>
  );
}
