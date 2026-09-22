export type RenderMode = 'legacy-canvas' | 'three-world';

const STORAGE_KEY = 'lifelens.web.renderMode';

export function readRenderMode(): RenderMode {
  const value = globalThis.localStorage?.getItem(STORAGE_KEY);
  return value === 'three-world' ? 'three-world' : 'legacy-canvas';
}

export function writeRenderMode(mode: RenderMode): void {
  globalThis.localStorage?.setItem(STORAGE_KEY, mode);
}
