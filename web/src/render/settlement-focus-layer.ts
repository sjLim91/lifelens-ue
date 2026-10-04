import * as THREE from 'three';
import type { CivilizationWorldPayload, Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_UNITS_PER_GRID_CELL } from '../runtime/lifelens-contract';
import { createAuthoritativeGridProjector, type AuthoritativeGridProjector, type TargetWorldPosition } from './authoritative-spatial-target-layer';
import { WORLD_PRESENTATION } from './world-presentation-config';
import { observedSettlements, observedTradeRoutes, selectedFrontier } from './settlement-presentation';

const config = WORLD_PRESENTATION.settlement;
interface Batch { geometry: THREE.BufferGeometry; material: THREE.LineBasicMaterial; mesh: THREE.LineSegments;
  points: Float32Array; colors: Float32Array; count: number }

/** Three fixed observer batches: centers/selection, trade relationships, selected frontier direction.
 * These are not boundaries, roads, planned settlements or resident-to-route assignments. */
export class SettlementFocusLayer {
  readonly group = new THREE.Group();
  private readonly centers = this.batch((config.maxVisibleAnchors + 1) * config.focusSegments);
  private readonly routes = this.batch(config.maxVisibleRoutes * config.routeSegments);
  private readonly guide = this.batch(config.migrationSegments);
  private anchors: { id: string; position: THREE.Vector3 }[] = [];
  private civilization: CivilizationWorldPayload = {};
  private projector: AuthoritativeGridProjector | null = null;
  private selectedId: string | null = null;
  private selectedResident: Resident | undefined;
  private signature = '';
  private anchorScale = 1;
  private disposed = false;
  private readonly color = new THREE.Color();
  private readonly center = new THREE.Vector3();

  constructor() {
    this.group.name = 'settlement-observer';
    this.group.add(this.centers.mesh, this.routes.mesh, this.guide.mesh);
  }

  private batch(segments: number): Batch {
    const geometry = new THREE.BufferGeometry();
    const points = new Float32Array(segments * 6), colors = new Float32Array(segments * 8);
    geometry.setAttribute('position', new THREE.BufferAttribute(points, 3).setUsage(THREE.DynamicDrawUsage));
    geometry.setAttribute('color', new THREE.BufferAttribute(colors, 4).setUsage(THREE.DynamicDrawUsage));
    geometry.setDrawRange(0, 0);
    const material = new THREE.LineBasicMaterial({ vertexColors: true, transparent: true,
      depthWrite: false, depthTest: true, opacity: config.anchorOpacity });
    const mesh = new THREE.LineSegments(geometry, material);
    mesh.frustumCulled = false;
    mesh.visible = false;
    return { geometry, material, mesh, points, colors, count: 0 };
  }

  private segment(batch: Batch, first: TargetWorldPosition | null, second: TargetWorldPosition | null,
    color: number, strength = 1): void {
    if (!first || !second || batch.count * 6 >= batch.points.length) return;
    const offset = batch.count++ * 6;
    const lift = WORLD_UNITS_PER_GRID_CELL * config.groundLift;
    batch.points.set([first.x, first.y + lift, first.z, second.x, second.y + lift, second.z], offset);
    this.color.setHex(color);
    batch.colors.set([this.color.r, this.color.g, this.color.b, strength, this.color.r, this.color.g, this.color.b, strength], (batch.count - 1) * 8);
  }

  private flush(batch: Batch): void {
    batch.geometry.attributes.position.needsUpdate = true;
    batch.geometry.attributes.color.needsUpdate = true;
    batch.geometry.setDrawRange(0, batch.count * 2);
    batch.mesh.visible = batch.count > 0;
  }

  setTargets(civilization: CivilizationWorldPayload, terrain: TerrainWindow): void {
    if (this.disposed) return;
    const settlements = observedSettlements(civilization), routes = observedTradeRoutes(civilization, settlements);
    const signature = JSON.stringify([terrain.worldSeed, terrain.centerChunkX, terrain.centerChunkY, terrain.available,
      settlements.map(entry => [entry.id, entry.gridX, entry.gridY, entry.active]), routes,
      terrain.chunks.map(chunk => [chunk.x, chunk.y, chunk.elevation01])]);
    if (signature === this.signature) return;
    this.signature = signature;
    this.civilization = civilization;
    this.projector = createAuthoritativeGridProjector(terrain);
    this.anchors = [];
    // Selection receives its slot before the visible cap, without changing any membership truth.
    const ordered = [...settlements].sort((a, b) => Number(b.id === this.selectedId) - Number(a.id === this.selectedId)
      || Number(b.active) - Number(a.active));
    for (const entry of ordered) {
      if (this.anchors.length >= config.maxVisibleAnchors) break;
      const point = this.projector.project(entry.gridX, entry.gridY);
      if (point) this.anchors.push({ id: entry.id, position: new THREE.Vector3(point.x, point.y, point.z) });
    }
    this.rebuildCenters();
    this.routes.count = 0;
    for (const route of [...routes].sort((a, b) => Number(b.active) - Number(a.active)
      || b.exchangeCount - a.exchangeCount).slice(0, config.maxVisibleRoutes)) {
      const evidence = Math.min(1, (route.exchangeCount + route.partnerCount) / config.routeEvidenceScale);
      const strength = (route.active ? 1 : config.inactiveStrength)
        * (config.routeBaseStrength + evidence * config.routeEvidenceStrength);
      for (let i = 0; i < config.routeSegments; i++) {
        const at = (t: number) => this.projector!.project(
          route.firstGridX + (route.secondGridX - route.firstGridX) * t,
          route.firstGridY + (route.secondGridY - route.firstGridY) * t);
        this.segment(this.routes, at(i / config.routeSegments),
          at((i + config.routeDashFraction) / config.routeSegments), config.routeColor, strength);
      }
    }
    this.flush(this.routes);
    this.rebuildGuide();
  }

