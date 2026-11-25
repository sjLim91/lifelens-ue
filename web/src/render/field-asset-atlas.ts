
export const FIELD_SPRITES = [
  'pine','cedar','oak','bush','cliff','rock','flowers',
  'citizen_f','citizen_m','deer','hare','hut','fire',
  'workbench','store','sleeping','furnace','farm','site','flag',
] as const;
export type FieldSpriteName = typeof FIELD_SPRITES[number];
// Pixel bounds of the painted subject in each padded WebP.
const artworkBounds:Record<FieldSpriteName,readonly [number,number,number,number]>={
  bush:[138,176,204,191],
  cedar:[152,290,176,283],
  citizen_f:[127,119,185,333],
  citizen_m:[140,104,160,347],
  cliff:[132,291,255,251],
  deer:[151,86,218,312],
  farm:[120,154,259,205],
  fire:[138,199,204,179],
  flag:[158,128,130,255],
  flowers:[135,167,210,199],
  furnace:[120,211,230,200],
  hare:[158,173,136,189],
  hut:[105,231,282,225],
  oak:[104,283,270,266],
  pine:[112,315,254,258],
  rock:[140,160,239,200],
  site:[93,182,283,208],
  sleeping:[130,173,225,162],
  store:[119,208,261,219],
  workbench:[128,180,245,200],
};
export function fieldArtworkWidth(name:FieldSpriteName,height:number):number {
  const [, , width, sourceHeight]=artworkBounds[name];
  return height*width/sourceHeight;
}
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
    if(image.src.endsWith('.webp')){
      const [sx,sy,sw,sh]=artworkBounds[name];
      ctx.drawImage(variant??image,sx,sy,sw,sh,x,y,width,height);
    }else{
      ctx.drawImage(variant??image,x,y,width,height);
    }
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
