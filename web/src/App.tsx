import { useEffect, useRef, useState } from 'react';
import { startObserverEngine } from './observer-engine';
import {
  SIMULATION_SPEED_MODES,
  WORLD_GRID_CONTRACT,
  simulationTimeHint,
} from './runtime/lifelens-contract';
import { observerActions } from './state/observer-actions';
import { observerStore } from './state/observer-store';
import { useObserverSnapshot } from './state/use-observer-snapshot';
import {
  ObserverMetrics,
  ResidentReadout,
  RuntimeBadge,
  SelectedResidentReadout,
  WorldOverlay,
} from './ui/observer-readout';
import { DiagnosticsPanel } from './ui/diagnostics-panel';
import {
  ObservationFeedOverlay,
  ObservationFeedPanel,
  ObservationFocusBanner,
} from './ui/observation-feed';
import { RenderModeControl } from './ui/render-mode-control';
import { HumanTraceDetail, HumanTracePanel } from './ui/human-traces';
import { WorldActivityPanel } from './ui/world-activity';
import { FastForwardControl } from './ui/fast-forward-control';
import { visibleHumanTraces } from './state/human-traces';

type MobileModalView =
  | 'menu'
  | 'residents'
  | 'resident'
  | 'seed'
  | 'events'
  | 'traces'
  | 'activity'
  | 'world'
  | null;

function Topbar() {
  const snapshot = useObserverSnapshot();

  return (
    <header className="topbar">
      <div className="brand">
        <strong>라이프렌즈</strong>
        <span>실시간 관찰</span>
      </div>
      <RuntimeBadge runtime={snapshot.runtime} />
    </header>
  );
}

function WorldViewport({
  onOpenMenu,
  onOpenResidents,
  onOpenSeed,
}: {
  onOpenMenu: () => void;
  onOpenResidents: () => void;
  onOpenSeed: () => void;
}) {
  const snapshot = useObserverSnapshot();
  const selectedTrace = visibleHumanTraces(snapshot.terrain)
    .find(trace => trace.id === snapshot.selectedHumanTraceId);
  const focusedObservation = snapshot.focusedObservationId
    ? snapshot.observations.find(
        (event) => event.id === snapshot.focusedObservationId,
      ) ?? null
    : null;
  const worldSeed = snapshot.world.worldSeed;

  return (
    <section className="world">
      <canvas id="worldCanvas" aria-label="라이프렌즈 세계 관찰 화면" />
      <canvas id="threeWorldCanvas" aria-label="라이프렌즈 3차원 세계" />
      <canvas id="characterCanvas" aria-label="라이프렌즈 주민" />
      <WorldOverlay snapshot={snapshot} />

      <button
        className="mobile-seed-chip"
        type="button"
        onClick={onOpenSeed}
        aria-label="현재 월드 시드 보기"
      >
        Seed #{worldSeed ?? '—'}
      </button>

      {focusedObservation ? (
        <ObservationFocusBanner
          event={focusedObservation}
          onClear={() => observerActions.clearObservationFocus()}
        />
      ) : null}

      {snapshot.fastForward.status === 'running' && (
        <div className="fast-forward-world-status" aria-live="polite">
          <strong>세계 변화 계산 중</strong>
          <span>
            {Math.round(snapshot.fastForward.progress01 * 100)}%
            {' · '}
            {Math.floor(snapshot.fastForward.completedMinutes / 1440)}일 /
            {' '}{snapshot.fastForward.requestedDays}일
          </span>
        </div>
      )}

      {!selectedTrace && (
        <ObservationFeedOverlay
          observations={snapshot.observations}
          onFocus={(event) => observerActions.focusObservation(event)}
        />
      )}

      <div className="mobile-quick-actions" aria-label="모바일 관찰 메뉴">
        <button type="button" onClick={onOpenResidents}>주민</button>
        <button type="button" onClick={onOpenMenu}>메뉴</button>
      </div>

      <div
        id="errorCard"
        className={`error-card ${snapshot.runtime.status === 'error' ? '' : 'hidden'}`}
      >
        <strong>시뮬레이션 코어 연결 실패</strong>
        <span id="errorText">
          {'시뮬레이션 코어를 불러오지 못했습니다. 다시 실행해 주세요.'}
        </span>
      </div>
    </section>
  );
}