  select(id: string | null): void {
    if (this.disposed || id === this.selectedId) return;
    this.selectedId = id;
    this.rebuildCenters();
  }

  setSelectedResident(resident: Resident | undefined): void {
    this.selectedResident = resident;
    this.rebuildGuide();
  }

  private rebuildGuide(anchor?: TargetWorldPosition | null): void {
    this.guide.count = 0;
    const resident = this.selectedResident, frontier = selectedFrontier(resident);
    if (frontier && resident && this.projector) {
      const dx = frontier.x - resident.gridX!, dy = frontier.y - resident.gridY!;
      const distance = Math.hypot(dx, dy);
      const length = Math.min(distance, config.migrationLengthGrid);
      const start = this.projector.project(resident.gridX!, resident.gridY!);
      if (start && distance > 0) for (let i = 0; i < config.migrationSegments; i++) {
        const at = (t: number) => {
          const point = this.projector!.project(resident.gridX! + dx / distance * length * t,
            resident.gridY! + dy / distance * length * t);
          // Follow the real rendered actor, without altering its authoritative movement.
          if (point && anchor) return { x: point.x + anchor.x - start.x,
            y: point.y + anchor.y - start.y, z: point.z + anchor.z - start.z };
          return point;
        };
        this.segment(this.guide, at(i / config.migrationSegments),
          at((i + config.routeDashFraction) / config.migrationSegments), config.migrationColor);
      }
    }
    this.flush(this.guide);
  }

  private rebuildCenters(): void {
    this.centers.count = 0;
    const entries = new Map(observedSettlements(this.civilization).map(entry => [entry.id, entry]));
    if (this.projector) for (const anchor of this.anchors) {
      const entry = entries.get(anchor.id);
      if (!entry) continue;
      const ring = (radius: number, segments: number, strength: number) => {
        const at = (i: number) => this.projector!.project(entry.gridX + Math.cos(i / segments * Math.PI * 2) * radius,
          entry.gridY + Math.sin(i / segments * Math.PI * 2) * radius);
        for (let i = 0; i < segments; i++) this.segment(this.centers, at(i), at(i + 1), config.focusColor, strength);
      };
      ring(config.anchorRadiusGrid * this.anchorScale, config.anchorSegments, entry.active ? 1 : config.inactiveStrength);
      if (entry.id === this.selectedId) ring(config.focusRadiusGrid * this.anchorScale, config.focusSegments, 1);
    }
    this.flush(this.centers);
  }

  pick(raycaster: THREE.Raycaster): string | null {
    const radius = config.hitRadiusGrid * this.anchorScale * WORLD_UNITS_PER_GRID_CELL;
    let best: { id: string; distance: number } | undefined;
    for (const anchor of this.anchors) {
      if (raycaster.ray.direction.dot(this.center.copy(anchor.position).sub(raycaster.ray.origin)) <= 0) continue;
      const distance = raycaster.ray.distanceSqToPoint(anchor.position);
      if (distance <= radius * radius && (!best || distance < best.distance)) best = { id: anchor.id, distance };
    }
    return best?.id ?? null;
  }

  update(camera: THREE.Camera, residentAnchor?: TargetWorldPosition | null): void {
    if (this.disposed) return;
    // Camera LOD only; static dashes have no gameplay or cosmetic clock to advance while paused.
    let nearest = Infinity;
    for (const anchor of this.anchors) nearest = Math.min(nearest, camera.position.distanceTo(anchor.position));
    const scale = Math.round(THREE.MathUtils.clamp(nearest / config.anchorLodDistance, 1, config.anchorLodMaxScale)
      * config.anchorLodSteps) / config.anchorLodSteps;
    if (scale !== this.anchorScale) { this.anchorScale = scale; this.rebuildCenters(); }
    const lod = config.nearOpacityScale + (1 - config.nearOpacityScale)
      * THREE.MathUtils.clamp((nearest - config.nearCameraDistance) / (config.farCameraDistance - config.nearCameraDistance), 0, 1);
    this.centers.material.opacity = config.anchorOpacity * lod;
    this.routes.material.opacity = config.routeOpacity * lod;
    this.guide.material.opacity = config.migrationOpacity;
    if (this.selectedResident && residentAnchor) this.rebuildGuide(residentAnchor);
    else { this.guide.count = 0; this.flush(this.guide); }
  }

  reset(): void {
    this.signature = ''; this.anchorScale = 1; this.selectedId = null; this.selectedResident = undefined;
    this.civilization = {}; this.projector = null; this.anchors = [];
    for (const batch of [this.centers, this.routes, this.guide]) { batch.count = 0; this.flush(batch); }
  }

  get diagnostics() { return { anchors: this.anchors.length, routeSegments: this.routes.count,
    guideSegments: this.guide.count, objects: this.group.children.length, geometries: 3, materials: 3 }; }

  dispose(): void {
    if (this.disposed) return;
    this.reset(); this.disposed = true;
    for (const batch of [this.centers, this.routes, this.guide]) { batch.geometry.dispose(); batch.material.dispose(); }
    this.group.clear();
  }
}
