import * as THREE from 'three';
import type { CivilizationWorldPayload, TerrainWindow } from '../runtime/core-types';
import { WORLD_UNITS_PER_GRID_CELL } from '../runtime/lifelens-contract';
import { createAuthoritativeGridProjector } from './authoritative-spatial-target-layer';
import { WORLD_PRESENTATION } from './world-presentation-config';

/** One reusable anchor halo on selection. This is not a territory or trade road. */
export class SettlementFocusLayer {
  readonly group = new THREE.Group();
  private readonly geometry = new THREE.BufferGeometry();
  private readonly material = new THREE.LineBasicMaterial({
    color: WORLD_PRESENTATION.settlement.focusColor, transparent: true,
    opacity: WORLD_PRESENTATION.settlement.focusOpacity, depthWrite: false,
  });
  private readonly points = new Float32Array(WORLD_PRESENTATION.settlement.focusSegments * 2 * 3);
  private readonly mesh = new THREE.LineSegments(this.geometry, this.material);
  private anchors: { id: string; position: THREE.Vector3 }[] = [];
  private civilization: CivilizationWorldPayload = {};
  private terrain: TerrainWindow | null = null;
  private selectedId: string | null = null;
  private signature = '';

  constructor() {
    this.group.name = 'settlement-focus';
    this.geometry.setAttribute('position', new THREE.BufferAttribute(this.points, 3).setUsage(THREE.DynamicDrawUsage));
    this.mesh.frustumCulled = false;
    this.mesh.visible = false;
    this.group.add(this.mesh);
  }

  setTargets(civilization: CivilizationWorldPayload, terrain: TerrainWindow): void {
    const signature = JSON.stringify([terrain.worldSeed, terrain.centerChunkX, terrain.centerChunkY,
      civilization.available, civilization.settlements?.map(entry => [entry.id, entry.gridX, entry.gridY]),
      terrain.chunks.map(chunk => [chunk.x, chunk.y, chunk.elevation01])]);
    if (signature === this.signature) return;
    this.signature = signature;
    this.civilization = civilization;
    this.terrain = terrain;
    const projector = createAuthoritativeGridProjector(terrain);
    this.anchors = [];
    if (civilization.available === true) for (const entry of civilization.settlements ?? []) {
      if (this.anchors.length >= WORLD_PRESENTATION.settlement.maxVisibleAnchors) break;
      if (!projector.containsGrid(entry.gridX, entry.gridY)) continue;
      const point = projector.project(entry.gridX, entry.gridY);
      if (point) this.anchors.push({ id: entry.id, position: new THREE.Vector3(point.x, point.y, point.z) });
    }
    this.rebuildFocus();
  }

  select(id: string | null): void {
    if (id === this.selectedId) return;
    this.selectedId = id;
    this.rebuildFocus();
  }

  pick(raycaster: THREE.Raycaster): string | null {
    const radius = WORLD_PRESENTATION.settlement.hitRadiusGrid * WORLD_UNITS_PER_GRID_CELL;
    let best: { id: string; distance: number } | undefined;
    for (const anchor of this.anchors) {
      if (raycaster.ray.direction.dot(anchor.position.clone().sub(raycaster.ray.origin)) <= 0) continue;
      const distance = raycaster.ray.distanceSqToPoint(anchor.position);
      if (distance <= radius * radius && (!best || distance < best.distance)) best = { id: anchor.id, distance };
    }
    return best?.id ?? null;
  }

  private rebuildFocus(): void {
    this.mesh.visible = false;
    const entry = this.civilization.available === true
      && this.civilization.settlements?.find(item => item.id === this.selectedId);
    if (!entry || !this.terrain || !this.anchors.some(anchor => anchor.id === entry.id)) return;
    const projector = createAuthoritativeGridProjector(this.terrain);
    const config = WORLD_PRESENTATION.settlement;
    for (let i = 0; i < config.focusSegments * 2; i++) {
      const segment = Math.floor(i / 2) + i % 2;
      const angle = segment / config.focusSegments * Math.PI * 2;
      const point = projector.project(entry.gridX + Math.cos(angle) * config.focusRadiusGrid,
        entry.gridY + Math.sin(angle) * config.focusRadiusGrid);
      if (!point) return;
      this.points.set([point.x, point.y + WORLD_UNITS_PER_GRID_CELL * config.groundLift, point.z], i * 3);
    }
    this.geometry.attributes.position.needsUpdate = true;
    this.geometry.setDrawRange(0, config.focusSegments * 2);
    this.mesh.visible = true;
  }

  dispose(): void { this.geometry.dispose(); this.material.dispose(); this.group.clear(); this.anchors = []; }
}
