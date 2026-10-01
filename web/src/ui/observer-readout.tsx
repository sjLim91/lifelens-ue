import type { Resident } from '../runtime/core-types';
import type { ObserverSnapshot } from '../state/observer-store';
import {
  formatBelief,
  formatBiome,
  formatDay,
  formatDevelopment,
  formatGenetics,
  formatKinship,
  formatLifeCondition,
  formatLifeStage,
  formatLocationText,
  formatMemoryTag,
  formatMemoryText,
  formatPartnerStage,
  formatPercent,
  formatPregnancyStage,
  formatSex,
  formatTrait,
  formatWeather,
} from './observer-format';
import { residentStatusText } from './resident-status';
import { ResidentNeeds } from './resident-needs';

export function RuntimeBadge({
  runtime,
}: {
  runtime: ObserverSnapshot['runtime'];
}) {
  const text = runtime.status === 'ready'
    ? '시뮬레이션 코어 연결됨'
    : runtime.status === 'error'
      ? '시뮬레이션 코어 연결 실패'
      : '시뮬레이션 코어 불러오는 중…';

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
  return (
    <div className="metrics">
      <div><span>생존 인구</span><b id="living">{snapshot.world.livingResidents ?? '—'}</b></div>
      <div><span>가구</span><b id="households">{snapshot.world.households ?? '—'}</b></div>
      <div><span>커플</span><b id="couples">{snapshot.world.activeCouples ?? '—'}</b></div>
      <div><span>주요 사건</span><b id="events">{snapshot.world.majorLifeEvents ?? '—'}</b></div>
    </div>
  );
}

function ResidentCard({
  resident,
  residents,
  selected,
  onSelect,
}: {
  resident: Resident;
  residents: Resident[];
  selected: boolean;
  onSelect: (residentId: string) => void;
}) {
  const statusText = residentStatusText(resident, residents);
  return (
    <button
      type="button"
      className={`resident-card resident-card-button ${selected ? 'selected' : ''}`}
      onClick={() => onSelect(resident.id)}
      aria-pressed={selected}
    >
      <div className="resident-title">
        <strong>{resident.name}</strong>
        <span>{formatSex(resident.sex)} · {statusText}</span>
      </div>
      <ResidentNeeds needs={resident.needs} />
      <small className="resident-card-footer">
        <span>{resident.hasPosition
          ? `좌표 ${resident.gridX}, ${resident.gridY}`
          : '위치 확인 불가'}</span>
        <span>자세히 보기 →</span>
      </small>
    </button>
  );
}

