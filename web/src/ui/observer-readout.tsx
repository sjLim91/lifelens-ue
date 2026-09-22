import type { Resident } from '../runtime/core-types';
import type { ObserverSnapshot } from '../state/observer-store';
import {
  formatBiome,
  formatDay,
  formatBeliefText,
  formatFacilityKind,
  formatFacilityState,
  formatItemKind,
  formatKnowledgeLevel,
  formatKnowledgeSource,
  formatLifeEventType,
  formatLifeStage,
  formatMaterialName,
  formatMemoryLocation,
  formatMemorySource,
  formatMemoryText,
  formatPartnerStage,
  formatPercent,
  formatResidentCurrentAction,
  formatSocialEventType,
  formatSex,
  formatTechniqueName,
  formatWeather,
} from './observer-format';

export function RuntimeBadge({
  runtime,
}: {
  runtime: ObserverSnapshot['runtime'];
}) {
  const text = runtime.status === 'ready'
    ? 'Core 연결됨'
    : runtime.status === 'error'
      ? 'Core 연결 실패'
      : 'Core 불러오는 중…';

  const className = runtime.status === 'loading' ? 'pending' : runtime.status;
  return <div id="status" className={`status ${className}`}>{text}</div>;
}

export function WorldOverlay({
  snapshot,
}: {
  snapshot: ObserverSnapshot;
}) {
  const center = snapshot.terrain?.chunks.find(
    (chunk) => chunk.x === snapshot.camera.centerChunkX
      && chunk.y === snapshot.camera.centerChunkY,
  );
  const forest = Math.round(
    Math.max(0, Math.min(1, Number(center?.forestCoverage01) || 0)) * 100,
  );

  return (
    <div className="world-overlay">
      <span id="timeLabel" className="world-overlay-primary">
        {formatDay(snapshot.world.minute)}
      </span>
      <span id="biomeLabel">
        {center
          ? `${formatBiome(center.biome)} · 숲 ${forest}%`
          : '환경 분석 중'}
      </span>
      <span id="weatherLabel">
        {snapshot.environment?.available
          ? `${formatWeather(snapshot.environment.summary)} · ${Math.round(Number(snapshot.environment.airTemperatureC) || 0)}°C`
          : '날씨 분석 중'}
      </span>
      <span id="livingOverlay">
        {snapshot.world.livingResidents !== undefined
          ? `인구 ${snapshot.world.livingResidents}`
          : '인구 —'}
      </span>
    </div>
  );
}

