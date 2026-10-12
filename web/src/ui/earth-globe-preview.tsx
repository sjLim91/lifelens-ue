import { useEffect, useRef, useState } from 'react';
import * as THREE from 'three';
import {
  paintNaturalEarthTexture,
  type EarthLandFeatureCollection,
} from '../earth/natural-earth-land';
import { geoFromGlobeUV, formatPreviewGeoPoint, type EarthPreviewGeoPoint } from '../earth/globe-region-picking';

export function EarthGlobePreview({ onClose }: { onClose: () => void }) {
  const holder = useRef<HTMLDivElement>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [activeSlot, setActiveSlot] = useState<'A' | 'B'>('A');
  const activeSlotRef = useRef<'A' | 'B'>('A');
  const [selectedSites, setSelectedSites] = useState<{
    A: EarthPreviewGeoPoint | null;
    B: EarthPreviewGeoPoint | null;
  }>({ A: null, B: null });

  const selectSlot = (slot: 'A' | 'B') => {
    activeSlotRef.current = slot;
    setActiveSlot(slot);
  };

  useEffect(() => {
    const closeOnEscape = (event: KeyboardEvent) => {
      if (event.key === 'Escape') onClose();
    };
    window.addEventListener('keydown', closeOnEscape);
    return () => window.removeEventListener('keydown', closeOnEscape);
  }, [onClose]);

  useEffect(() => {
    let aborted = false;
    let cleanup = () => {};
    const start = async () => {
      const response = await fetch(`${import.meta.env.BASE_URL}earth/ne_110m_land.geojson`);
      if (!response.ok) throw new Error('지구 자연지리 자료를 읽지 못했습니다.');
      const land = await response.json() as EarthLandFeatureCollection;
      if (aborted || !holder.current) return;

      const textureCanvas = document.createElement('canvas');
      textureCanvas.width = 1024;
      textureCanvas.height = 512;
      const context = textureCanvas.getContext('2d');
      if (!context) throw new Error('지구본 텍스처를 만들 수 없습니다.');
      paintNaturalEarthTexture(context, land, 1024, 512);

      const canvas = document.createElement('canvas');
      canvas.setAttribute('aria-label', '회전 가능한 자연지리 지구본 미리보기');
      canvas.style.width = '100%';
      canvas.style.height = '100%';
      canvas.style.touchAction = 'none';
      const renderer = new THREE.WebGLRenderer({
        canvas, antialias: !window.matchMedia('(pointer: coarse)').matches,
        alpha: false, powerPreference: 'low-power',
      });
      renderer.setPixelRatio(Math.min(1.5, window.devicePixelRatio || 1));
      const scene = new THREE.Scene();
      scene.background = new THREE.Color(0x07111f);
      const camera = new THREE.PerspectiveCamera(38, 1, 0.1, 20);
      camera.position.z = 4.1;
      scene.add(new THREE.HemisphereLight(0xc6eaff, 0x101b25, 2.5));
      const light = new THREE.DirectionalLight(0xffffff, 2.2);
      light.position.set(3, 2, 5);
      scene.add(light);
      const texture = new THREE.CanvasTexture(textureCanvas);
      texture.colorSpace = THREE.SRGBColorSpace;
      texture.anisotropy = 2;
      const material = new THREE.MeshStandardMaterial({
        map: texture, roughness: 1, metalness: 0,
      });
      const geometry = new THREE.SphereGeometry(1.35, 64, 40);
      const globe = new THREE.Mesh(geometry, material);
      globe.rotation.y = -0.75;
      globe.rotation.x = 0.1;
      scene.add(globe);
      // Preview pins travel with the globe; they are NOT Core residents.
      const siteMarkerGeometry = new THREE.SphereGeometry(0.037, 14, 10);
      const siteMarkerMaterials = {
        A: new THREE.MeshBasicMaterial({ color: 0xf4ca79, depthTest: false }),
        B: new THREE.MeshBasicMaterial({ color: 0x76d8d0, depthTest: false }),
      };
      const siteMarkers = {
        A: new THREE.Mesh(siteMarkerGeometry, siteMarkerMaterials.A),
        B: new THREE.Mesh(siteMarkerGeometry, siteMarkerMaterials.B),
      };
      siteMarkers.A.visible = false;
      siteMarkers.B.visible = false;
      globe.add(siteMarkers.A, siteMarkers.B);
      // A faint outer atmosphere is presentation-only.
      const shellGeometry = new THREE.SphereGeometry(1.37, 48, 32);
      const shellMaterial = new THREE.MeshBasicMaterial({
        color: 0x4e83be, transparent: true, opacity: 0.08,
        depthWrite: false, side: THREE.BackSide,
      });
      const shell = new THREE.Mesh(shellGeometry, shellMaterial);
      scene.add(shell);

      const area = holder.current;
      area.appendChild(canvas);
      const resize = () => {
        const width = Math.max(1, area.clientWidth);
        const height = Math.max(1, area.clientHeight);
        renderer.setSize(width, height, false);
        camera.aspect = width / height;
        camera.updateProjectionMatrix();
      };
      const observer = new ResizeObserver(resize);
      observer.observe(area);
      resize();

      let dragging = false;
      let didDrag = false;
      let startX = 0, startY = 0;
      let lastX = 0, lastY = 0;
      const picker = new THREE.Raycaster();
      const pointer = new THREE.Vector2();
      const down = (event: PointerEvent) => {
        dragging = true;
        didDrag = false;
        startX = event.clientX;
        startY = event.clientY;
        lastX = event.clientX;
        lastY = event.clientY;
        canvas.setPointerCapture(event.pointerId);
      };
      const move = (event: PointerEvent) => {
        if (!dragging) return;
        if (Math.hypot(event.clientX - startX, event.clientY - startY) > 7) {
          didDrag = true;
        }
        globe.rotation.y += (event.clientX - lastX) * 0.006;
        globe.rotation.x = THREE.MathUtils.clamp(
          globe.rotation.x + (event.clientY - lastY) * 0.004,
          -Math.PI / 2, Math.PI / 2,
        );
        lastX = event.clientX;
        lastY = event.clientY;
      };
      const end = (event: PointerEvent) => {
        if (!dragging) return;
        dragging = false;
        if (didDrag) return;
        const bounds = canvas.getBoundingClientRect();
        if (bounds.width <= 0 || bounds.height <= 0) return;
        pointer.set(
          ((event.clientX - bounds.left) / bounds.width) * 2 - 1,
          -((event.clientY - bounds.top) / bounds.height) * 2 + 1,
        );
        globe.updateMatrixWorld(true);
        picker.setFromCamera(pointer, camera);
        const hit = picker.intersectObject(globe, false)[0];
        if (!hit?.uv) return;
        const location = geoFromGlobeUV(hit.uv.x, hit.uv.y);
        if (!location) return;
        const slot = activeSlotRef.current;
        const marker = siteMarkers[slot];
        marker.position.copy(globe.worldToLocal(hit.point.clone()))
          .normalize().multiplyScalar(1.385);
        marker.visible = true;
        setSelectedSites((current) => ({ ...current, [slot]: location }));
      };
      const cancel = () => { dragging = false; didDrag = true; };
      const wheel = (event: WheelEvent) => {
        event.preventDefault();
        camera.position.z = THREE.MathUtils.clamp(
          camera.position.z + event.deltaY * 0.0035, 2.5, 6.5,
        );
      };
      canvas.addEventListener('pointerdown', down);
      canvas.addEventListener('pointermove', move);
      canvas.addEventListener('pointerup', end);
      canvas.addEventListener('pointercancel', cancel);
      canvas.addEventListener('wheel', wheel, { passive: false });

      let frame = 0, lastFrame = 0;
      const frameBudget = window.matchMedia('(pointer: coarse)').matches ? 40 : 24;
      const tick = (time: number) => {
        frame = window.requestAnimationFrame(tick);
        if (time - lastFrame < frameBudget) return;
        const delta = Math.min(time - lastFrame, 60);
        lastFrame = time;
        if (!dragging) globe.rotation.y += delta * 0.000055;
        renderer.render(scene, camera);
      };
      frame = window.requestAnimationFrame(tick);
      cleanup = () => {
        window.cancelAnimationFrame(frame);
        observer.disconnect();
        canvas.removeEventListener('pointerdown', down);
        canvas.removeEventListener('pointermove', move);
        canvas.removeEventListener('pointerup', end);
        canvas.removeEventListener('pointercancel', cancel);
        canvas.removeEventListener('wheel', wheel);
        siteMarkerGeometry.dispose();
        siteMarkerMaterials.A.dispose();
        siteMarkerMaterials.B.dispose();
        geometry.dispose();
        material.dispose();
        texture.dispose();
        shellGeometry.dispose();
        shellMaterial.dispose();
        renderer.dispose();
        renderer.forceContextLoss();
        canvas.remove();
      };
      if (aborted) cleanup();
      else setLoading(false);
    };
    void start().catch((reason: unknown) => {
      if (!aborted) {
        setLoading(false);
        setError(reason instanceof Error ? reason.message : '지구본을 표시할 수 없습니다.');
      }
    });
    return () => {
      aborted = true;
      cleanup();
    };
  }, []);

  return (
    <div
      className="earth-globe-backdrop"
      role="presentation"
      onMouseDown={(event) => {
        if (event.target === event.currentTarget) onClose();
      }}
    >
      <section className="earth-globe-dialog" role="dialog" aria-modal="true"
        aria-label="지구 자연지리 관찰 미리보기">
        <header className="earth-globe-header">
          <div><strong>지구 자연지리 관찰</strong><span>EARTH-2 · 구체 화면 기반</span></div>
          <button type="button" aria-label="지구본 닫기" onClick={onClose}>×</button>
        </header>
        <div ref={holder} className="earth-globe-stage">
          {loading && <span className="earth-globe-message">자연지리 지구본 준비 중…</span>}
          {error && <span className="earth-globe-message" role="alert">{error}</span>}
        </div>
        <div className="earth-globe-footer">
          <div className="earth-globe-sites" aria-label="지구 지역 후보 좌표 선택">
            {(['A', 'B'] as const).map((slot) => (
              <button key={slot} type="button"
                className={activeSlot === slot ? 'active' : ''}
                aria-pressed={activeSlot === slot}
                onClick={() => selectSlot(slot)}>
                <b>지역 {slot}</b>
                <span>{selectedSites[slot]
                  ? formatPreviewGeoPoint(selectedSites[slot]!)
                  : '좌표 미선택'}</span>
              </button>
            ))}
          </div>
          <p>지역 A 또는 B를 고른 뒤 지구본을 짧게 터치하면 위경도가 표시됩니다.
            드래그로 회전 · 마우스 휠로 확대</p>
          <p><b>선택한 좌표는 관찰용 후보입니다.</b> 주민이 이곳에 생성되거나,
            현재 Core 세계의 위치·시드·생활환경이 바뀌지는 않습니다.</p>
          <p><b>자연지리 미리보기:</b> 실제 대륙·해안 윤곽 자료를 보여줍니다.
            기존 주민 시뮬레이션의 위치·날씨·바람·해류와는 아직 연결되지 않았습니다.
            현재 바람·해류 실황을 표시하는 화면이 아닙니다.</p>
          <small>Made with Natural Earth · 1:110m land · Public Domain</small>
        </div>
      </section>
    </div>
  );
}