export function SelectedResidentReadout({
  resident,
  residents,
  onClear,
}: {
  resident: Resident | null;
  residents: Resident[];
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

  const conditionEntries = Object.entries(resident.lifeCondition ?? {})
    .filter((entry): entry is [string, number] => typeof entry[1] === 'number');

  const geneticsEntries = Object.entries(resident.genetics ?? {})
    .filter((entry): entry is [string, number] => typeof entry[1] === 'number');

  const developmentEntries = Object.entries(resident.development ?? {})
    .filter((entry): entry is [string, number] => typeof entry[1] === 'number');
  const developmentVisible = [
    'Baby',
    'Toddler',
    'Child',
    'Teen',
  ].includes(resident.lifeStage ?? '');


  const family = resident.family;
  const household = resident.household;
  const pregnancy = resident.pregnancy;
  const ownHouseholdMember = household?.members?.find(
    (member) => member.id === resident.id,
  );
  const partner = family?.hasActivePartner && family.partnerName
    ? `${family.partnerName} · ${formatPartnerStage(family.partnerStage)}`
    : '현재 파트너 없음';
  const closeFamily = [
    ...(family?.parents ?? []),
    ...(family?.children ?? []),
    ...(family?.siblings ?? []),
  ].slice(0, 6);
  const statusText = residentStatusText(resident, residents);
  const activityTarget = !resident.presentation?.active && resident.activityTargetName
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
        현재 <b>{statusText}</b>{activityTarget}
      </p>

      <ResidentNeeds needs={resident.needs} />

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
                  {formatTrait(key)} <b>{formatPercent(value)}</b>
                </span>
              ))
            : <span>성향 데이터 확인 중</span>}
        </div>
      </div>

      {conditionEntries.length > 0 ? (
        <div className="focused-life-section">
          <h3>건강 / 신체</h3>
          <div className="focused-life-chips">
            {conditionEntries.map(([key, value]) => (
              <span key={key}>
                {formatLifeCondition(key)} <b>{formatPercent(value)}</b>
              </span>
            ))}
          </div>
        </div>
      ) : null}

      {geneticsEntries.length > 0 ? (
        <div className="focused-life-section">
          <h3>유전</h3>
          <div className="focused-life-chips">
            {geneticsEntries.map(([key, value]) => (
              <span key={key}>
                {formatGenetics(key)} <b>{formatPercent(value)}</b>
              </span>
            ))}
          </div>
        </div>
      ) : null}

      {developmentVisible && developmentEntries.length > 0 ? (
        <div className="focused-life-section">
          <h3>발달</h3>
          <div className="focused-life-chips">
            {developmentEntries.map(([key, value]) => (
              <span key={key}>
                {formatDevelopment(key)} <b>{formatPercent(value)}</b>
              </span>
            ))}
          </div>
        </div>
      ) : null}

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
          {family?.cohabitingWithPartner
            ? <span className="life-event-chip">동거 중</span>
            : null}
          {family?.expectingChild
            ? (
                <span className="life-event-chip">
                  {family.isGestationalParent ? '본인 임신 진행 중' : '파트너 임신 진행 중'}
                </span>
              )
            : null}
        </div>
        {closeFamily.length > 0 ? (
          <div className="focused-life-chips">
            {closeFamily.map((member) => (
              <span key={member.id}>
                {member.name || member.id}
                {' · '}
                <b>{formatKinship(member.kinship)}</b>
              </span>
            ))}
          </div>
        ) : null}
        {household ? (
          <div className="focused-life-chips">
            <span>가구 <b>#{household.id}</b></span>
            <span>구성원 <b>{household.members?.length ?? 0}</b></span>
            <span>공동 자원 <b>{Math.round(Number(household.resources) || 0)}</b></span>
            <span>공동 자금 <b>{Math.round(Number(household.sharedMoney) || 0)}</b></span>
            {ownHouseholdMember
              ? (
                  <span>
                    가구 기여 <b>{formatPercent(ownHouseholdMember.contributionWeight)}</b>
                  </span>
                )
              : null}
            {ownHouseholdMember?.responsibilities?.caregiving
              ? (
                  <span>
                    돌봄 역할 <b>{formatPercent(ownHouseholdMember.responsibilities.caregiving)}</b>
                  </span>
                )
              : null}
          </div>
        ) : null}
        {pregnancy ? (
          <div className="focused-life-chips">
            <span>
              임신 단계 <b>{formatPregnancyStage(pregnancy.stage)}</b>
            </span>
            {pregnancy.dueMinute !== undefined
              ? <span>출산 예정 <b>{formatDay(pregnancy.dueMinute)}</b></span>
              : null}
            <span>임신 건강 <b>{formatPercent(pregnancy.health)}</b></span>
            <span>영양 <b>{formatPercent(pregnancy.nutrition)}</b></span>
            <span>피로 <b>{formatPercent(pregnancy.fatigue)}</b></span>
            <span>스트레스 <b>{formatPercent(pregnancy.stress)}</b></span>
          </div>
        ) : null}
      </div>

      <div className="focused-life-section">
        <h3>기억</h3>
        {memories.length > 0
          ? memories.map((memory, index) => (
              <div className="memory-row" key={`${memory.minute ?? 0}:${index}`}>
                <span>{formatMemoryText(memory.what)}</span>
                <small>
                  신뢰도 {formatPercent(memory.effectiveConfidence ?? memory.confidence)}
                  {memory.recallScore !== undefined
                    ? ` · 회상 ${formatPercent(memory.recallScore)}`
                    : ''}
                  {memory.where ? ` · ${formatLocationText(memory.where)}` : ''}
                  {memory.tags?.length
                    ? ` · ${memory.tags.map((tag) => `#${formatMemoryTag(tag)}`).join(' ')}`
                    : ''}
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
              <span>{formatBelief(belief.proposition)}</span>
              <small>
                확신 {formatPercent(belief.confidence)}
                {belief.supportCount !== undefined
                  ? ` · 지지 ${belief.supportCount}`
                  : ''}
                {belief.contradictionCount !== undefined
                  ? ` · 반박 ${belief.contradictionCount}`
                  : ''}
              </small>
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
                residents={residents}
                selected={resident.id === selectedResidentId}
                onSelect={onSelect}
              />
            ))
          : <div className="empty">표시할 주민이 없습니다.</div>}
      </div>
    </>
  );
}
