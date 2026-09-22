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
        Three World가 현재 기본 통합 렌더러입니다. Legacy는 WebGL/통합 렌더러 실패 시를 위한 비상 표현 경로입니다.
      </p>
    </section>
  );
}
