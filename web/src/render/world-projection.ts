export interface ProjectionConfig {
  width: number;
  height: number;
  extent: number;
  zoom: number;
  angle: number;
  heightScaleFactor?: number;
}

export interface WorldProjector {
  tile: number;
  heightScale: number;
  project: (x: number, y: number, elevation: number) => [number, number];
}

export function createWorldProjector(config: ProjectionConfig): WorldProjector {
  const tile = Math.min(
    config.width / (Math.max(1, config.extent) * 1.22),
    config.height / (Math.max(1, config.extent) * 0.7),
  ) * config.zoom;
  const centerPxX = config.width * 0.5;
  const centerPxY = config.height * 0.62;
  const cos = Math.cos(config.angle);
  const sin = Math.sin(config.angle);
  const heightScale = tile * (config.heightScaleFactor ?? 6.25);

  return {
    tile,
    heightScale,
    project(x: number, y: number, elevation: number): [number, number] {
      const rx = x * cos - y * sin;
      const ry = x * sin + y * cos;
      return [
        centerPxX + rx * tile,
        centerPxY + ry * tile * 0.52 - (elevation - 0.5) * heightScale,
      ];
    },
  };
}
