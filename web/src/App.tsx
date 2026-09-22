import { useEffect } from 'react';
import { useObserverSnapshot } from './state/use-observer-snapshot';
import {
  ObserverMetrics,
  ResidentReadout,
  RuntimeBadge,
  WorldOverlay,
} from './ui/observer-readout';

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

  return (
    <aside className="observer">
      <section className="panel">
        <h2>World</h2>
        <label className="field">
          WorldSeed
          <input
            id="seedInput"
            inputMode="numeric"
            defaultValue="42"
            autoComplete="off"
          />
        </label>
        <p id="seedError" className="field-error hidden">
          WorldSeed를 입력해 주세요.
        </p>
        <div className="button-row">
          <button id="newWorld">NEW WORLD</button>
          <button id="step10" disabled>+10 MIN</button>
          <button id="step60" disabled>+1 HOUR</button>
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
          <button id="left" disabled>←</button>
          <button id="up" disabled>↑</button>
          <button id="down" disabled>↓</button>
          <button id="right" disabled>→</button>
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
