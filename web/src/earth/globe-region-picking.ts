// Coordinates from a Three.js SphereGeometry UV and equirectangular
// Natural Earth imagery. This is a *user-selected preview point*, not a
// Core World / Scenario / resident location.
export interface EarthPreviewGeoPoint {
  latitudeDeg: number;
  longitudeDeg: number;
}

export function geoFromGlobeUV(
  u: number, v: number,
): EarthPreviewGeoPoint | null {
  if (!Number.isFinite(u) || !Number.isFinite(v)
      || u < 0 || u > 1 || v < 0 || v > 1) return null;
  const longitude = u >= 1 ? -180 : -180 + 360 * u;
  return {
    latitudeDeg: -90 + 180 * v,
    longitudeDeg: longitude,
  };
}

export function formatPreviewGeoPoint(location: EarthPreviewGeoPoint): string {
  if (!Number.isFinite(location.latitudeDeg)
      || !Number.isFinite(location.longitudeDeg)) return '유효하지 않은 좌표';
  const latSide = location.latitudeDeg >= 0 ? 'N' : 'S';
  const lonSide = location.longitudeDeg >= 0 ? 'E' : 'W';
  return `${Math.abs(location.latitudeDeg).toFixed(3)}°${latSide}, ${Math.abs(location.longitudeDeg).toFixed(3)}°${lonSide}`;
}
