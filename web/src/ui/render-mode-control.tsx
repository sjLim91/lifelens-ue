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
      <h2>개발용 렌더러</h2>
      <div>
        <span>현재 방식</span>
        <b>{current === 'three-world' ? '통합 3차원' : '비상 캔버스'}</b>
      </div>
      <div className="button-row">
        <button
          onClick={() => switchMode('legacy-canvas')}
          disabled={current === 'legacy-canvas'}
        >
          비상 캔버스
        </button>
        <button
          onClick={() => switchMode('three-world')}
          disabled={current === 'three-world'}
        >
          통합 3차원
        </button>
      </div>
      <p className="hint">
        통합 3차원이 현재 기본 렌더러입니다. 비상 캔버스는 웹 그래픽 또는 통합 렌더러 실패 시에만 사용하는 대체 표현 경로입니다.
      </p>
    </section>
  );
}
