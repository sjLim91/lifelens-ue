import * as THREE from 'three';
import type { Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createAuthoritativeGridProjector } from './authoritative-spatial-target-layer';
import { ObservedFootTraffic } from './observed-foot-traffic';
import { WORLD_PRESENTATION } from './world-presentation-config';

/** One pooled draw call; all marks are actual observed grid positions. */
export class FootTrafficLayer {
  readonly group = new THREE.Group();
  private readonly history = new ObservedFootTraffic();
  private readonly geometry = new THREE.BufferGeometry();
  private readonly positions = new Float32Array(WORLD_PRESENTATION.paths.maxMarks * 6 * 3);
  private readonly colors = new Float32Array(WORLD_PRESENTATION.paths.maxMarks * 6 * 4);
  private readonly material = new THREE.MeshBasicMaterial({
    vertexColors: true, transparent: true, depthWrite: false,
    polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1,
    side: THREE.DoubleSide,
  });
  private readonly mesh = new THREE.Mesh(this.geometry, this.material);
  private lastMinute = NaN;
  private lastTerrain: TerrainWindow | null = null;

  constructor() {
    this.group.name = 'observed-foot-traffic';
    this.geometry.setAttribute('position', new THREE.BufferAttribute(this.positions, 3).setUsage(THREE.DynamicDrawUsage));
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
    const projector = createAuthoritativeGridProjector(terrain);
    const tint = new THREE.Color(WORLD_PRESENTATION.paths.color);
    const width = WORLD_PRESENTATION.paths.widthGrid;
    let vertex = 0;
    for (const mark of this.history.marks.values()) {
      if (!projector.containsGrid(mark.x, mark.y)) continue;
      const opacity = this.history.opacity(mark, minute);
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
