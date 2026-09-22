import { useEffect, useState } from 'react';
import { startObserverEngine } from './observer-engine';
import {
  SIMULATION_SPEED_MODES,
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
import { RenderModeControl } from './ui/render-mode-control';

function Topbar() {
  const snapshot = useObserverSnapshot();

  return (
    <header className="topbar">
      <div className="brand">
        <strong>LifeLens</strong>
        <span>실시간 관찰</span>
      </div>
      <RuntimeBadge runtime={snapshot.runtime} />
    </header>
  );
}

function WorldViewport({
  onToggleObserver,
}: {
  onToggleObserver: () => void;
}) {
  const snapshot = useObserverSnapshot();

  return (
    <section className="world">
      <canvas id="worldCanvas" aria-label="LifeLens 월드 관찰 화면" />
      <canvas id="threeWorldCanvas" aria-label="LifeLens 3D 월드" />
      <canvas id="characterCanvas" aria-label="LifeLens 주민" />
      <WorldOverlay snapshot={snapshot} />
      <button
        className="observer-fab"
        type="button"
        onClick={onToggleObserver}
        aria-label="관찰 패널 열기"
      >
        관찰
      </button>
      <div
        id="errorCard"
        className={`error-card ${snapshot.runtime.status === 'error' ? '' : 'hidden'}`}
      >
        <strong>LifeLensCore 연결 실패</strong>
        <span id="errorText">
          {snapshot.runtime.errorMessage ?? '실제 Core WASM을 불러오지 못했습니다.'}
        </span>
      </div>
    </section>
  );
}

function ObserverPanel({
  mobileOpen,
  onToggleMobile,
}: {
  mobileOpen: boolean;
  onToggleMobile: () => void;
}) {
  const snapshot = useObserverSnapshot();
  const [seed, setSeed] = useState('');
  const [seedError, setSeedError] = useState(false);
  const controlsDisabled = snapshot.runtime.status !== 'ready';
  const selectedResident = snapshot.selectedResidentId
    ? snapshot.residents.find(
      (resident) => resident.id === snapshot.selectedResidentId,
    ) ?? null
    : null;

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
    <aside className={`observer ${mobileOpen ? 'mobile-open' : 'mobile-closed'}`}>
      <button
        className="observer-sheet-handle"
        type="button"
        onClick={onToggleMobile}
        aria-expanded={mobileOpen}
      >
        <span />
        {mobileOpen ? '세계 보기' : '관찰 정보'}
      </button>
      <section className="panel">
        <h2>월드</h2>
        <div className="button-row single">
          <button onClick={createWorld} disabled={controlsDisabled}>
            새 월드
          </button>
        </div>

        <details className="seed-replay">
          <summary>Seed로 동일한 월드 재현</summary>
          <label className="field">
            WorldSeed
            <input
              inputMode="numeric"
              value={seed}
              autoComplete="off"
              placeholder="재현할 Seed 입력"
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
            재현할 WorldSeed를 입력해 주세요.
          </p>
          <button
            type="button"
            onClick={createWorldFromSeed}
            disabled={controlsDisabled}
          >
            이 Seed로 생성
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
        <p className="hint">
          {simulationTimeHint()}
        </p>
      </section>

      <section className="panel">
        <h2>선택한 삶</h2>
        <SelectedResidentReadout
          resident={selectedResident}
          onClear={() => observerActions.selectResident(null)}
        />
      </section>

      <section className="panel">
        <h2>관찰 정보</h2>
        <ObserverMetrics snapshot={snapshot} />
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
          모바일: 한 손가락 드래그 회전 · 두 손가락 이동 팬 · 핀치 줌 · PC: 우클릭 회전 · 중클릭 팬 · 휠 줌
        </p>
      </section>

      <DiagnosticsPanel />
      <RenderModeControl />
    </aside>
  );
}

export default function App() {
  const [mobileObserverOpen, setMobileObserverOpen] = useState(false);
  const snapshot = useObserverSnapshot();

  useEffect(() => {
    if (snapshot.selectedResidentId) setMobileObserverOpen(true);
  }, [snapshot.selectedResidentId]);

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
          onToggleObserver={() => setMobileObserverOpen((open) => !open)}
        />
        <ObserverPanel
          mobileOpen={mobileObserverOpen}
          onToggleMobile={() => setMobileObserverOpen((open) => !open)}
        />
      </main>
    </div>
  );
}
