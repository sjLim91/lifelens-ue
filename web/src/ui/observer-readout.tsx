import type { Resident } from '../runtime/core-types';
import type { ObserverSnapshot } from '../state/observer-store';
import {
  formatActivity,
  formatBiome,
  formatDay,
  formatBeliefText,
  formatLifeEventType,
  formatLifeStage,
  formatMemoryLocation,
  formatMemoryText,
  formatPartnerStage,
  formatPercent,
  formatSex,
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

  return (
    <>
      <div className="metrics">
        <div><span>생존 인구</span><b id="living">{snapshot.world.livingResidents ?? '—'}</b></div>
        <div><span>가구</span><b id="households">{snapshot.world.households ?? '—'}</b></div>
        <div><span>커플</span><b id="couples">{snapshot.world.activeCouples ?? '—'}</b></div>
        <div><span>주요 사건</span><b id="events">{snapshot.world.majorLifeEvents ?? '—'}</b></div>
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
        <span>{formatSex(resident.sex)} · {formatActivity(resident.activityLabel)}</span>
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
  const activityTarget = resident.activityTargetName
    ? ` → ${resident.activityTargetName}`
    : '';

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
        현재 <b>{formatActivity(resident.activityLabel)}</b>{activityTarget}
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
