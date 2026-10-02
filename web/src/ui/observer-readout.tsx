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
  formatResidentName,
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

const migrationMaterialLabels: Record<string, string> = {
  Water: '물',
  PlantFood: '식량',
  Wood: '목재',
  Stone: '석재',
  Fiber: '섬유',
  Clay: '점토',
  Unknown: '미확인 자원',
};

function formatMigrationMaterial(material: string | undefined): string {
  if (!material) return '미확인 자원';
  return migrationMaterialLabels[material] ?? material;
}

function residentDisplayName(
  residents: Resident[],
  id: string | undefined,
  fallbackName?: string,
): string {
  if (fallbackName?.trim()) return formatResidentName(fallbackName);
  if (!id) return '대상 미확인';
  const resident = residents.find((candidate) => candidate.id === String(id));
  return resident ? formatResidentName(resident.name) : `주민 #${id}`;
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
        <strong>{formatResidentName(resident.name)}</strong>
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
  const visibleMemories = memories.map((memory) => ({
    memory,
    subjectName: residentDisplayName(residents, memory.who),
    tags: Array.from(new Set((memory.tags ?? []).map(formatMemoryTag))),
  }));
  const visibleBeliefs = beliefs.map((belief) => ({
    belief,
    subjectName: residentDisplayName(residents, belief.subject),
  }));
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
    ? `${formatResidentName(family.partnerName)} · ${formatPartnerStage(family.partnerStage)}`
    : '현재 파트너 없음';
  const closeFamily = [
    ...(family?.parents ?? []),
    ...(family?.children ?? []),
    ...(family?.siblings ?? []),
  ].slice(0, 6);
  const statusText = residentStatusText(resident, residents);
  const healthStageLabels: Record<string, string> = {
    Well: '양호',
    Exposed: '노출',
    Ill: '질병',
    Recovering: '회복 중',
    Injured: '부상',
    Critical: '위중',
  };
  const health=resident.health;
  const migration=resident.migration;
  const showMigrationPressure=
    Number(migration?.pressure01) >= 0.12
    || migration?.candidate === true;
  const activityTarget = !resident.presentation?.active && resident.activityTargetName
    ? ` → ${resident.activityTargetName}`
    : '';

  return (
    <article className="focused-life">
      <div className="focused-life-heading">
        <div>
          <span>집중 관찰</span>
          <strong>{formatResidentName(resident.name)}</strong>
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

      {showMigrationPressure ? (
        <div className="focused-life-section">
          <h3>이동 / 이주 압력</h3>
          <div className="focused-life-chips">
            <span>
              현재 압력 <b>{formatPercent(migration?.pressure01)}</b>
            </span>
            <span>
              부족 자원 <b>{formatMigrationMaterial(migration?.bottleneckMaterial)}</b>
            </span>
            <span>
              지역 희소성 <b>{formatPercent(migration?.resourceScarcity01)}</b>
            </span>
            <span>
              이동 부담 <b>{formatPercent(migration?.travelBurden01)}</b>
            </span>
            <span>
              정착 애착 <b>{formatPercent(migration?.settlementAttachment01)}</b>
            </span>
            {migration?.candidate
              ? <span className="life-event-chip">이주 후보</span>
              : null}
            {migration?.hasFrontierTarget
              ? (
                  <span>
                    탐색 거리 <b>{migration.frontierDistanceChunks ?? 0}청크</b>
                  </span>
                )
              : null}
          </div>
        </div>
      ) : null}

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

      {health ? (
        <div className="focused-life-section">
          <h3>건강 상태</h3>
          <div className="focused-life-chips">
            <span>상태 <b>{healthStageLabels[health.stage ?? 'Well'] ?? health.stage ?? '양호'}</b></span>
            <span>활동 능력 <b>{formatPercent(health.functionalCapacity)}</b></span>
            <span>병원체 부담 <b>{formatPercent(health.pathogenLoad)}</b></span>
            <span>질병 심각도 <b>{formatPercent(health.illnessSeverity)}</b></span>
            <span>면역 <b>{formatPercent(health.immunity)}</b></span>
            <span>부상 <b>{formatPercent(health.injurySeverity)}</b></span>
            <span>환경 스트레스 <b>{formatPercent(health.environmentalStress)}</b></span>
            <span>돌봄 지식 <b>{formatPercent(health.careKnowledge)}</b></span>
            {(health.infectionEpisodes ?? 0) > 0
              ? <span>감염 경험 <b>{health.infectionEpisodes}</b></span>
              : null}
            {(health.recoveryEpisodes ?? 0) > 0
              ? <span>회복 경험 <b>{health.recoveryEpisodes}</b></span>
              : null}
            {(health.accidentEpisodes ?? 0) > 0
              ? <span>사고 경험 <b>{health.accidentEpisodes}</b></span>
              : null}
          </div>
        </div>
      ) : null}

      {conditionEntries.length > 0 ? (
        <div className="focused-life-section">
          <h3>신체 / 생애 조건</h3>
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
                <strong>{residentDisplayName(
                  residents,
                  relationship.targetId,
                  relationship.targetName,
                )}</strong>
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
                {residentDisplayName(residents, member.id, member.name)}
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
        {visibleMemories.length > 0
          ? visibleMemories.map(({ memory, subjectName, tags }, index) => (
              <div className="memory-row" key={`${memory.minute ?? 0}:${index}`}>
                <span><b>{subjectName}</b> · {formatMemoryText(memory.what)}</span>
                <small>
                  신뢰도 {formatPercent(memory.effectiveConfidence ?? memory.confidence)}
                  {memory.recallScore !== undefined
                    ? ` · 회상 ${formatPercent(memory.recallScore)}`
                    : ''}
                  {memory.where ? ` · ${formatLocationText(memory.where)}` : ''}
                  {tags.length ? (
                    <>
                      {' · '}
                      {tags.map((tag) => (
                        <span className="memory-tag" key={tag}>#{tag}</span>
                      ))}
                    </>
                  ) : null}
                </small>
              </div>
            ))
          : <div className="focused-life-muted">아직 강하게 남은 기억이 없습니다.</div>}
      </div>

      {visibleBeliefs.length > 0 ? (
        <div className="focused-life-section">
          <h3>믿음</h3>
          {visibleBeliefs.map(({ belief, subjectName }, index) => (
            <div className="belief-row" key={`${belief.subject ?? '0'}:${index}`}>
              <span><b>{subjectName}</b> · {formatBelief(belief.proposition)}</span>
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
