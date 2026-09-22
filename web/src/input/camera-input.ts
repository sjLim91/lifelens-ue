import { OBSERVER_CAMERA_CONTRACT } from '../runtime/lifelens-contract';

export interface CameraInputState {
  angle: number;
  elevation: number;
  zoom: number;
}

export interface CameraInputOptions {
  initialAngle: number;
  initialElevation: number;
  initialZoom: number;
  minZoom?: number;
  maxZoom?: number;
  minElevation?: number;
  maxElevation?: number;
  rotateSensitivity?: number;
  onChange: (state: CameraInputState) => void;
  onPan?: (deltaX: number, deltaY: number) => void;
  onTap?: (clientX: number, clientY: number) => void;
}

type DragMode = 'select' | 'orbit' | 'pan' | null;

interface PointerPoint {
  x: number;
  y: number;
  pointerType: string;
}

export class CameraInput {
  private angle: number;
  private elevation: number;
  private zoom: number;
  private readonly minZoom: number;
  private readonly maxZoom: number;
  private readonly minElevation: number;
  private readonly maxElevation: number;
  private readonly rotateSensitivity: number;
  private readonly pointers = new Map<number, PointerPoint>();
  private drag: { x: number; y: number } | null = null;
  private dragMode: DragMode = null;
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
    this.elevation = options.initialElevation;
    this.zoom = options.initialZoom;
    this.minZoom = options.minZoom ?? OBSERVER_CAMERA_CONTRACT.minZoom;
    this.maxZoom = options.maxZoom ?? OBSERVER_CAMERA_CONTRACT.maxZoom;
    this.minElevation = options.minElevation ?? OBSERVER_CAMERA_CONTRACT.minElevationRadians;
    this.maxElevation = options.maxElevation ?? OBSERVER_CAMERA_CONTRACT.maxElevationRadians;
    this.rotateSensitivity = options.rotateSensitivity ?? OBSERVER_CAMERA_CONTRACT.rotateSensitivity;