export function ObserverMetrics({
  snapshot,
}: {
  snapshot: ObserverSnapshot;
}) {
  const majorEvents = snapshot.world.majorLifeEventItems ?? [];
  const totalMajorEvents = snapshot.world.majorLifeEvents ?? 0;
  const socialEvents = [...(snapshot.presentation?.socialEvents ?? [])]
    .reverse()
    .slice(0, 6);
  const facilities = snapshot.presentation?.facilities ?? [];
  const resources = snapshot.presentation?.resources ?? [];
  const storages = snapshot.presentation?.storages ?? [];
  const discoveries = [...(snapshot.presentation?.discoveries ?? [])]
    .reverse()
    .slice(0, 5);
  const visibleFacilities = facilities
    .filter((facility) => facility.state !== 'Ruined')
    .slice(0, 6);
  const sanitationSites = snapshot.presentation?.sanitationSites ?? [];
  const residues = snapshot.presentation?.residues ?? [];
  const activeProjects = facilities.filter(
    (facility) => facility.state === 'Planned'
      || facility.state === 'UnderConstruction',
  ).length;

  return (
    <>
      <div className="metrics">
        <div><span>생존 인구</span><b id="living">{snapshot.world.livingResidents ?? '—'}</b></div>
        <div><span>가구</span><b id="households">{snapshot.world.households ?? '—'}</b></div>
        <div><span>커플</span><b id="couples">{snapshot.world.activeCouples ?? '—'}</b></div>
        <div><span>주요 사건</span><b id="events">{snapshot.world.majorLifeEvents ?? '—'}</b></div>
      </div>

      <div className="world-consequence-summary">
        <div>
          <span>자원 노드</span>
          <b>{resources.length}</b>
          <small>Core 실제 채집 대상</small>
        </div>
        <div>
          <span>생활 시설</span>
          <b>{facilities.length}</b>
          <small>{activeProjects > 0 ? `건설 중 ${activeProjects}` : `저장소 ${storages.length}`}</small>
        </div>
        <div>
          <span>위생시설</span>
          <b>{sanitationSites.length}</b>
          <small>{sanitationSites.some((site) => site.kind === 'DugPit') ? '구덩이 시설 사용 중' : '초기 위생 단계'}</small>
        </div>
        <div>
          <span>오염 지점</span>
          <b>{residues.length}</b>
          <small>최대 강도 {formatPercent(snapshot.presentation?.peakWasteIntensity)}</small>
        </div>
      </div>

      {visibleFacilities.length > 0 ? (
        <div className="world-event-list">
          <div className="world-event-heading">
            <h3>생활 기반</h3>
            <span>Core 실제 시설</span>
          </div>
          {visibleFacilities.map((facility) => (
            <div className="world-event-row facility-row" key={facility.id}>
              <div>
                <strong>{formatFacilityKind(facility.kind)}</strong>
                <span>{formatFacilityState(facility.state)}</span>
              </div>
              <small>
                좌표 {facility.gridX}, {facility.gridY}
                {facility.state === 'UnderConstruction'
                  ? ` · 공정 ${Math.round((Number(facility.workProgress) || 0) * 100)}%`
                  : ''}
                {facility.lit ? ' · 불 사용 중' : ''}
              </small>
            </div>
          ))}
        </div>
      ) : null}

      {discoveries.length > 0 ? (
        <div className="world-event-list discovery-event-list">
          <div className="world-event-heading">
            <h3>기술 발견과 전파</h3>
            <span>Core 지식 계보</span>
          </div>
          {discoveries.map((discovery) => (
            <div className="world-event-row discovery-event-row" key={discovery.factId}>
              <div>
                <strong>{discovery.discovererName || '주민'}</strong>
                <span>{formatTechniqueName(discovery.technique)} 발견</span>
              </div>
              <small>
                {formatDay(discovery.minute)}
                {Number(discovery.livingKnowerCount) > 1
                  ? ` · 현재 아는 주민 ${discovery.livingKnowerCount}명`
                  : ''}
                {Number(discovery.recipientCount) > 0
                  ? ` · 전달 ${discovery.recipientCount}명`
                  : ''}
              </small>
            </div>
          ))}
        </div>
      ) : null}

      <div className="world-event-list social-event-list">
        <div className="world-event-heading">
          <h3>사람들 사이</h3>
          <span>{socialEvents.length > 0 ? '최근 상호작용' : '아직 기록 없음'}</span>
        </div>
        {socialEvents.length > 0
          ? socialEvents.map((event) => (
              <div
                className={`world-event-row social-event-row ${String(event.presentationLevel ?? '').toLowerCase()}`}
                key={event.sequence}
              >
                <div>
                  <strong>{event.actorName || '주민'} → {event.targetName || '주민'}</strong>
                  <span>{formatSocialEventType(event.type)}</span>
                </div>
                <small>
                  {formatDay(event.minute)}
                  {Number(event.intensity) > 0
                    ? ` · 강도 ${formatPercent(event.intensity)}`
                    : ''}
                </small>
              </div>
            ))
          : <div className="focused-life-muted">아직 관찰된 사회적 상호작용이 없습니다.</div>}
      </div>

      <div className="world-event-list">
        <div className="world-event-heading">
          <h3>최근 주요 사건</h3>
          <span>
            {totalMajorEvents > 0
              ? `최근 ${majorEvents.length}건 · 누적 ${totalMajorEvents}건`
              : '아직 주요 사건 없음'}
          </span>
        </div>
        {majorEvents.length > 0
          ? majorEvents.map((event, index) => {
              const relatedNames = (event.related ?? [])
                .map((related) => related.name)
                .filter((name): name is string => Boolean(name));
              return (
                <div
                  className="world-event-row"
                  key={`${event.minute}:${event.residentId}:${event.type}:${index}`}
                >
                  <div>
                    <strong>{event.residentName || '주민'}</strong>
                    <span>{formatLifeEventType(event.type)}</span>
                  </div>
                  <small>
                    {formatDay(event.minute)}
                    {relatedNames.length > 0
                      ? ` · 관련: ${relatedNames.join(', ')}`
                      : ''}
                  </small>
                </div>
              );
            })
          : <div className="focused-life-muted">아직 기록된 주요 사건이 없습니다.</div>}
      </div>
    </>
  );
}

function ResidentNeedsGrid({ resident }: { resident: Resident }) {
  return (
    <div className="need-grid">
      <span>배고픔 <b>{formatPercent(resident.needs?.hunger)}</b></span>
      <span>갈증 <b>{formatPercent(resident.needs?.thirst)}</b></span>
      <span>수면 <b>{formatPercent(resident.needs?.sleep)}</b></span>
      <span>방광 <b>{formatPercent(resident.needs?.bladder)}</b></span>
      <span>위생 <b>{formatPercent(resident.needs?.hygiene)}</b></span>
    </div>
  );
}

