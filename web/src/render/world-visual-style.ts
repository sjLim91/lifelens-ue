/**
 * Optional presentation experiment for the actual Three.js observer world.
 * No Core/DTO/seed, terrain, resource, camera, save, or simulation authority changes.
 * The mode is deliberately URL-scoped so the stable world always loads as before.
 */
export type WorldVisualStyle = 'classic' | 'illustrated';

export const WORLD_STYLE_QUERY_KEY = 'world-style';

export function worldStyleFromSearch(search: string): WorldVisualStyle {
  return new URLSearchParams(search).get(WORLD_STYLE_QUERY_KEY) === 'illustrated'
    ? 'illustrated' : 'classic';
}

export function readWorldVisualStyle(): WorldVisualStyle {
  return typeof window === 'undefined'
    ? 'classic'
    : worldStyleFromSearch(window.location.search);
}

export function urlForWorldVisualStyle(href: string, style: WorldVisualStyle): string {
  const url = new URL(href);
  if (style === 'classic') url.searchParams.delete(WORLD_STYLE_QUERY_KEY);
  else url.searchParams.set(WORLD_STYLE_QUERY_KEY, 'illustrated');
  return url.toString();
}

// Colors belong to the Web presentation only. Existing placement and material
// stock remain from the very same authoritative Core observation.
export const ILLUSTRATED_GROUND_PALETTE = {
  grass: [0x709b4b, 0x83aa59, 0x5d8548, 0x9ab36c],
  shrub: [0x3d7952, 0x4c9360, 0x34684b, 0x6a9c63],
  rock: [0x9c9d8e, 0xb8b7a4, 0x83897d, 0xc4c2ae],
  foliage: [0x335d45, 0x447957, 0x639260, 0x89ad67, 0x9db86e, 0xc7aa6e, 0xcb8d78],
} as const;
