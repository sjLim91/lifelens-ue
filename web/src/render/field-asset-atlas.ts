
export const FIELD_SPRITES = [
  'pine','cedar','oak','bush','cliff','rock','flowers',
  'citizen_f','citizen_m','deer','hare','hut','fire',
  'workbench','store','sleeping','furnace','farm','site','flag',
] as const;
export type FieldSpriteName = typeof FIELD_SPRITES[number];
export interface FieldSpritePainter {
  /** Coordinates specify an image rectangle, not Core/object positions. */
  draw(ctx: CanvasRenderingContext2D, name: FieldSpriteName,
    x: number, y: number, width: number, height: number,
    variation?: number): boolean;
}

/** Small original SVG artwork; one Image instance per reusable sprite, not per tree. */
export class FieldSpriteAtlas implements FieldSpritePainter {
  private readonly images = new Map<FieldSpriteName, HTMLImageElement>();
  // A tiny bounded set of pre-rasterized plant hue variants. No per-frame
  // Canvas filter compositing in mobile software-rendered observers.
  private readonly plantVariants = new Map<string, HTMLCanvasElement>();
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
      image.onload = () => {
        if (this.disposed) return;
        if (name==='pine'||name==='cedar'||name==='oak'||name==='bush') {
          this.preparePlantVariants(name,image);
        }
        this.onReady();
      };
      image.onerror = () => { if (!this.disposed) console.warn('LifeLens field sprite unavailable:', name); };
      // Vite base './' also works on GitHub Pages /lifelens-ue/ and locally.
      image.src = new URL('field/' + name + '.svg', document.baseURI).toString();
    }
  }

  private preparePlantVariants(name:FieldSpriteName,image:HTMLImageElement):void {
    if(typeof document==='undefined'||image.naturalWidth<=0||image.naturalHeight<=0)return;
    for(let index=0;index<3;index++){
      try {
        const tile=document.createElement('canvas');
        tile.width=image.naturalWidth;tile.height=image.naturalHeight;
        const painter=tile.getContext('2d');
        if(!painter)continue;
        const hue=(index-1)*11;
        painter.filter=`hue-rotate(${hue}deg) saturate(${1+(index-1)*.09}) brightness(${1+(index-1)*.04})`;
        painter.drawImage(image,0,0);
        this.plantVariants.set(name+':'+index,tile);
      } catch {
        // Keep the original same-origin SVG if Canvas2D filtering is unavailable.
      }
    }
  }

  draw(ctx: CanvasRenderingContext2D, name: FieldSpriteName,
    x: number, y: number, width: number, height: number,
    variation?: number): boolean {
    this.warm();
    const image = this.images.get(name);
    if (!image || !image.complete || image.naturalWidth <= 0) return false;
    const variant=variation===undefined?null:
      this.plantVariants.get(name+':'+Math.min(2,Math.max(0,
        Math.floor(variation*3))));
    ctx.drawImage(variant??image, x, y, width, height);
    return true;
  }

  dispose(): void {
    this.disposed = true;
    for (const image of this.images.values()) { image.onload = null; image.onerror = null; }
    this.images.clear();
    this.plantVariants.clear();
  }
}
