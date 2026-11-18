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
        <b>{current === 'field-canvas' ? '현장 2.5D' : current === 'isometric-canvas'
          ? '지역 지도 2.5D' : current === 'three-world' ? '기존 3차원' : '비상 캔버스'}</b>
      </div>
      <div className="button-row">
        <button onClick={() => switchMode('field-canvas')}
          disabled={current === 'field-canvas'}>
          현장 2.5D
        </button>
        <button onClick={() => switchMode('isometric-canvas')}
          disabled={current === 'isometric-canvas'}>
          지역 지도
        </button>
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
        현장 2.5D와 지역 지도는 같은 Core를 관찰하는 별도 화면입니다. 비상 캔버스는 렌더러 오류 시 Core를 보존하기 위한 대체 경로입니다.
      </p>
    </section>
  );
}
