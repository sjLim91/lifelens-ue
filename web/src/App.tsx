import { useEffect, useState } from 'react';
import { startObserverEngine } from './observer-engine';
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
        <span>LIVE WEB OBSERVER</span>
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
      <canvas id="worldCanvas" aria-label="LifeLens world observer" />
      <canvas id="threeWorldCanvas" aria-label="LifeLens experimental Three.js world" />
      <canvas id="characterCanvas" aria-label="LifeLens residents" />
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
  const [seed, setSeed] = useState('42');
  const [seedError, setSeedError] = useState(false);
  const controlsDisabled = snapshot.runtime.status !== 'ready';
  const selectedResident = snapshot.selectedResidentId
    ? snapshot.residents.find(
      (resident) => resident.id === snapshot.selectedResidentId,
    ) ?? null
    : null;

  const createWorld = (): void => {
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
        <h2>World</h2>
        <label className="field">
          WorldSeed
          <input
            inputMode="numeric"
            value={seed}
            autoComplete="off"
            onChange={(event) => {
              setSeed(event.target.value);
              if (seedError) setSeedError(false);
            }}
            onKeyDown={(event) => {
              if (event.key === 'Enter') createWorld();
            }}
          />
        </label>
        <p className={`field-error ${seedError ? '' : 'hidden'}`}>
          WorldSeed를 입력해 주세요.
        </p>
        <div className="button-row">
          <button onClick={createWorld} disabled={controlsDisabled}>
            NEW WORLD
          </button>
          <button
            onClick={() => observerActions.stepMinutes(10)}
            disabled={controlsDisabled}
          >
            +10 MIN
          </button>
          <button
            onClick={() => observerActions.stepMinutes(60)}
            disabled={controlsDisabled}
          >
            +1 HOUR
          </button>
        </div>
      </section>

      <section className="panel">
        <h2>Focused life</h2>
        <SelectedResidentReadout
          resident={selectedResident}
          onClear={() => observerActions.selectResident(null)}
        />
      </section>

      <section className="panel">
        <h2>Observer</h2>
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
        <h2>Camera</h2>
        <button
          onClick={() => observerActions.recenterObserver()}
          disabled={controlsDisabled || snapshot.camera.followResidents}
        >
          현재 주민 위치로 돌아가기
        </button>
        <p className="hint">
          한 손가락 드래그: 지역 이동 · 핀치: 확대/축소 · 두 손가락 좌우 이동: 시점 회전
        </p>
      </section>

      <section className="panel runtime">
        <h2>Runtime</h2>
        <div><span>UI</span><b>React 19 + TypeScript</b></div>
        <div><span>Authority</span><b>LifeLensCore C++</b></div>
        <div><span>Client</span><b>Web / WASM</b></div>
        <div><span>Characters</span><b>Quaternius CC0 / Three.js</b></div>
        <div><span>Fallback</span><b>Fake world 없음</b></div>
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