    canvas.style.touchAction = 'none';
    canvas.addEventListener('contextmenu', this.onContextMenu);
    canvas.addEventListener('pointerdown', this.onPointerDown);
    canvas.addEventListener('pointermove', this.onPointerMove);
    canvas.addEventListener('pointerup', this.onPointerStop);
    canvas.addEventListener('pointercancel', this.onPointerStop);
    canvas.addEventListener('wheel', this.onWheel, { passive: false });
  }

  snapshot(): CameraInputState {
    return {
      angle: this.angle,
      elevation: this.elevation,
      zoom: this.zoom,
    };
  }

  dispose(): void {
    this.canvas.removeEventListener('contextmenu', this.onContextMenu);
    this.canvas.removeEventListener('pointerdown', this.onPointerDown);
    this.canvas.removeEventListener('pointermove', this.onPointerMove);
    this.canvas.removeEventListener('pointerup', this.onPointerStop);
    this.canvas.removeEventListener('pointercancel', this.onPointerStop);
    this.canvas.removeEventListener('wheel', this.onWheel);
  }

  private clampElevation(value: number): number {
    return Math.max(this.minElevation, Math.min(this.maxElevation, value));
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

  private orbit(deltaX: number, deltaY: number): void {
    this.angle += deltaX * this.rotateSensitivity;
    this.elevation = this.clampElevation(
      this.elevation - deltaY * this.rotateSensitivity,
    );
    this.emit();
  }

  private readonly onContextMenu = (event: MouseEvent): void => {
    event.preventDefault();
  };

  private readonly onPointerDown = (event: PointerEvent): void => {
    event.preventDefault();
    this.canvas.setPointerCapture(event.pointerId);
    this.pointers.set(event.pointerId, {
      x: event.clientX,
      y: event.clientY,
      pointerType: event.pointerType,
    });

    if (event.pointerType === 'touch') {
      if (this.pointers.size === 1) {
        // Canonical Android contract:
        // one-finger drag = orbit, short one-finger release = select.
        this.dragMode = 'orbit';
        this.drag = { x: event.clientX, y: event.clientY };
        this.tapCandidate = {
          pointerId: event.pointerId,
          startX: event.clientX,
          startY: event.clientY,
          moved: false,
        };
      } else {
        // Two-finger gesture owns pan + pinch and can never become a tap.
        this.dragMode = null;
        this.drag = null;
        this.tapCandidate = null;
        this.pinchDistance = this.pointerDistance();
        this.pinchCentroid = this.pointerCentroid();
      }
      return;
    }

    // Canonical PC contract:
    // left = selection, middle = pan, right = orbit.
    if (event.button === 2) {
      this.dragMode = 'orbit';
      this.drag = { x: event.clientX, y: event.clientY };
      this.tapCandidate = null;
    } else if (event.button === 1) {
      this.dragMode = 'pan';
      this.drag = { x: event.clientX, y: event.clientY };
      this.tapCandidate = null;
    } else {
      this.dragMode = 'select';
      this.drag = { x: event.clientX, y: event.clientY };
      this.tapCandidate = {
        pointerId: event.pointerId,
        startX: event.clientX,
        startY: event.clientY,
        moved: false,
      };
    }
  };

  private readonly onPointerMove = (event: PointerEvent): void => {
    if (!this.pointers.has(event.pointerId)) return;
    event.preventDefault();

    this.pointers.set(event.pointerId, {
      x: event.clientX,
      y: event.clientY,
      pointerType: event.pointerType,
    });

    if (
      this.tapCandidate?.pointerId === event.pointerId
      && Math.hypot(
        event.clientX - this.tapCandidate.startX,
        event.clientY - this.tapCandidate.startY,
      ) > OBSERVER_CAMERA_CONTRACT.tapMoveThresholdPx
    ) {
      this.tapCandidate.moved = true;
    }

    const touchPointers = [...this.pointers.values()]
      .filter((pointer) => pointer.pointerType === 'touch');

    if (touchPointers.length >= 2) {
      this.tapCandidate = null;
      const nextDistance = this.pointerDistance();
      const nextCentroid = this.pointerCentroid();

      if (nextDistance && this.pinchDistance) {
        this.zoom = Math.max(
          this.minZoom,
          Math.min(
            this.maxZoom,
            this.zoom * (nextDistance / this.pinchDistance),
          ),
        );
        this.emit();
      }

      if (nextCentroid && this.pinchCentroid) {
        const deltaX = nextCentroid.x - this.pinchCentroid.x;
        const deltaY = nextCentroid.y - this.pinchCentroid.y;
        if (Math.abs(deltaX) > OBSERVER_CAMERA_CONTRACT.pointerMotionEpsilonPx || Math.abs(deltaY) > OBSERVER_CAMERA_CONTRACT.pointerMotionEpsilonPx) {
          // Canonical Android contract: two fingers moving together pan.
          this.options.onPan?.(deltaX, deltaY);
        }
      }

      this.pinchDistance = nextDistance;
      this.pinchCentroid = nextCentroid;
      return;
    }

    if (!this.drag) return;
    const deltaX = event.clientX - this.drag.x;
    const deltaY = event.clientY - this.drag.y;
    this.drag = { x: event.clientX, y: event.clientY };

    if (Math.abs(deltaX) <= OBSERVER_CAMERA_CONTRACT.pointerMotionEpsilonPx && Math.abs(deltaY) <= OBSERVER_CAMERA_CONTRACT.pointerMotionEpsilonPx) return;

    if (this.dragMode === 'orbit') {
      this.orbit(deltaX, deltaY);
    } else if (this.dragMode === 'pan') {
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
    if (remaining.length === 1 && remaining[0].pointerType === 'touch') {
      this.dragMode = 'orbit';
      this.drag = { x: remaining[0].x, y: remaining[0].y };
    } else {
      this.dragMode = null;
      this.drag = null;
    }

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
        this.zoom * Math.exp(
          -event.deltaY * OBSERVER_CAMERA_CONTRACT.wheelZoomSensitivity,
        ),
      ),
    );
    this.emit();
  };
}