function ObserverPanel({
  onViewPlace,
}: {
  onViewPlace: () => void;
}) {
  const snapshot = useObserverSnapshot();
  const panelRef = useRef<HTMLElement>(null);
  const [seed, setSeed] = useState('');
  const [seedError, setSeedError] = useState(false);
  const controlsDisabled = snapshot.runtime.status !== 'ready'
    || snapshot.fastForward.status === 'running';
  const selectedResident = snapshot.selectedResidentId
    ? snapshot.residents.find(
      (resident) => resident.id === snapshot.selectedResidentId,
    ) ?? null
    : null;
  const selectedTrace = visibleHumanTraces(snapshot.terrain)
    .find(trace => trace.id === snapshot.selectedHumanTraceId);

  useEffect(() => {
    if (panelRef.current) panelRef.current.scrollTop = 0;
  }, [snapshot.selectedResidentId]);

  const createWorld = (): void => {
    setSeedError(false);
    observerActions.createWorld('');
  };

  const createWorldFromSeed = (): void => {
    const trimmed = seed.trim();
    if (!trimmed) {
      setSeedError(true);
      return;
    }

    setSeedError(false);
    observerActions.createWorld(trimmed);
  };

  return (
    <aside id="observer-panel" ref={panelRef} className="observer desktop-observer">
      <section className="panel">
        <h2>{selectedTrace ? '선택한 장소' : '선택한 삶'}</h2>
        {selectedTrace ? (
          <HumanTraceDetail
            trace={selectedTrace}
            residents={snapshot.residents}
            onClose={() => observerActions.selectHumanTrace(null)}
          />
        ) : (
          <SelectedResidentReadout
            resident={selectedResident}
            onClear={() => observerActions.selectResident(null)}
          />
        )}
      </section>

      <section className="panel">
        <h2>생활 흔적</h2>
        <HumanTracePanel
          terrain={snapshot.terrain}
          selectedId={snapshot.selectedHumanTraceId}
          onFocus={(id) => {
            observerActions.selectHumanTrace(id, true);
            onViewPlace();
          }}
        />
      </section>

      <section className="panel">
        <h2>월드</h2>
        <div className="button-row single">
          <button onClick={createWorld} disabled={controlsDisabled}>
            새 월드
          </button>
        </div>

        <details className="seed-replay">
          <summary>시드로 동일한 세계 재현</summary>
          <label className="field">
            WorldSeed
            <input
              inputMode="numeric"
              value={seed}
              autoComplete="off"
              placeholder="재현할 시드 입력"
              onChange={(event) => {
                setSeed(event.target.value);
                if (seedError) setSeedError(false);
              }}
              onKeyDown={(event) => {
                if (event.key === 'Enter') createWorldFromSeed();
              }}
            />
          </label>
          <p className={`field-error ${seedError ? '' : 'hidden'}`}>
            재현할 월드 시드를 입력해 주세요.
          </p>
          <button
            type="button"
            onClick={createWorldFromSeed}
            disabled={controlsDisabled}
          >
            이 시드로 생성
          </button>
        </details>

        <div className="time-speed-control" aria-label="시뮬레이션 관찰 속도">
          {SIMULATION_SPEED_MODES.map((preset) => (
            <button
              key={preset.speed}
              type="button"
              title={preset.title}
              className={snapshot.simulationSpeed === preset.speed ? 'active' : ''}
              onClick={() => observerActions.setSimulationSpeed(preset.speed)}
              disabled={controlsDisabled}
              aria-pressed={snapshot.simulationSpeed === preset.speed}
            >
              {preset.label}
            </button>
          ))}
        </div>
        <p className="hint">{simulationTimeHint()}</p>

        <FastForwardControl
          state={snapshot.fastForward}
          disabled={snapshot.runtime.status !== 'ready'}
        />
      </section>

      <section className="panel">
        <h2>관찰 정보</h2>
        <ObserverMetrics snapshot={snapshot} />
      </section>

      <section className="panel">
        <h2>월드 활동</h2>
        <WorldActivityPanel
          civilization={snapshot.civilization}
          worldObjects={snapshot.worldObjects}
          residents={snapshot.residents}
          onSelectResident={(residentId) => observerActions.selectResident(residentId)}
          onFocusGrid={(gridX, gridY) => {
            const targetChunkX = Math.floor(
              gridX / WORLD_GRID_CONTRACT.gridCellsPerChunk,
            );
            const targetChunkY = Math.floor(
              gridY / WORLD_GRID_CONTRACT.gridCellsPerChunk,
            );
            observerActions.moveObserver(
              targetChunkX - snapshot.camera.centerChunkX,
              targetChunkY - snapshot.camera.centerChunkY,
            );
            onViewPlace();
          }}
        />
      </section>

      <section className="panel">
        <h2>최근 관찰 포착</h2>
        <ObservationFeedPanel
          observations={snapshot.observations}
          onFocus={(event) => {
            observerActions.focusObservation(event);
            onViewPlace();
          }}
        />
      </section>

      <section className="panel">
        <ResidentReadout
          residents={snapshot.residents}
          selectedResidentId={snapshot.selectedResidentId}
          onSelect={(residentId) => observerActions.selectResident(residentId)}
        />
      </section>

      <section className="panel">
        <h2>카메라</h2>
        <button
          onClick={() => observerActions.recenterObserver()}
          disabled={controlsDisabled}
        >
          현재 주민 위치로 돌아가기
        </button>
        <p className="hint">
          모바일: 한 손가락 드래그 회전 · 두 손가락 이동 팬 · 핀치 줌 · 컴퓨터: 우클릭 회전 · 중클릭 팬 · 휠 줌
        </p>
      </section>

      <DiagnosticsPanel />
      <RenderModeControl />
    </aside>
  );
}