function ResidentCard({
  resident,
  selected,
  onSelect,
}: {
  resident: Resident;
  selected: boolean;
  onSelect: (residentId: string) => void;
}) {
  return (
    <button
      type="button"
      className={`resident-card resident-card-button ${selected ? 'selected' : ''}`}
      onClick={() => onSelect(resident.id)}
      aria-pressed={selected}
    >
      <div className="resident-title">
        <strong>{resident.name}</strong>
        <span>{formatSex(resident.sex)} · {formatResidentCurrentAction(resident)}</span>
      </div>
      <ResidentNeedsGrid resident={resident} />
      <small>
        {resident.hasPosition
          ? `좌표 ${resident.gridX}, ${resident.gridY}`
          : '위치 확인 불가'}
      </small>
    </button>
  );
}

export function SelectedResidentReadout({
  resident,
  onClear,
}: {
  resident: Resident | null;
  onClear: () => void;
}) {
  if (!resident) {
    return (
      <div className="focused-life-empty">
        월드의 사람을 터치하면 그 사람의 현재 삶을 자세히 관찰할 수 있습니다.
      </div>
    );
  }

  const relationships = resident.relationships ?? [];
  const importantRelationships = relationships.slice(0, 4);
  const memories = (resident.memories ?? []).slice(0, 3);
  const beliefs = (resident.beliefs ?? []).slice(0, 3);
  const traitLabels: Record<string, string> = {
    resilience: '회복력',
    creativity: '창의성',
    discipline: '규율',
    compassion: '공감/연민',
    adaptability: '적응력',
    boldness: '대담성',
    perseverance: '끈기',
    resourcefulness: '생활력',
  };
  const topTraits = Object.entries(resident.traits ?? {})
    .filter((entry): entry is [string, number] => typeof entry[1] === 'number')
    .sort((a, b) => b[1] - a[1])
    .slice(0, 4);
  const emotionLabels: Record<string, string> = {
    joy: '기쁨',
    sadness: '슬픔',
    anger: '분노',
    fear: '두려움',
    embarrassment: '당혹',
    pride: '자부심',
    jealousy: '질투',
    affection: '애정',
    anxiety: '불안',
    relief: '안도',
    grief: '비탄',
  };
  const strongestEmotion = Object.entries(resident.emotion ?? {})
    .filter(
      (entry): entry is [string, number] => (
        entry[0] in emotionLabels
        && typeof entry[1] === 'number'
      ),
    )
    .sort((a, b) => b[1] - a[1])[0];

  const family = resident.family;
  const partner = family?.hasActivePartner && family.partnerName
    ? `${family.partnerName} · ${formatPartnerStage(family.partnerStage)}`
    : '현재 파트너 없음';
  return (
    <article className="focused-life">
      <div className="focused-life-heading">
        <div>
          <span>집중 관찰</span>
          <strong>{resident.name}</strong>
        </div>
        <button type="button" onClick={onClear} aria-label="선택 해제">×</button>
      </div>

      <div className="focused-life-identity">
        <span>{formatSex(resident.sex)}</span>
        <span>{formatLifeStage(resident.lifeStage)}</span>
        <span>{resident.ageYears !== undefined ? `${resident.ageYears}세` : '나이 —'}</span>
      </div>

      <p className="focused-life-activity">
        현재 <b>{formatResidentCurrentAction(resident)}</b>
      </p>

      <ResidentNeedsGrid resident={resident} />

      <div className="focused-life-section">
        <h3>감정</h3>
        <div className="focused-life-inline">
          <span>
            중심 감정 <b>{strongestEmotion ? emotionLabels[strongestEmotion[0]] : '평온'}</b>
          </span>
          <span>
            강도 <b>{formatPercent(resident.emotion?.intensity)}</b>
          </span>
        </div>
      </div>

      <div className="focused-life-section">
        <h3>성향</h3>
        <div className="focused-life-chips">
          {topTraits.length > 0
            ? topTraits.map(([key, value]) => (
                <span key={key}>
                  {traitLabels[key] ?? key} <b>{formatPercent(value)}</b>
                </span>
              ))
            : <span>성향 데이터 확인 중</span>}
        </div>
      </div>

      <div className="focused-life-section">
        <h3>생활 기술과 소지품</h3>
        <div className="focused-life-inline">
          <span>채집 <b>{formatPercent(resident.civilization?.gatheringSkill)}</b></span>
          <span>제작 <b>{formatPercent(resident.civilization?.craftingSkill)}</b></span>
          <span>학습 <b>{formatPercent(resident.civilization?.learningSkill)}</b></span>
          <span>소지품 <b>{resident.civilization?.totalInventoryUnits ?? 0}</b></span>
        </div>
        {(resident.civilization?.inventory ?? []).length > 0 ? (
          <div className="civilization-item-grid">
            {(resident.civilization?.inventory ?? []).slice(0, 6).map((item, index) => (
              <span key={`${item.item}:${item.material}:${index}`}>
                {item.item === 'RawMaterial'
                  ? formatMaterialName(item.material)
                  : formatItemKind(item.item)}
                <b> ×{item.quantity ?? 0}</b>
              </span>
            ))}
          </div>
        ) : <div className="focused-life-muted">아직 들고 있는 물건이 없습니다.</div>}
        {(resident.civilization?.techniques ?? []).length > 0 ? (
          <div className="technique-list">
            {(resident.civilization?.techniques ?? []).slice(0, 8).map((technique, index) => (
              <div className="technique-row" key={`${technique.technique}:${index}`}>
                <div>
                  <strong>{formatTechniqueName(technique.technique)}</strong>
                  <span>{formatKnowledgeLevel(technique.level)}</span>
                </div>
                <small>
                  확신 {formatPercent(technique.confidence)}
                  {technique.hasProvenance
                    ? ` · ${formatKnowledgeSource(technique.source)}`
                    : ''}
                  {Number(technique.successfulUses) > 0
                    ? ` · 사용 ${technique.successfulUses}회`
                    : ''}
                </small>
              </div>
            ))}
          </div>
        ) : <div className="focused-life-muted">아직 습득한 생활 기술이 없습니다.</div>}
      </div>

      <div className="focused-life-section">
        <h3>관계</h3>
        {importantRelationships.length > 0
          ? importantRelationships.map((relationship) => (
              <div className="relationship-row" key={relationship.targetId}>
                <strong>{relationship.targetName || relationship.targetId}</strong>
                <span>유대 {formatPercent(relationship.socialBond)}</span>
                <span>신뢰 {formatPercent(relationship.trust)}</span>
                {Number(relationship.conflict) > 0.05
                  ? <span>갈등 {formatPercent(relationship.conflict)}</span>
                  : null}
                {Number(relationship.romancePotential) > 0.2
                  ? <span>연애 {formatPercent(relationship.romancePotential)}</span>
                  : null}
              </div>
            ))
          : <div className="focused-life-muted">아직 뚜렷한 관계가 없습니다.</div>}
      </div>

      <div className="focused-life-section">
        <h3>가족</h3>
        <div className="focused-life-inline">
          <span>파트너 <b>{partner}</b></span>
          <span>자녀 <b>{family?.children?.length ?? 0}</b></span>
          {family?.expectingChild ? <span className="life-event-chip">임신 진행 중</span> : null}
        </div>
      </div>

      <div className="focused-life-section">
        <h3>기억</h3>
        {memories.length > 0
          ? memories.map((memory, index) => (
              <div className="memory-row" key={`${memory.minute ?? 0}:${index}`}>
                <span>{formatMemoryText(memory.what)}</span>
                <small>
                  신뢰도 {formatPercent(memory.effectiveConfidence ?? memory.confidence)}
                  {memory.source ? ` · ${formatMemorySource(memory.source)}` : ''}
                  {memory.where ? ` · ${formatMemoryLocation(memory.where)}` : ''}
                </small>
              </div>
            ))
          : <div className="focused-life-muted">아직 강하게 남은 기억이 없습니다.</div>}
      </div>

      {beliefs.length > 0 ? (
        <div className="focused-life-section">
          <h3>믿음</h3>
          {beliefs.map((belief, index) => (
            <div className="belief-row" key={`${belief.subject ?? '0'}:${index}`}>
              <span>{formatBeliefText(belief.proposition, belief.stance)}</span>
              <small>확신 {formatPercent(belief.confidence)}</small>
            </div>
          ))}
        </div>
      ) : null}

      <small>
        {resident.hasPosition
          ? `현재 좌표 ${resident.gridX}, ${resident.gridY}`
          : '현재 위치를 확인하는 중'}
      </small>
    </article>
  );
}

export function ResidentReadout({
  residents,
  selectedResidentId,
  onSelect,
}: {
  residents: Resident[];
  selectedResidentId: string | null;
  onSelect: (residentId: string) => void;
}) {
  return (
    <>
      <div className="panel-heading">
        <h2>주민</h2>
        <span id="residentCount">{residents.length}</span>
      </div>
      <div id="residentList" className="resident-list">
        {residents.length > 0
          ? residents.map((resident) => (
              <ResidentCard
                key={resident.id}
                resident={resident}
                selected={resident.id === selectedResidentId}
                onSelect={onSelect}
              />
            ))
          : <div className="empty">표시할 주민이 없습니다.</div>}
      </div>
    </>
  );
}
