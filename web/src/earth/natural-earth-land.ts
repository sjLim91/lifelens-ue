/** Coarse, public-domain Natural Earth physical LAND ONLY.
 * Observer presentation; it does not imply active Core geographic placement.
 */
export interface EarthLandFeatureCollection {
  type: 'FeatureCollection';
  features: Array<{
    type: 'Feature';
    geometry: {
      type: 'Polygon' | 'MultiPolygon';
      coordinates: number[][][] | number[][][][];
    };
  }>;
}

export interface CoastlinePoint {
  longitudeDeg: number;
  latitudeDeg: number;
}

// Consecutive points follow the shortest antimeridian crossing. The copies
// shifted by +/- one texture width are necessary to avoid a vertical tear.
export function unwrapCoastlineRing(ring: number[][]): CoastlinePoint[] {
  const output: CoastlinePoint[] = [];
  let lastLongitude = 0;
  for (const pair of ring) {
    const longitude = pair[0];
    const latitude = pair[1];
    if (!Number.isFinite(longitude) || !Number.isFinite(latitude)
      || Math.abs(latitude) > 90 || Math.abs(longitude) > 180) {
      continue;
    }
    let current = longitude;
    if (output.length > 0) {
      while (current - lastLongitude > 180) current -= 360;
      while (current - lastLongitude < -180) current += 360;
    }
    output.push({ longitudeDeg: current, latitudeDeg: latitude });
    lastLongitude = current;
  }
  return output;
}

export function landTexturePixel(
  point: CoastlinePoint, width: number, height: number,
): { x: number; y: number } {
  return {
    x: (point.longitudeDeg + 180) / 360 * width,
    y: (90 - point.latitudeDeg) / 180 * height,
  };
}

function landPolygons(
  feature: EarthLandFeatureCollection['features'][number],
): number[][][][] {
  if (feature.geometry.type === 'Polygon') {
    return [feature.geometry.coordinates as number[][][]];
  }
  if (feature.geometry.type === 'MultiPolygon') {
    return feature.geometry.coordinates as number[][][][];
  }
  return [];
}

/** Rasterize a static sea/land map once; no animated or invented global
 * weather or current vectors are computed in the observer.
 */
export function paintNaturalEarthTexture(
  context: CanvasRenderingContext2D,
  data: EarthLandFeatureCollection,
  width: number,
  height: number,
): void {
  if (data?.type !== 'FeatureCollection' || !Array.isArray(data.features)
    || data.features.length > 10000 || width <= 0 || height <= 0) {
    throw new Error('지원하지 않는 지구 자연지리 데이터입니다.');
  }
  context.fillStyle = '#0d284b';
  context.fillRect(0, 0, width, height);
  // Geographic graticule: only an observer aid, NOT a climate model.
  context.strokeStyle = 'rgba(97, 144, 169, .26)';
  context.lineWidth = 0.75;
  context.beginPath();
  for (let longitude = -150; longitude <= 150; longitude += 30) {
    const x = (longitude + 180) / 360 * width;
    context.moveTo(x, 0);
    context.lineTo(x, height);
  }
  for (let latitude = -60; latitude <= 60; latitude += 30) {
    const y = (90 - latitude) / 180 * height;
    context.moveTo(0, y);
    context.lineTo(width, y);
  }
  context.stroke();

  context.fillStyle = '#527d61';
  context.strokeStyle = '#a3c0a2';
  context.lineWidth = Math.max(0.65, width / 1900);
  for (const feature of data.features) {
    if (feature?.type !== 'Feature' || !feature.geometry) continue;
    for (const polygon of landPolygons(feature)) {
      for (const wrap of [-width, 0, width]) {
        context.beginPath();
        let hasValidRing = false;
        for (const ring of polygon) {
          const vertices = unwrapCoastlineRing(ring);
          if (vertices.length < 3) continue;
          const start = landTexturePixel(vertices[0], width, height);
          context.moveTo(start.x + wrap, start.y);
          for (let i = 1; i < vertices.length; i += 1) {
            const p = landTexturePixel(vertices[i], width, height);
            context.lineTo(p.x + wrap, p.y);
          }
          context.closePath();
          hasValidRing = true;
        }
        if (hasValidRing) {
          context.fill('evenodd');
          context.stroke();
        }
      }
    }
  }
}
