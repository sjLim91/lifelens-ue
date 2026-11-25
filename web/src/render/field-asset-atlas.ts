
export const FIELD_SPRITES = [
  'pine','cedar','oak','bush','cliff','rock','flowers',
  'citizen_f','citizen_m','deer','hare','hut','fire',
  'workbench','store','sleeping','furnace','farm','site','flag',
] as const;
export type FieldSpriteName = typeof FIELD_SPRITES[number];
export interface FieldSpritePainter {
  drawResidentFrame?(ctx:CanvasRenderingContext2D, female:boolean, wrapped:boolean,
    frame:number, width:number, height:number):boolean;
  /** Coordinates specify an image rectangle, not Core/object positions. */
  draw(ctx: CanvasRenderingContext2D, name: FieldSpriteName,
    x: number, y: number, width: number, height: number,
    variation?: number): boolean;
}

/** Small original field artwork; one Image instance per reusable sprite, not per tree. */
export class FieldSpriteAtlas implements FieldSpritePainter {
  private readonly images = new Map<FieldSpriteName, HTMLImageElement>();
  // A tiny bounded set of pre-rasterized plant hue variants. No per-frame
  // Canvas filter compositing in mobile software-rendered observers.
  private readonly plantVariants = new Map<string, HTMLCanvasElement>();
  private readonly motionSheets = new Map<string, HTMLImageElement>();
  private started = false;
  private disposed = false;

  constructor(private readonly onReady: () => void) {}

  warm(): void {
    if (this.started || this.disposed || typeof Image === 'undefined'
      || typeof document === 'undefined') return;
    this.started = true;
    for (const key of ['f-base','m-base','f-wrap','m-wrap']) {
      const sheet=new Image();
      sheet.onload=()=>{ if(!this.disposed)this.onReady(); };
      sheet.src=new URL('field/motion-'+key+'.webp',document.baseURI).toString();
      this.motionSheets.set(key,sheet);
    }
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
      image.onerror = () => {
        if (this.disposed) return;
        image.onerror = null;
        image.src = new URL('field/' + name + '.svg', document.baseURI).toString();
      };
      // Vite base './' also works on GitHub Pages /lifelens-ue/ and locally.
      image.src = new URL('field/' + name + '.webp', document.baseURI).toString();
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

  drawResidentFrame(ctx:CanvasRenderingContext2D, female:boolean, wrapped:boolean,
    frame:number, width:number, height:number):boolean {
    const sheet=this.motionSheets.get((female?'f':'m')+(wrapped?'-wrap':'-base'));
    if(!sheet?.complete || !sheet.naturalWidth)return false;
    const cell=sheet.naturalWidth/4;
    // The sole baseline is at row 186 of each 192-pixel animation cell.
    ctx.drawImage(sheet,(frame%4)*cell,Math.floor(frame/4)*cell,cell,cell,
      -width/2,-height*186/192,width,height);
    return true;
  }

  dispose(): void {
    for(const sheet of this.motionSheets.values())sheet.onload=null;
    this.motionSheets.clear();
    this.disposed = true;
    for (const image of this.images.values()) { image.onload = null; image.onerror = null; }
    this.images.clear();
    this.plantVariants.clear();
  }
}
