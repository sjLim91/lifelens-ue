
export const FIELD_SPRITES = [
  'pine','cedar','oak','bush','cliff','rock','flowers',
  'citizen_f','citizen_m','deer','hare','hut','fire',
] as const;
export type FieldSpriteName = typeof FIELD_SPRITES[number];
export interface FieldSpritePainter {
  /** Coordinates specify an image rectangle, not Core/object positions. */
  draw(ctx: CanvasRenderingContext2D, name: FieldSpriteName,
    x: number, y: number, width: number, height: number): boolean;
}

/** Small original SVG artwork; one Image instance per reusable sprite, not per tree. */
export class FieldSpriteAtlas implements FieldSpritePainter {
  private readonly images = new Map<FieldSpriteName, HTMLImageElement>();
  private started = false;
  private disposed = false;

  constructor(private readonly onReady: () => void) {}

  warm(): void {
    if (this.started || this.disposed || typeof Image === 'undefined'
      || typeof document === 'undefined') return;
    this.started = true;
    for (const name of FIELD_SPRITES) {
      const image = new Image();
      this.images.set(name, image);
      image.onload = () => { if (!this.disposed) this.onReady(); };
      image.onerror = () => { if (!this.disposed) console.warn('LifeLens field sprite unavailable:', name); };
      // Vite base './' also works on GitHub Pages /lifelens-ue/ and locally.
      image.src = new URL('field/' + name + '.svg', document.baseURI).toString();
    }
  }

  draw(ctx: CanvasRenderingContext2D, name: FieldSpriteName,
    x: number, y: number, width: number, height: number): boolean {
    this.warm();
    const image = this.images.get(name);
    if (!image || !image.complete || image.naturalWidth <= 0) return false;
    ctx.drawImage(image, x, y, width, height);
    return true;
  }

  dispose(): void {
    this.disposed = true;
    for (const image of this.images.values()) { image.onload = null; image.onerror = null; }
    this.images.clear();
  }
}
