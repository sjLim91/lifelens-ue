import * as THREE from 'three';
import type { Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createAuthoritativeGridProjector } from './authoritative-spatial-target-layer';
import { ObservedFootTraffic } from './observed-foot-traffic';
import { WORLD_PRESENTATION } from './world-presentation-config';
import { clamp01 } from './environment-surface-presentation';

/** One pooled draw call; all marks are actual observed grid positions. */
export class FootTrafficLayer {
  readonly group = new THREE.Group();
  private readonly history = new ObservedFootTraffic();
  private readonly geometry = new THREE.BufferGeometry();
  private readonly positions = new Float32Array(WORLD_PRESENTATION.paths.maxMarks * 6 * 3);
  private readonly colors = new Float32Array(WORLD_PRESENTATION.paths.maxMarks * 6 * 4);
  private readonly material = new THREE.MeshStandardMaterial({
    roughness: WORLD_PRESENTATION.paths.dryRoughness, metalness: 0,
    vertexColors: true, transparent: true, depthWrite: false,
    polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1,
    side: THREE.DoubleSide,
  });
  private readonly mesh = new THREE.Mesh(this.geometry, this.material);
  private lastMinute = NaN;
  private wetness = 0;
  private lastTerrain: TerrainWindow | null = null;

  constructor() {
    this.group.name = 'observed-foot-traffic';
    this.geometry.setAttribute('position', new THREE.BufferAttribute(this.positions, 3).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setAttribute('normal', new THREE.BufferAttribute(new Float32Array(this.positions.length), 3));
    for (let i = 1; i < this.positions.length; i += 3) this.geometry.attributes.normal.array[i] = 1;
    this.geometry.setAttribute('color', new THREE.BufferAttribute(this.colors, 4).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setDrawRange(0, 0);
    this.mesh.frustumCulled = false;
    this.group.add(this.mesh);
  }

  observe(residents: Resident[], terrain: TerrainWindow, minute: number): void {
    if (!Number.isFinite(minute)) return;
    if (minute === this.lastMinute && terrain === this.lastTerrain) return;
    this.lastMinute = minute; this.lastTerrain = terrain;
    this.history.observe(residents, terrain.worldSeed, minute);
    this.redraw(terrain, minute);
  }

  setWetness(wetness: number): void {
    const next = clamp01(wetness);
    if (next === this.wetness) return;
    this.wetness = next;
    this.material.roughness = THREE.MathUtils.lerp(WORLD_PRESENTATION.paths.dryRoughness, WORLD_PRESENTATION.paths.wetRoughness, next);
    // Redraw the same observed marks; do not observe again, add visits or alter decay.
    if (this.lastTerrain) this.redraw(this.lastTerrain, this.lastMinute);
  }

  private redraw(terrain: TerrainWindow, minute: number): void {
    const projector = createAuthoritativeGridProjector(terrain);
    const config = WORLD_PRESENTATION.paths;
    const dryTint = new THREE.Color(config.color), wetTint = new THREE.Color(config.wetMudColor);
    let vertex = 0;
    for (const mark of this.history.marks.values()) {
      if (!projector.containsGrid(mark.x, mark.y)) continue;
      const strength = this.history.opacity(mark, minute);
      const mud = this.wetness * clamp01(strength / config.maxOpacity);
      const opacity = strength * (1 + mud * (config.wetMudOpacityMultiplier - 1));
      const tint = dryTint.clone().lerp(wetTint, mud);
      const width = config.widthGrid * (1 + mud * (config.wetMudWidthMultiplier - 1));
      // Narrow diamond at the visited cell, no fabricated segment between samples.
      const alongX = Math.cos(mark.angle), alongY = Math.sin(mark.angle);
      const corners = [[-0.5, 0], [0, width], [0.5, 0], [0, -width]];
      const points = corners.map(([along, across]) => projector.project(
        mark.x + along * alongX - across * alongY,
        mark.y + along * alongY + across * alongX,
      ));
      if (points.some(point => !point)) continue;
      for (const index of [0, 1, 2, 0, 2, 3]) {
        const point = points[index]!;
        this.positions.set([point.x, point.y + WORLD_GRID_CONTRACT.worldUnitsPerChunk / WORLD_GRID_CONTRACT.gridCellsPerChunk * 0.015, point.z], vertex * 3);
        this.colors.set([tint.r, tint.g, tint.b, opacity], vertex * 4);
        vertex++;
      }
    }
    this.geometry.setDrawRange(0, vertex);
    this.geometry.attributes.position.needsUpdate = true;
    this.geometry.attributes.color.needsUpdate = true;
  }

  setSpeed(speed: number): void { this.history.setSpeed(speed); }

  breakContinuity(): void { this.history.breakContinuity(); }

  dispose(): void {
    this.geometry.dispose(); this.material.dispose(); this.group.clear();
  }
}

