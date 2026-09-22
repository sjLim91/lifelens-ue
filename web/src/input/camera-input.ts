export interface CameraInputState {
  angle: number;
  zoom: number;
}

export interface CameraInputOptions {
  initialAngle: number;
  initialZoom: number;
  minZoom?: number;
  maxZoom?: number;
  onChange: (state: CameraInputState) => void;
  onPan?: (deltaX: number, deltaY: number) => void;
  onTap?: (clientX: number, clientY: number) => void;
}

export class CameraInput {
  private angle: number;
  private zoom: number;
  private readonly minZoom: number;
  private readonly maxZoom: number;
  private readonly pointers = new Map<number, { x: number; y: number }>();
  private drag: { x: number; y: number } | null = null;
  private pinchDistance: number | null = null;
  private pinchCentroid: { x: number; y: number } | null = null;
  private tapCandidate: {
    pointerId: number;
    startX: number;
    startY: number;
    moved: boolean;
  } | null = null;

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly options: CameraInputOptions,
  ) {
    this.angle = options.initialAngle;
    this.zoom = options.initialZoom;
    this.minZoom = options.minZoom ?? 0.55;
    this.maxZoom = options.maxZoom ?? 2.7;

    canvas.style.touchAction = 'none';
    canvas.addEventListener('pointerdown', this.onPointerDown);
    canvas.addEventListener('pointermove', this.onPointerMove);
    canvas.addEventListener('pointerup', this.onPointerStop);
    canvas.addEventListener('pointercancel', this.onPointerStop);
    canvas.addEventListener('wheel', this.onWheel, { passive: false });
  }

  snapshot(): CameraInputState {
    return { angle: this.angle, zoom: this.zoom };
  }

  dispose(): void {
    this.canvas.removeEventListener('pointerdown', this.onPointerDown);
    this.canvas.removeEventListener('pointermove', this.onPointerMove);
    this.canvas.removeEventListener('pointerup', this.onPointerStop);
    this.canvas.removeEventListener('pointercancel', this.onPointerStop);
    this.canvas.removeEventListener('wheel', this.onWheel);
  }

  private pointerDistance(): number | null {
    const values = [...this.pointers.values()];
    if (values.length < 2) return null;
    return Math.hypot(
      values[0].x - values[1].x,
      values[0].y - values[1].y,
    );
  }

  private pointerCentroid(): { x: number; y: number } | null {
    const values = [...this.pointers.values()];
    if (values.length < 2) return null;
    return {
      x: (values[0].x + values[1].x) * 0.5,
      y: (values[0].y + values[1].y) * 0.5,
    };
  }

  private emit(): void {
    this.options.onChange(this.snapshot());
  }

  private readonly onPointerDown = (event: PointerEvent): void => {
    event.preventDefault();
    this.canvas.setPointerCapture(event.pointerId);
    this.pointers.set(event.pointerId, {
      x: event.clientX,
      y: event.clientY,
    });

    if (this.pointers.size === 1) {
      this.drag = { x: event.clientX, y: event.clientY };
      this.tapCandidate = {
        pointerId: event.pointerId,
        startX: event.clientX,
        startY: event.clientY,
        moved: false,
      };
    } else {
      this.drag = null;
      this.tapCandidate = null;
      this.pinchDistance = this.pointerDistance();
      this.pinchCentroid = this.pointerCentroid();
    }
  };

  private readonly onPointerMove = (event: PointerEvent): void => {
    if (!this.pointers.has(event.pointerId)) return;
    event.preventDefault();

    this.pointers.set(event.pointerId, {
      x: event.clientX,
      y: event.clientY,
    });

    if (
      this.tapCandidate?.pointerId === event.pointerId
      && Math.hypot(
        event.clientX - this.tapCandidate.startX,
        event.clientY - this.tapCandidate.startY,
      ) > 8
    ) {
      this.tapCandidate.moved = true;
    }

    if (this.pointers.size >= 2) {
      this.tapCandidate = null;
      const nextDistance = this.pointerDistance();
      const nextCentroid = this.pointerCentroid();
      let changed = false;

      if (nextDistance && this.pinchDistance) {
        this.zoom = Math.max(
          this.minZoom,
          Math.min(
            this.maxZoom,
            this.zoom * (nextDistance / this.pinchDistance),
          ),
        );
        changed = true;
      }

      if (nextCentroid && this.pinchCentroid) {
        const centroidDx = nextCentroid.x - this.pinchCentroid.x;
        if (Math.abs(centroidDx) > 0.01) {
          this.angle += centroidDx * 0.006;
          changed = true;
        }
      }

      this.pinchDistance = nextDistance;
      this.pinchCentroid = nextCentroid;
      if (changed) this.emit();
      return;
    }

    if (!this.drag) return;
    const deltaX = event.clientX - this.drag.x;
    const deltaY = event.clientY - this.drag.y;
    this.drag = { x: event.clientX, y: event.clientY };

    if (Math.abs(deltaX) > 0.01 || Math.abs(deltaY) > 0.01) {
      this.options.onPan?.(deltaX, deltaY);
    }
  };

  private readonly onPointerStop = (event: PointerEvent): void => {
    event.preventDefault();
    const shouldTap = event.type === 'pointerup'
      && this.tapCandidate?.pointerId === event.pointerId
      && this.tapCandidate.moved === false
      && this.pointers.size === 1;

    this.pointers.delete(event.pointerId);
    this.pinchDistance = this.pointerDistance();
    this.pinchCentroid = this.pointerCentroid();
    const remaining = [...this.pointers.values()];
    this.drag = remaining.length === 1 ? { ...remaining[0] } : null;

    if (shouldTap) {
      this.options.onTap?.(event.clientX, event.clientY);
    }

    if (this.tapCandidate?.pointerId === event.pointerId) {
      this.tapCandidate = null;
    }
  };

  private readonly onWheel = (event: WheelEvent): void => {
    event.preventDefault();
    this.zoom = Math.max(
      this.minZoom,
      Math.min(
        this.maxZoom,
        this.zoom * Math.exp(-event.deltaY * 0.001),
      ),
    );
    this.emit();
  };
}
