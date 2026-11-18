/** Field observer, regional 2.5D map, classic 3D and safe rescue coexist. */
export type RenderMode = 'field-canvas' | 'isometric-canvas' | 'legacy-canvas' | 'three-world';

const STORAGE_KEY = 'lifelens.web.renderMode';
export const ISOMETRIC_QUERY_KEY = 'view';

export function readRenderMode(): RenderMode {
  const search = typeof window === 'undefined' ? '' : window.location?.search ?? '';
  const requested = new URLSearchParams(search).get(ISOMETRIC_QUERY_KEY);
  // Existing 2.5D links now open the actual close-up field. The former
  // diamond isometric display stays available as an explicit regional view.
  if (requested === '2.5d' || requested === 'field') return 'field-canvas';
  if (requested === 'region') return 'isometric-canvas';
  if (requested === '3d') return 'three-world';
  if (requested === 'legacy') return 'legacy-canvas';
  const value = globalThis.localStorage?.getItem(STORAGE_KEY);
  if (value === 'field-canvas' || value === 'isometric-canvas' || value === 'legacy-canvas')
    return value;
  // Default change and 3D deletion are gated on F0-F4 visual/function QA.
  return 'three-world';
}

export function writeRenderMode(mode: RenderMode): void {
  globalThis.localStorage?.setItem(STORAGE_KEY, mode);
}

export function urlForRenderMode(href: string, mode: RenderMode): string {
  const url = new URL(href);
  url.searchParams.set(ISOMETRIC_QUERY_KEY,
    mode === 'field-canvas' ? '2.5d'
    : mode === 'isometric-canvas' ? 'region'
    : mode === 'legacy-canvas' ? 'legacy' : '3d');
  return url.toString();
}