function MobileObserverModal({
  view,
  onView,
  onClose,
}: {
  view: MobileModalView;
  onView: (view: MobileModalView) => void;
  onClose: () => void;
}) {
  const snapshot = useObserverSnapshot();
  const [seed, setSeed] = useState('');
  const [seedError, setSeedError] = useState(false);
  const [seedCopied, setSeedCopied] = useState(false);
  const controlsDisabled = snapshot.runtime.status !== 'ready'
    || snapshot.fastForward.status === 'running';
  const selectedResident = snapshot.selectedResidentId
    ? snapshot.residents.find(
        (resident) => resident.id === snapshot.selectedResidentId,
      ) ?? null
    : null;
  const selectedTrace = visibleHumanTraces(snapshot.terrain)
    .find(trace => trace.id === snapshot.selectedHumanTraceId);
  const currentSeed = snapshot.world.worldSeed;

  useEffect(() => {
    setSeedCopied(false);
  }, [view, currentSeed]);

  if (!view) return null;

  const createWorld = (): void => {
    if (!window.confirm('현재 세계를 종료하고 새 월드를 생성할까요?')) return;
    setSeedError(false);
    observerActions.createWorld('');
    onClose();
  };

  const createWorldFromSeed = (): void => {
    const trimmed = seed.trim();
    if (!trimmed) {
      setSeedError(true);
      return;
    }
    if (!window.confirm(`현재 세계를 종료하고 Seed #${trimmed}로 다시 시작할까요?`)) {
      return;
    }

    setSeedError(false);
    observerActions.createWorld(trimmed);
    onClose();
  };

  const replayCurrentSeed = (): void => {
    if (currentSeed === undefined || currentSeed === null) return;
    const value = String(currentSeed);
    if (!window.confirm(`현재 세계를 종료하고 Seed #${value}로 처음부터 재현할까요?`)) {
      return;
    }
    observerActions.createWorld(value);
    onClose();
  };

  const copyCurrentSeed = async (): Promise<void> => {
    if (currentSeed === undefined || currentSeed === null) return;
    try {
      await navigator.clipboard.writeText(String(currentSeed));
      setSeedCopied(true);
    } catch {
      setSeedCopied(false);
    }
  };

  const title = {
    menu: '관찰 메뉴',
    residents: '주민',
    resident: selectedResident?.name ?? '주민 상세',
    seed: '현재 월드 시드',
    events: '최근 관찰',
    traces: '생활 흔적',
    activity: '월드 활동',
    world: '월드 설정',
  }[view];

  const focusGrid = (gridX: number, gridY: number): void => {
    const targetChunkX = Math.floor(
      gridX / WORLD_GRID_CONTRACT.gridCellsPerChunk,
    );
    const targetChunkY = Math.floor(
      gridY / WORLD_GRID_CONTRACT.gridCellsPerChunk,
    );
    observerActions.moveObserver(
      targetChunkX - snapshot.camera.centerChunkX,
      targetChunkY - snapshot.camera.centerChunkY,
    );
    onClose();
  };

  return (
    <div
      className="mobile-modal-backdrop"
      role="presentation"
      onMouseDown={(event) => {
        if (event.target === event.currentTarget) onClose();
      }}
    >
      <section
        className={`mobile-modal mobile-modal-${view}`}
        role="dialog"
        aria-modal="true"
        aria-label={title}
      >
        <header className="mobile-modal-header">
          {view !== 'menu' ? (
            <button
              type="button"
              className="mobile-modal-back"
              onClick={() => onView('menu')}
              aria-label="관찰 메뉴로 돌아가기"
            >
              ‹
            </button>
          ) : <span className="mobile-modal-header-spacer" />}
          <strong>{title}</strong>
          <button
            type="button"
            className="mobile-modal-close"
            onClick={onClose}
            aria-label="팝업 닫기"
          >
            ×
          </button>
        </header>

        <div className="mobile-modal-body">
          {view === 'menu' && (
            <>
              <div className="mobile-world-summary">
                <button type="button" onClick={() => onView('seed')}>
                  <span>현재 Seed</span>
                  <b>#{currentSeed ?? '—'}</b>
                </button>
                <div>
                  <span>주민</span>
                  <b>{snapshot.residents.length}명</b>
                </div>
              </div>

              <div className="mobile-menu-grid">
                <button type="button" onClick={() => onView('residents')}>
                  <strong>주민</strong>
                  <span>삶과 욕구 확인</span>
                </button>
                <button type="button" onClick={() => onView('events')}>
                  <strong>최근 관찰</strong>
                  <span>주요 사건 보기</span>
                </button>
                <button type="button" onClick={() => onView('traces')}>
                  <strong>생활 흔적</strong>
                  <span>시설과 흔적 보기</span>
                </button>
                <button type="button" onClick={() => onView('activity')}>
                  <strong>월드 활동</strong>
                  <span>자원과 문명 활동</span>
                </button>
                <button type="button" onClick={() => onView('world')}>
                  <strong>월드 설정</strong>
                  <span>시간 · 카메라 · 재현</span>
                </button>
              </div>

              <details className="mobile-advanced">
                <summary>고급 표시 설정</summary>
                <RenderModeControl />
                <DiagnosticsPanel />
              </details>
            </>
          )}

          {view === 'residents' && (
            <ResidentReadout
              residents={snapshot.residents}
              selectedResidentId={snapshot.selectedResidentId}
              onSelect={(residentId) => {
                observerActions.selectResident(residentId);
                onView('resident');
              }}
            />
          )}

          {view === 'resident' && (
            <SelectedResidentReadout
              resident={selectedResident}
              onClear={() => {
                observerActions.selectResident(null);
                onView('residents');
              }}
            />
          )}

          {view === 'seed' && (
            <div className="mobile-seed-detail">
              <span>현재 실행 중인 WorldSeed</span>
              <strong>#{currentSeed ?? '—'}</strong>
              <p>
                이 값으로 같은 월드를 처음부터 다시 생성할 수 있습니다.
              </p>
              <div className="mobile-action-row">
                <button
                  type="button"
                  onClick={() => void copyCurrentSeed()}
                  disabled={currentSeed === undefined || currentSeed === null}
                >
                  {seedCopied ? '복사됨' : 'Seed 복사'}
                </button>
                <button
                  type="button"
                  onClick={replayCurrentSeed}
                  disabled={
                    controlsDisabled
                    || currentSeed === undefined
                    || currentSeed === null
                  }
                >
                  이 Seed로 재현
                </button>
              </div>
            </div>
          )}

          {view === 'events' && (
            <ObservationFeedPanel
              observations={snapshot.observations}
              onFocus={(event) => {
                observerActions.focusObservation(event);
                onClose();
              }}
            />
          )}

          {view === 'traces' && (
            <>
              {selectedTrace ? (
                <HumanTraceDetail
                  trace={selectedTrace}
                  residents={snapshot.residents}
                  onClose={() => observerActions.selectHumanTrace(null)}
                />
              ) : null}
              <HumanTracePanel
                terrain={snapshot.terrain}
                selectedId={snapshot.selectedHumanTraceId}
                onFocus={(id) => {
                  observerActions.selectHumanTrace(id, true);
                }}
              />
            </>
          )}

          {view === 'activity' && (
            <WorldActivityPanel
              civilization={snapshot.civilization}
              worldObjects={snapshot.worldObjects}
              residents={snapshot.residents}
              onSelectResident={(residentId) => {
                observerActions.selectResident(residentId);
                onView('resident');
              }}
              onFocusGrid={focusGrid}
            />
          )}

          {view === 'world' && (
            <div className="mobile-world-controls">
              <section>
                <h3>관찰 속도</h3>
                <div className="time-speed-control" aria-label="시뮬레이션 관찰 속도">
                  {SIMULATION_SPEED_MODES.map((preset) => (
                    <button
                      key={preset.speed}
                      type="button"
                      title={preset.title}
                      className={snapshot.simulationSpeed === preset.speed ? 'active' : ''}
                      onClick={() => observerActions.setSimulationSpeed(preset.speed)}
                      disabled={controlsDisabled}
                      aria-pressed={snapshot.simulationSpeed === preset.speed}
                    >
                      {preset.label}
                    </button>
                  ))}
                </div>
                <p className="hint">{simulationTimeHint()}</p>
                <FastForwardControl
                  state={snapshot.fastForward}
                  disabled={snapshot.runtime.status !== 'ready'}
                />
              </section>

              <section>
                <h3>카메라</h3>
                <button
                  type="button"
                  className="mobile-full-button"
                  onClick={() => {
                    observerActions.recenterObserver();
                    onClose();
                  }}
                  disabled={controlsDisabled}
                >
                  현재 주민 위치로 돌아가기
                </button>
              </section>

              <section>
                <h3>월드</h3>
                <div className="mobile-current-seed">
                  <span>현재 Seed</span>
                  <b>#{currentSeed ?? '—'}</b>
                </div>
                <button
                  type="button"
                  className="mobile-full-button"
                  onClick={createWorld}
                  disabled={controlsDisabled}
                >
                  새 월드
                </button>

                <label className="field mobile-seed-input">
                  시드로 동일한 세계 재현
                  <input
                    inputMode="numeric"
                    value={seed}
                    autoComplete="off"
                    placeholder="재현할 시드 입력"
                    onChange={(event) => {
                      setSeed(event.target.value);
                      if (seedError) setSeedError(false);
                    }}
                    onKeyDown={(event) => {
                      if (event.key === 'Enter') createWorldFromSeed();
                    }}
                  />
                </label>
                <p className={`field-error ${seedError ? '' : 'hidden'}`}>
                  재현할 월드 시드를 입력해 주세요.
                </p>
                <button
                  type="button"
                  className="mobile-full-button"
                  onClick={createWorldFromSeed}
                  disabled={controlsDisabled}
                >
                  입력한 Seed로 생성
                </button>
              </section>

              <section>
                <h3>관찰 정보</h3>
                <ObserverMetrics snapshot={snapshot} />
              </section>
            </div>
          )}
        </div>
      </section>
    </div>
  );
}

