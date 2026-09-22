export type RenderMode = 'legacy-canvas' | 'three-world';

const STORAGE_KEY = 'lifelens.web.renderMode';

export function readRenderMode(): RenderMode {
  const value = globalThis.localStorage?.getItem(STORAGE_KEY);
  if (value === 'legacy-canvas') return 'legacy-canvas';
  return 'three-world';
}

export function writeRenderMode(mode: RenderMode): void {
  globalThis.localStorage?.setItem(STORAGE_KEY, mode);
}
