import { useEffect } from 'react';

function Topbar() {
  return (
    <header className="topbar">
      <div className="brand"><strong>LifeLens</strong><span>LIVE WEB OBSERVER</span></div>
      <div id="status" className="status pending">Core loading…</div>
    </header>
  );
}

function WorldViewport() {
  return (
    <section className="world">
      <canvas id="worldCanvas" aria-label="LifeLens world observer" />
      <canvas id="characterCanvas" aria-label="LifeLens residents" />
      <div className="world-overlay">
        <span id="seedLabel">WorldSeed —</span>
        <span id="timeLabel">Day —</span>
        <span id="chunkLabel">Chunk 0, 0</span>
        <span id="biomeLabel">Biome —</span>
      </div>
      <div id="errorCard" className="error-card hidden">
        <strong>LifeLensCore 연결 실패</strong>
        <span id="errorText">실제 Core WASM을 불러오지 못했습니다.</span>
      </div>
    </section>
  );
}

function ObserverPanel() {
  return (
    <aside className="observer">
      <section className="panel">
        <h2>World</h2>
        <label className="field">
          WorldSeed
          <input id="seedInput" inputMode="numeric" defaultValue="42" autoComplete="off" />
        </label>
        <p id="seedError" className="field-error hidden">WorldSeed를 입력해 주세요.</p>
        <div className="button-row">
          <button id="newWorld">NEW WORLD</button>
          <button id="step10" disabled>+10 MIN</button>
          <button id="step60" disabled>+1 HOUR</button>
        </div>
      </section>

      <section className="panel">
        <h2>Observer</h2>
        <div className="metrics">
          <div><span>Living</span><b id="living">—</b></div>
          <div><span>Households</span><b id="households">—</b></div>
          <div><span>Couples</span><b id="couples">—</b></div>
          <div><span>Life events</span><b id="events">—</b></div>
        </div>
      </section>

      <section className="panel">
        <div className="panel-heading"><h2>Residents</h2><span id="residentCount">0</span></div>
        <div id="residentList" className="resident-list"><div className="empty">Core 연결 중</div></div>
      </section>

      <section className="panel">
        <h2>World truth</h2>
        <div className="chunk-controls">
          <button id="left" disabled>←</button>
          <button id="up" disabled>↑</button>
          <button id="down" disabled>↓</button>
          <button id="right" disabled>→</button>
        </div>
        <p className="hint">드래그: 시점 회전 · 휠/핀치: 확대/축소 · 화살표: 관찰 Chunk 이동</p>
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
