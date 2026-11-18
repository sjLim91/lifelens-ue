import { useEffect, useRef, useState } from 'react';
import type {
  CivilizationWorldPayload,
  Resident,
  TerrainWindow,
} from '../runtime/core-types';
import {
  illustratedRegionHit,
  illustratedRegionProjection,
  paintIllustratedRegion,
  type IllustratedRegionView,
} from '../render/illustrated-region-map';
import './region-map-preview.css';

interface RegionMapPreviewProps {
  terrain: TerrainWindow | null;
  civilization: CivilizationWorldPayload;
  residents: readonly Resident[];
  centerChunkX: number;
  centerChunkY: number;
  onClose: () => void;
  onFocusChunk: (x: number, y: number) => void;
}

interface PointerDrag {
  x: number;
  y: number;
  centerX: number;
  centerY: number;
  moved: boolean;
}

export function RegionMapPreview({
  terrain, civilization, residents, centerChunkX, centerChunkY, onClose, onFocusChunk,
}: RegionMapPreviewProps) {
  const stageRef = useRef<HTMLDivElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const pointerRef = useRef<PointerDrag | null>(null);
  const [zoom, setZoom] = useState(1);
  const [center, setCenter] = useState({ x: centerChunkX, y: centerChunkY });
  const [size, setSize] = useState({ width: 0, height: 0 });

  useEffect(() => {
    const stage = stageRef.current;
    if (!stage) return;
    const resize = () => {
      const rect = stage.getBoundingClientRect();
      setSize({ width: Math.max(1, rect.width), height: Math.max(1, rect.height) });
    };
    const observer = new ResizeObserver(resize);
    observer.observe(stage);
    resize();
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    const onKey = (event: KeyboardEvent) => {
      if (event.key === 'Escape') onClose();
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [onClose]);

  const view: IllustratedRegionView = {
    width: size.width,
    height: size.height,
    centerX: center.x,
    centerY: center.y,
    zoom,
  };

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !size.width || !size.height) return;
    const context = canvas.getContext('2d');
    if (!context) return;
    const pixelRatio = Math.min(1.5, window.devicePixelRatio || 1);
    canvas.width = Math.max(1, Math.round(size.width * pixelRatio));
    canvas.height = Math.max(1, Math.round(size.height * pixelRatio));
    context.setTransform(pixelRatio, 0, 0, pixelRatio, 0, 0);
    paintIllustratedRegion(context, { terrain, civilization, residents, view });
  }, [terrain, civilization, residents, center.x, center.y, zoom, size.width, size.height]);

  function canvasPosition(event: React.PointerEvent<HTMLCanvasElement>) {
    const rect = event.currentTarget.getBoundingClientRect();
    return { x: event.clientX - rect.left, y: event.clientY - rect.top };
  }

  const beginDrag = (event: React.PointerEvent<HTMLCanvasElement>) => {
    if (event.button !== 0) return;
    const p = canvasPosition(event);
    pointerRef.current = { x: p.x, y: p.y, centerX: center.x, centerY: center.y, moved: false };
    event.currentTarget.setPointerCapture(event.pointerId);
  };

  const moveDrag = (event: React.PointerEvent<HTMLCanvasElement>) => {
    const drag = pointerRef.current;
    if (!drag) return;
    const p = canvasPosition(event);
    const dx = p.x - drag.x, dy = p.y - drag.y;
    if (Math.hypot(dx, dy) > 7) drag.moved = true;
    if (!drag.moved) return;
    const projection = illustratedRegionProjection(terrain, view);
    if (!projection) return;
    setCenter({
      x: drag.centerX - dx / projection.tile,
      y: drag.centerY - dy / projection.tile,
    });
  };

  const endDrag = (event: React.PointerEvent<HTMLCanvasElement>) => {
    const drag = pointerRef.current;
    if (!drag) return;
    pointerRef.current = null;
    if (event.currentTarget.hasPointerCapture(event.pointerId)) {
      event.currentTarget.releasePointerCapture(event.pointerId);
    }
    if (drag.moved) return;
    const p = canvasPosition(event);
    const hit = illustratedRegionHit(terrain, view, p.x, p.y);
    if (hit) onFocusChunk(hit.x, hit.y);
  };

  return (
    <section className="illustrated-map-backdrop" role="dialog" aria-modal="true" aria-labelledby="illustrated-map-title">
      <header className="illustrated-map-header">
        <div>
          <h2 id="illustrated-map-title">지역 지도 <span>시각화 실험</span></h2>
          <p>현재 관찰 가능한 Core 지형 · 지도 위 지점을 누르면 현장으로 이동</p>
        </div>
        <button type="button" onClick={onClose} aria-label="지역 지도 닫기">닫기</button>
      </header>
      <div className="illustrated-map-stage" ref={stageRef}>
        <canvas
          ref={canvasRef}
          aria-label="Core 관측 지형 기반 지역 지도. 드래그하여 이동, 클릭하여 해당 지역 관찰"
          onPointerDown={beginDrag}
          onPointerMove={moveDrag}
          onPointerUp={endDrag}
          onPointerCancel={() => { pointerRef.current = null; }}
          onWheel={event => {
            event.preventDefault();
            setZoom(previous => Math.max(1, Math.min(4, previous * (event.deltaY > 0 ? .85 : 1.15))));
          }}
        />
        <div className="illustrated-map-legend" aria-label="지도 범례">
          <span><i className="forest" /> 숲</span>
          <span><i className="water" /> 물</span>
          <span><i className="rock" /> 암석 지형</span>
          <span><i className="resident" /> 주민</span>
          <span><i className="settlement" /> 정착지</span>
        </div>
        <div className="illustrated-map-controls">
          <button type="button" onClick={() => setZoom(value => Math.max(1, +(value / 1.4).toFixed(2)))} disabled={zoom <= 1} aria-label="지도 축소">−</button>
          <button type="button" onClick={() => setZoom(value => Math.min(4, +(value * 1.4).toFixed(2)))} disabled={zoom >= 4} aria-label="지도 확대">+</button>
          <button type="button" onClick={() => { setZoom(1); setCenter({ x: centerChunkX, y: centerChunkY }); }}>현재 위치</button>
        </div>
      </div>
      <footer className="illustrated-map-footer">
        <span>관측 청크 {terrain?.available ? terrain.chunks.length : 0}개 · 시각 요소는 실제 지형 수치의 표현이며 개별 나무/자원 위치를 뜻하지 않습니다.</span>
        <span>세계 시뮬레이션과 저장 데이터는 변경되지 않습니다.</span>
      </footer>
    </section>
  );
}
