interface Point { x: number; y: number; }
interface GlobeInputOptions {
  rotate(dx: number, dy: number): void;
  zoom(delta: number): void;
  select(x: number, y: number): void;
}
/** Globe-only gestures. Pointer transitions never become a click or a zoom jump. */
export class GlobeInput {
  private readonly pointers = new Map<number, Point>();
  private anchor: Point | null = null;
  private last: Point | null = null;
  private pinch = 0;
  private moved = false;
  constructor(private readonly canvas: HTMLCanvasElement, private readonly options: GlobeInputOptions) {
    canvas.style.touchAction = 'none';
    canvas.addEventListener('pointerdown', this.down);
    canvas.addEventListener('pointermove', this.move);
    canvas.addEventListener('pointerup', this.up);
    canvas.addEventListener('pointercancel', this.cancel);
    canvas.addEventListener('lostpointercapture', this.cancel);
    canvas.addEventListener('wheel', this.wheel, { passive: false });
    window.addEventListener('blur', this.clear);
    document.addEventListener('visibilitychange', this.visibility);
  }
  private distance(): number {
    const points = [...this.pointers.values()];
    const n = points.length === 2 ? Math.hypot(points[0].x - points[1].x, points[0].y - points[1].y) : 0;
    return Number.isFinite(n) ? n : 0;
  }
  private rebase(): void {
    this.pinch = this.distance();
    this.last = this.pointers.size === 1 ? [...this.pointers.values()][0] : null;
  }
  private readonly down = (e: PointerEvent): void => {
    if (!Number.isFinite(e.clientX) || !Number.isFinite(e.clientY) || this.pointers.has(e.pointerId)) return;
    if (e.pointerType === 'mouse' && e.button !== 0) return;
    const point = { x: e.clientX, y: e.clientY };
    this.pointers.set(e.pointerId, point);
    try { this.canvas.setPointerCapture(e.pointerId); } catch { /* capture can end during OS gestures */ }
    if (this.pointers.size === 1) { this.anchor = point; this.moved = false; }
    else this.moved = true;
    this.rebase();
  };
  private readonly move = (e: PointerEvent): void => {
    if (!this.pointers.has(e.pointerId) || !Number.isFinite(e.clientX) || !Number.isFinite(e.clientY)) return;
    const point = { x: e.clientX, y: e.clientY };
    this.pointers.set(e.pointerId, point);
    if (this.pointers.size === 2) {
      const next = this.distance();
      if (next > 0 && this.pinch > 0) this.options.zoom((this.pinch - next) * .009);
      this.pinch = next; this.moved = true;
    } else if (this.pointers.size === 1 && this.last) {
      if (this.anchor && Math.hypot(point.x - this.anchor.x, point.y - this.anchor.y) > 7) this.moved = true;
      this.options.rotate(point.x - this.last.x, point.y - this.last.y);
      this.last = point;
    }
  };
  private readonly up = (e: PointerEvent): void => {
    if (!this.pointers.has(e.pointerId)) return;
    const select = this.pointers.size === 1 && !this.moved && this.anchor !== null;
    this.pointers.delete(e.pointerId);
    if (this.pointers.size) this.moved = true;
    this.rebase();
    if (select && Number.isFinite(e.clientX) && Number.isFinite(e.clientY)) this.options.select(e.clientX, e.clientY);
    if (!this.pointers.size) this.anchor = null;
  };
  private readonly cancel = (e: PointerEvent): void => {
    if (!this.pointers.delete(e.pointerId)) return;
    this.moved = true; this.anchor = null; this.rebase();
  };
  private readonly wheel = (e: WheelEvent): void => {
    e.preventDefault();
    const scale = e.deltaMode === 1 ? 16 : e.deltaMode === 2 ? this.canvas.clientHeight : 1;
    if (Number.isFinite(e.deltaY)) this.options.zoom(e.deltaY * scale * .0035);
  };
  private readonly visibility = (): void => { if (document.hidden) this.clear(); };
  private readonly clear = (): void => {
    const ids = [...this.pointers.keys()];
    this.pointers.clear(); this.anchor = this.last = null; this.pinch = 0; this.moved = true;
    for (const id of ids) try { if (this.canvas.hasPointerCapture(id)) this.canvas.releasePointerCapture(id); } catch { /* already released */ }
  };
  dispose(): void {
    this.clear();
    this.canvas.removeEventListener('pointerdown', this.down);
    this.canvas.removeEventListener('pointermove', this.move);
    this.canvas.removeEventListener('pointerup', this.up);
    this.canvas.removeEventListener('pointercancel', this.cancel);
    this.canvas.removeEventListener('lostpointercapture', this.cancel);
    this.canvas.removeEventListener('wheel', this.wheel);
    window.removeEventListener('blur', this.clear);
    document.removeEventListener('visibilitychange', this.visibility);
  }
}
