/** The legacy and full-3D modes remain reachable during the staged migration. */
export type RenderMode = 'isometric-canvas' | 'legacy-canvas' | 'three-world';

const STORAGE_KEY = 'lifelens.web.renderMode';
export const ISOMETRIC_QUERY_KEY = 'view';

export function readRenderMode(): RenderMode {
  const search = typeof window === 'undefined' ? '' : window.location?.search ?? '';
  const requested = new URLSearchParams(search).get(ISOMETRIC_QUERY_KEY);
  if (requested === '2.5d') return 'isometric-canvas';
  if (requested === '3d') return 'three-world';
  if (requested === 'legacy') return 'legacy-canvas';
  const value = globalThis.localStorage?.getItem(STORAGE_KEY);
  if (value === 'isometric-canvas' || value === 'legacy-canvas') return value;
  return 'three-world';
}

export function writeRenderMode(mode: RenderMode): void {
  globalThis.localStorage?.setItem(STORAGE_KEY, mode);
}

export function urlForRenderMode(href: string, mode: RenderMode): string {
  const url = new URL(href);
  url.searchParams.set(ISOMETRIC_QUERY_KEY,
    mode === 'isometric-canvas' ? '2.5d' : mode === 'legacy-canvas' ? 'legacy' : '3d');
  return url.toString();
}
