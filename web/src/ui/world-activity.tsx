import type {
  CivilizationWorldFacility,
  CivilizationWorldPayload,
  Resident,
  WorldObjectsPayload,
} from '../runtime/core-types';

function formatPercent(value: unknown): string {
  const normalized = Math.max(0, Math.min(1, Number(value) || 0));
  return `${Math.round(normalized * 100)}%`;
}

function facilityLabel(kind: string): string {
  switch (kind) {
    case 'PrimitiveStorage': return '원시 저장소';
    case 'FirePit': return '화덕';
    case 'WorkSurface': return '작업대';
    case 'SleepingPlace': return '수면 장소';
    case 'Shelter': return '쉼터';
    case 'Furnace': return '용광로';
    default: return kind || '시설';
  }
}

function facilityStateLabel(state: string): string {
  switch (state) {
    case 'Planned': return '계획';
    case 'UnderConstruction': return '건설 중';
    case 'Operational': return '가동';
    case 'Ruined': return '파손';
    default: return state || '상태 확인 중';
  }
}

function techniqueLabel(technique: string): string {
  switch (technique) {
    case 'SharpFlake': return '날카로운 석편';
    case 'ChippedStoneTool': return '뗀석기';
    case 'FireMaking': return '불 피우기';
    case 'FiberCordage': return '섬유 끈';
    case 'SimpleContainer': return '단순 용기';
    case 'DesignatedSanitationArea': return '위생 구역 지정';
    case 'DugSanitationPit': return '위생 구덩이';
    case 'PrimitiveStorage': return '원시 저장';
    case 'DiggingStick': return '굴착 막대';
    case 'StoneHammer': return '돌망치';
    case 'CopperSmelting': return '구리 제련';
    default: return technique || '새 지식';
  }
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
  onSelectResident,
  onFocusGrid,
}: {
  civilization: CivilizationWorldPayload;
  worldObjects: WorldObjectsPayload;
  residents: Resident[];
  onSelectResident: (residentId: string) => void;
  onFocusGrid: (gridX: number, gridY: number) => void;
}) {
  const names = new Map(residents.map(resident => [resident.id, resident.name]));
  const facilities = [...(civilization.facilities ?? [])]
    .sort(facilitySort)
    .slice(0, 4);
  const discoveries = [...(civilization.recentDiscoveries ?? [])]
    .sort((a, b) => (Number(b.minute) || 0) - (Number(a.minute) || 0))
    .slice(0, 4);
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
                  {facilityLabel(facility.kind)} · {facilityStateLabel(facility.state)}
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
              <strong>{resource.material || '자원'} · 잔량 {resource.quantity}/{resource.maxQuantity}</strong>
              <span className="observation-event-detail">
                {resource.renewable ? '재생 가능' : '비재생'} · 위치 보기
              </span>
            </button>
          ))}
        </div>
      ) : null}

      {discoveries.length > 0 ? (
        <div className="focused-life-section">
          <h3>최근 발견</h3>
          {discoveries.map(discovery => {
            const canSelect = residents.some(resident => resident.id === discovery.discovererId);
            const content = (
              <>
                <strong>
                  {discovery.discovererName || names.get(discovery.discovererId) || '누군가'}
                  {' · '}
                  {techniqueLabel(discovery.technique)}
                </strong>
                <span className="observation-event-detail">
                  {discovery.livingKnowerCount > 1
                    ? `현재 ${discovery.livingKnowerCount}명이 알고 있음`
                    : '아직 개인 지식에 가까움'}
                </span>
              </>
            );
            return canSelect ? (
              <button
                type="button"
                className="observation-event observation-event-button"
                key={discovery.factId}
                onClick={() => onSelectResident(discovery.discovererId)}
              >
                {content}
              </button>
            ) : (
              <div className="observation-event" key={discovery.factId}>
                {content}
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
              {site.kind === 'DugPit' ? '위생 구덩이' : '지정 위생 구역'}
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
