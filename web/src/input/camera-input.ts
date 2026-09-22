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
}

export class CameraInput {
  private angle: number;
  private zoom: number;
  private readonly minZoom: number;
  private readonly maxZoom: number;
  private readonly pointers = new Map<number, { x: number; y: number }>();
  private drag: { x: number; y: number } | null = null;
  private pinchDistance: number | null = null;

  constructor(
    private readonly canvas: HTMLCanvasElement,
    private readonly options: CameraInputOptions,
  ) {
    this.angle = options.initialAngle;
    this.zoom = options.initialZoom;
    this.minZoom = options.minZoom ?? 0.55;
    this.maxZoom = options.maxZoom ?? 2.7;

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

  private emit(): void {
    this.options.onChange(this.snapshot());
  }

  private readonly onPointerDown = (event: PointerEvent): void => {
    this.canvas.setPointerCapture(event.pointerId);
    this.pointers.set(event.pointerId, {
      x: event.clientX,
      y: event.clientY,
    });

    if (this.pointers.size === 1) {
      this.drag = { x: event.clientX, y: event.clientY };
    } else {
      this.drag = null;
      this.pinchDistance = this.pointerDistance();
    }
  };

  private readonly onPointerMove = (event: PointerEvent): void => {
    if (!this.pointers.has(event.pointerId)) return;

    this.pointers.set(event.pointerId, {
      x: event.clientX,
      y: event.clientY,
    });

    if (this.pointers.size >= 2) {
      const next = this.pointerDistance();
      if (next && this.pinchDistance) {
        this.zoom = Math.max(
          this.minZoom,
          Math.min(this.maxZoom, this.zoom * (next / this.pinchDistance)),
        );
        this.emit();
      }
      this.pinchDistance = next;
      return;
    }

    if (!this.drag) return;
    this.angle += (event.clientX - this.drag.x) * 0.008;
    this.drag = { x: event.clientX, y: event.clientY };
    this.emit();
  };

  private readonly onPointerStop = (event: PointerEvent): void => {
    this.pointers.delete(event.pointerId);
    this.pinchDistance = this.pointerDistance();
    const remaining = [...this.pointers.values()];
    this.drag = remaining.length === 1 ? { ...remaining[0] } : null;
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
