import { useEffect, useState } from 'react';
import { observerActions } from './state/observer-actions';
import { useObserverSnapshot } from './state/use-observer-snapshot';
import {
  ObserverMetrics,
  ResidentReadout,
  RuntimeBadge,
  WorldOverlay,
} from './ui/observer-readout';
import { DiagnosticsPanel } from './ui/diagnostics-panel';

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

function WorldViewport() {
  const snapshot = useObserverSnapshot();

  return (
    <section className="world">
      <canvas id="worldCanvas" aria-label="LifeLens world observer" />
      <canvas id="characterCanvas" aria-label="LifeLens residents" />
      <WorldOverlay snapshot={snapshot} />
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

function ObserverPanel() {
  const snapshot = useObserverSnapshot();
  const [seed, setSeed] = useState('42');
  const [seedError, setSeedError] = useState(false);
  const controlsDisabled = snapshot.runtime.status !== 'ready';

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
    <aside className="observer">
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
        <h2>Observer</h2>
        <ObserverMetrics snapshot={snapshot} />
      </section>

      <section className="panel">
        <ResidentReadout residents={snapshot.residents} />
      </section>

      <section className="panel">
        <h2>World truth</h2>
        <div className="chunk-controls">
          <button
            onClick={() => observerActions.moveObserver(-1, 0)}
            disabled={controlsDisabled}
          >
            ←
          </button>
          <button
            onClick={() => observerActions.moveObserver(0, 1)}
            disabled={controlsDisabled}
          >
            ↑
          </button>
          <button
            onClick={() => observerActions.moveObserver(0, -1)}
            disabled={controlsDisabled}
          >
            ↓
          </button>
          <button
            onClick={() => observerActions.moveObserver(1, 0)}
            disabled={controlsDisabled}
          >
            →
          </button>
        </div>
        <p className="hint">
          드래그: 시점 회전 · 휠/핀치: 확대/축소 · 화살표: 관찰 Chunk 이동
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
    </aside>
  );
}

export default function App() {
  useEffect(() => {
    void import('./observer-engine');
  }, []);

  return (
    <div id="app">
      <Topbar />
      <main className="layout">
        <WorldViewport />
        <ObserverPanel />
      </main>
    </div>
  );
}
