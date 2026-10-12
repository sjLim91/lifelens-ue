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

// Exact +1-radius direction used by THREE.SphereGeometry's equirectangular
// UV mapping. Keeping the inverse together with geoFromGlobeUV prevents
// moving a real Core-authorized resident marker to the wrong longitude.
export function globeSurfaceUnitVector(position: EarthPreviewGeoPoint): {
  x: number; y: number; z: number;
} | null {
  if (!Number.isFinite(position.latitudeDeg) ||
      !Number.isFinite(position.longitudeDeg) ||
      position.latitudeDeg < -90 || position.latitudeDeg > 90 ||
      position.longitudeDeg < -180 || position.longitudeDeg > 180) return null;
  const phi = (position.longitudeDeg + 180) * Math.PI / 180;
  const latitude = position.latitudeDeg * Math.PI / 180;
  return {
    x: -Math.cos(phi) * Math.cos(latitude),
    y: Math.sin(latitude),
    z: Math.sin(phi) * Math.cos(latitude),
  };
}

export function formatPreviewGeoPoint(location: EarthPreviewGeoPoint): string {
  if (!Number.isFinite(location.latitudeDeg)
      || !Number.isFinite(location.longitudeDeg)) return '유효하지 않은 좌표';
  const latSide = location.latitudeDeg >= 0 ? 'N' : 'S';
  const lonSide = location.longitudeDeg >= 0 ? 'E' : 'W';
  return `${Math.abs(location.latitudeDeg).toFixed(3)}°${latSide}, ${Math.abs(location.longitudeDeg).toFixed(3)}°${lonSide}`;
}
