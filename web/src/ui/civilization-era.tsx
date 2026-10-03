import { useId, useRef } from 'react';
import type { CivilizationEraObservation, CivilizationEraEvidence } from '../runtime/core-types';

const eraLabels: Record<string, string> = {
  NaturalSurvival: '자연 생존기', EarlySettlement: '초기 정착기',
  AgrarianSettlement: '농경 정착기', CopperMetallurgy: '초기 금속기',
  BronzeTechnology: '청동 기술기',
};
const evidenceLabels: Record<string, string> = {
  OperationalSleep: '운영 중인 잠자리 또는 주거',
  SettlementInfrastructure: '저장·불·주거 중 둘 이상 운영 또는 실제 저장 기반',
  BasicLivingCapabilities: '저장·불 사용·액체 운반 능력 중 둘 이상',
  OperationalCultivation: '실제로 운용 가능한 재배 지식', CultivateFood: '재배 능력',
  OperationalPlot: '운영 중인 경작지', ManagedFoodProduction: '파종 또는 수확이 있는 관리 식량 생산',
  OperationalCopperSmelting: '실제로 운용 가능한 구리 제련', SmeltMetal: '금속 제련 능력',
  OperationalFurnace: '운영 중인 제련로', MetallurgicalProduction: '현재 금속 생산물',
  OperationalTinSmelting: '실제로 운용 가능한 주석 제련',
  OperationalBronzeAlloying: '실제로 운용 가능한 청동 합금', AlloyMetal: '합금 능력',
  AdvancedTooling: '실제 청동 도구 보유',
};

export const formatCivilizationEra = (id: string) => eraLabels[id] ?? '문명 단계 확인 중';
export const formatEraEvidence = (id: string) => evidenceLabels[id] ?? '새로운 운영 근거';

function EvidenceList({ evidence }: { evidence: CivilizationEraEvidence[] }) {
  return <ul className="era-evidence">{evidence.map(fact => <li key={fact.id}>
    <b aria-label={fact.satisfied ? '충족' : '미충족'}>{fact.satisfied ? '✓' : '✕'}</b>
    {formatEraEvidence(fact.id)}
  </li>)}</ul>;
}

export function CivilizationEraBadge({ era }: { era?: CivilizationEraObservation }) {
  const dialog = useRef<HTMLDialogElement>(null);
  const titleId = useId();
  if (!era) return <span>시대 · 관찰 준비 중</span>;
  return <>
    <button type="button" className="era-badge" aria-haspopup="dialog"
      onClick={() => dialog.current?.showModal()}>
      시대 · {formatCivilizationEra(era.currentEra.id)}
    </button>
    <dialog ref={dialog} className="era-dialog" aria-labelledby={titleId}
      onClick={event => { if (event.target === event.currentTarget) dialog.current?.close(); }}>
      <div className="era-dialog-content">
        <button type="button" className="era-close" onClick={() => dialog.current?.close()}>닫기</button>
        <h2 id={titleId}>현재 문명 단계</h2>
        <p className="era-current">{formatCivilizationEra(era.currentEra.id)}</p>
        <h3>판정 근거</h3>
        {era.evidence.length ? <EvidenceList evidence={era.evidence} />
          : <p>정착 생활 기반이 아직 안정적으로 운영되지 않고 있습니다.</p>}
        <h3>다음 단계</h3>
        {era.nextEra ? <>
          <p>{formatCivilizationEra(era.nextEra)}</p>
          <h3>필요 조건</h3>
          <EvidenceList evidence={era.nextEraRequirements} />
        </> : <p>현재 단계의 운영 근거를 계속 관찰합니다.</p>}
        <p className="era-note">현재 실제 운영 상태를 요약합니다. 시설이나 능력을 잃으면 단계가 내려갈 수 있습니다.</p>
      </div>
    </dialog>
  </>;
}