export default function App() {
  const [mobileModal, setMobileModal] = useState<MobileModalView>(null);
  const snapshot = useObserverSnapshot();

  useEffect(() => {
    if (
      snapshot.selectedResidentId
      && window.matchMedia('(max-width: 800px)').matches
    ) {
      setMobileModal('resident');
    }
  }, [snapshot.selectedResidentId]);

  useEffect(() => {
    if (
      snapshot.selectedHumanTraceId
      && window.matchMedia('(max-width: 800px)').matches
    ) {
      setMobileModal('traces');
    }
  }, [snapshot.selectedHumanTraceId]);

  useEffect(() => {
    try {
      startObserverEngine();
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      console.error('LifeLens observer engine startup failed', error);
      observerStore.setRuntime('error', message);
    }
  }, []);

  return (
    <div id="app">
      <Topbar />
      <main className="layout">
        <WorldViewport
          onOpenMenu={() => setMobileModal('menu')}
          onOpenResidents={() => setMobileModal('residents')}
          onOpenSeed={() => setMobileModal('seed')}
        />
        <ObserverPanel onViewPlace={() => undefined} />
      </main>

      <MobileObserverModal
        view={mobileModal}
        onView={setMobileModal}
        onClose={() => setMobileModal(null)}
      />
    </div>
  );
}
