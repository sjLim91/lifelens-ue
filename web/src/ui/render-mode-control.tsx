import {
  readRenderMode,
  writeRenderMode,
  type RenderMode,
} from '../render/render-mode';

export function RenderModeControl() {
  if (!import.meta.env.DEV) return null;

  const current = readRenderMode();

  const switchMode = (mode: RenderMode): void => {
    if (mode === current) return;
    writeRenderMode(mode);
    window.location.reload();
  };

  return (
    <section className="panel runtime">
      <h2>Development renderer</h2>
      <div>
        <span>Current</span>
        <b>{current}</b>
      </div>
      <div className="button-row">
        <button
          onClick={() => switchMode('legacy-canvas')}
          disabled={current === 'legacy-canvas'}
        >
          Legacy
        </button>
        <button
          onClick={() => switchMode('three-world')}
          disabled={current === 'three-world'}
        >
          Three World
        </button>
      </div>
      <p className="hint">
        Three World는 지형·물·식생·실제 GLB 주민 통합 검증용입니다. 운영 기본값은 여전히 Legacy입니다.
      </p>
    </section>
  );
}
