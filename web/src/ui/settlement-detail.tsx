import { useEffect, useRef } from 'react';
import { observerActions } from '../state/observer-actions';
import type { ObserverSnapshot } from '../state/observer-store';
import { representativeStorage, settlementLabel } from '../state/settlement-observation';
import { WORLD_PRESENTATION } from '../render/world-presentation-config';
import { formatMaterial } from './observer-format';

/** Exactly one dialog, with native keyboard focus and Escape handling. */
export function SettlementDetail({ snapshot }: { snapshot: ObserverSnapshot }) {
  const ref = useRef<HTMLDialogElement>(null);
  const settlement = snapshot.civilization.available === true
    ? snapshot.civilization.settlements?.find(entry => entry.id === snapshot.selectedSettlementId) : undefined;
  useEffect(() => {
    const dialog = ref.current;
    if (settlement && dialog && !dialog.open) dialog.showModal();
    if (!settlement && dialog?.open) dialog.close();
  }, [settlement?.id]);
  const close = () => observerActions.selectSettlement(null);
  const storage = settlement && representativeStorage(settlement.id, snapshot.civilization);
  const routes = settlement ? (snapshot.civilization.tradeRoutes ?? []).filter(route =>
    route.firstSettlement === settlement.id || route.secondSettlement === settlement.id)
    .slice(0, WORLD_PRESENTATION.settlement.maxRelationsInPopup) : [];
  const events = settlement ? snapshot.observations.filter(event =>
    event.settlementId === settlement.id || event.relatedSettlementId === settlement.id)
    .slice(0, WORLD_PRESENTATION.settlement.maxRecentEvents) : [];
  return (
    <dialog ref={ref} className="settlement-dialog" aria-labelledby="settlement-title" onCancel={close}>
      {settlement && <>
        <div className="human-trace-heading">
          <h2 id="settlement-title">{settlementLabel(settlement.id)}</h2>
          <button type="button" onClick={close} aria-label="정착지 정보 닫기">닫기</button>
        </div>
        <p className="hint">실제 시설과 저장소를 중심으로 형성된 생활권입니다. 바닥의 은은한 표시는 생활권의 중심 위치입니다.</p>
        <dl className="settlement-facts">
          <dt>인구</dt><dd>{settlement.residentCount}명</dd>
          <dt>정착 기반</dt><dd>{settlement.established ? '기반 형성' : '형성 중'}</dd>
          <dt>활동</dt><dd>{settlement.active ? '주민·가동 시설·저장소 확인' : '현재 활동 없음'}</dd>
          <dt>시설</dt><dd>{settlement.facilityCount}곳 · 가동 {settlement.operationalFacilityCount}곳 · 계획 {settlement.plannedFacilityCount}곳</dd>
          <dt>저장소</dt><dd>{settlement.storageSiteCount}곳</dd>
        </dl>
        {storage && <section className="focused-life-section">
          <h3>대표 저장소 보관량</h3>
          <p>총 {storage.totalUnits}개</p>
          <div className="focused-life-chips">{(storage.inventory ?? []).filter(item => Number(item.quantity) > 0).slice(0, WORLD_PRESENTATION.storage.maxInventoryRows).map(item =>
            <span key={item.material}>{formatMaterial(item.material)} <b>{item.quantity}개</b></span>)}</div>
        </section>}
        <section className="focused-life-section">
          <h3>다른 생활권과의 교역</h3>
          {routes.length ? routes.map(route => {
            const other = route.firstSettlement === settlement.id ? route.secondSettlement : route.firstSettlement;
            return <p key={route.id}>{settlementLabel(other)} · {route.active ? '교역 활성' : '교역 기록 있음'}
              {' · '}교환 {route.exchangeCount}회 · 거래 파트너 {route.partnerCount}쌍</p>;
          }) : <p className="hint">관측된 정착지 간 교역 기록이 없습니다.</p>}
        </section>
        {events.length > 0 && <section className="focused-life-section">
          <h3>최근 관측한 변화</h3>
          {events.map(event => <p key={event.id}>{event.summary}</p>)}
        </section>}
      </>}
    </dialog>
  );
}
