import * as THREE from 'three';
import type { DynamicEnvironment, HumanTrace, TerrainWindow } from '../runtime/core-types';
import { visibleHumanTraces } from '../state/human-traces';
import { createAuthoritativeGridProjector } from './authoritative-spatial-target-layer';
import { clamp01, presentationHash01 } from './environment-surface-presentation';
import { createVisibleWaterFootprintTester } from './water-geometry';
import { WORLD_PRESENTATION } from './world-presentation-config';

type FireTrace = Extract<HumanTrace, { kind: 'Facility' }>;
interface FireSource { trace: FireTrace; x: number; y: number; z: number; phase: number }

export function isEmittingFire(trace: FireTrace): boolean {
  return (trace.facilityKind === 'FirePit' || trace.facilityKind === 'Furnace')
    && trace.state === 'Operational' && trace.active === true && trace.lit === true;
}

/** Two global pools, never a particle system/material per facility.
 * Scorch represents current fire-facility existence/state, not burn history.
 * Smoke uses the read model's Operational/active/lit contract, never guessed fuel.
 */
export class FacilityEmissionLayer {
  readonly group = new THREE.Group();
  private readonly smokeGeometry = new THREE.BufferGeometry();
  private readonly smokePositions = new Float32Array(WORLD_PRESENTATION.fire.smokeMaxParticles * 3);
  private readonly smokeSizes = new Float32Array(WORLD_PRESENTATION.fire.smokeMaxParticles);
  private readonly smokeAlphas = new Float32Array(WORLD_PRESENTATION.fire.smokeMaxParticles);
  private readonly smokeMaterial = new THREE.ShaderMaterial({
    uniforms: { pixelRatio: { value: 1 } },
    vertexShader: `
      uniform float pixelRatio;
      attribute float smokeSize;
      attribute float smokeAlpha;
      varying float alpha;
      void main() {
        vec4 mv = modelViewMatrix * vec4(position, 1.0);
        gl_Position = projectionMatrix * mv;
        gl_PointSize = clamp(smokeSize * 300.0 * pixelRatio / max(1.0, -mv.z), 1.0, 64.0 * pixelRatio);
        alpha = smokeAlpha;
      }`,
    fragmentShader: `
      varying float alpha;
      void main() {
        float radius = length(gl_PointCoord - 0.5);
        float opacity = (1.0 - smoothstep(0.08, 0.5, radius)) * alpha;
        if (opacity < 0.005) discard;
        gl_FragColor = vec4(vec3(0.43, 0.45, 0.44), opacity);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }`,
    transparent: true, depthWrite: false, depthTest: true,
  });
  private readonly smoke = new THREE.Points(this.smokeGeometry, this.smokeMaterial);
  private readonly scorchGeometry = new THREE.CircleGeometry(1, 20).rotateX(-Math.PI / 2);
  private readonly scorchMaterial = new THREE.ShaderMaterial({
    uniforms: { opacity: { value: WORLD_PRESENTATION.fire.scorchOpacity } },
    vertexShader: `
      attribute float strength;
      varying vec2 patchUV;
      varying float patchStrength;
      void main() {
        patchUV = uv; patchStrength = strength;
        gl_Position = projectionMatrix * modelViewMatrix * instanceMatrix * vec4(position, 1.0);
      }`,
    fragmentShader: `
      uniform float opacity;
      varying vec2 patchUV;
      varying float patchStrength;
      void main() {
        vec2 p = patchUV - 0.5;
        float mottle = 0.75 + 0.25 * sin(p.x * 36.0) * cos(p.y * 27.0);
        float alpha = (1.0 - smoothstep(0.12, 0.5, length(p))) * mottle * opacity * patchStrength;
        gl_FragColor = vec4(vec3(0.055, 0.052, 0.045), alpha);
        #include <tonemapping_fragment>
        #include <colorspace_fragment>
      }`,
    transparent: true, depthWrite: false, depthTest: true,
    polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1,
  });
  private readonly scorch = new THREE.InstancedMesh(this.scorchGeometry, this.scorchMaterial, WORLD_PRESENTATION.fire.scorchMaxInstances);
  private readonly scorchStrength = new THREE.InstancedBufferAttribute(new Float32Array(WORLD_PRESENTATION.fire.scorchMaxInstances), 1);
  private sources: FireSource[] = [];
  private wind = 0;
  private minute = 0;
  private signature = '';
  private readonly matrix = new THREE.Matrix4();
  private readonly position = new THREE.Vector3();
  private readonly rotation = new THREE.Quaternion();
  private readonly scale = new THREE.Vector3();

  constructor() {
    this.group.name = 'facility-emissions';
    this.smokeGeometry.setAttribute('position', new THREE.BufferAttribute(this.smokePositions, 3).setUsage(THREE.DynamicDrawUsage));
    this.smokeGeometry.setAttribute('smokeSize', new THREE.BufferAttribute(this.smokeSizes, 1).setUsage(THREE.DynamicDrawUsage));
    this.smokeGeometry.setAttribute('smokeAlpha', new THREE.BufferAttribute(this.smokeAlphas, 1).setUsage(THREE.DynamicDrawUsage));
    this.scorchGeometry.setAttribute('strength', this.scorchStrength);
    this.scorch.count = 0;
    this.smokeGeometry.setDrawRange(0, 0);
    this.scorch.frustumCulled = this.smoke.frustumCulled = false;
    this.scorch.renderOrder = 1;
    this.smoke.renderOrder = 4;
    this.smoke.onBeforeRender = renderer => { this.smokeMaterial.uniforms.pixelRatio.value = renderer.getPixelRatio(); };
    this.group.add(this.scorch, this.smoke);
  }

  setTerrain(terrain: TerrainWindow): void {
    const fires = visibleHumanTraces(terrain).filter((trace): trace is FireTrace => trace.kind === 'Facility'
      && (trace.facilityKind === 'FirePit' || trace.facilityKind === 'Furnace')
      && (trace.state === 'Operational' || trace.state === 'Ruined'));
    const signature = JSON.stringify([terrain.worldSeed, terrain.centerChunkX, terrain.centerChunkY,
      terrain.chunks.map(c => [c.x, c.y, c.elevation01, c.waterKind]), fires]);
    if (signature === this.signature) return;
    this.signature = signature;
    const projector = createAuthoritativeGridProjector(terrain);
    const water = createVisibleWaterFootprintTester(terrain);
    const config = WORLD_PRESENTATION.fire;
    let scorchCount = 0;
    this.sources = [];
    for (const trace of fires) {
      const point = projector.project(trace.gridX, trace.gridY);
      if (!point) continue;
      const phase = presentationHash01(`${terrain.worldSeed ?? '0'}:${trace.id}:smoke`);
      if (isEmittingFire(trace) && this.sources.length < Math.ceil(config.smokeMaxParticles / config.smokePerFacility)) {
        this.sources.push({ trace, x: point.x, y: point.y, z: point.z, phase });
      }
      if (scorchCount >= config.scorchMaxInstances || water(point.x, point.z)) continue;
      const radius = config.scorchRadius * (trace.facilityKind === 'Furnace' ? 1.2 : 1);
      this.position.set(point.x, point.y + WORLD_PRESENTATION.weather.surfaceLift, point.z);
      this.scale.set(radius, 1, radius * (0.8 + phase * 0.2));
      // Align the shared plane to the observed local ground, no horizontal floating disk.
      const east = projector.project(trace.gridX + 0.5, trace.gridY);
      const west = projector.project(trace.gridX - 0.5, trace.gridY);
      const south = projector.project(trace.gridX, trace.gridY + 0.5);
      const north = projector.project(trace.gridX, trace.gridY - 0.5);
      const normal = new THREE.Vector3(
        east && west ? -(east.y - west.y) / (east.x - west.x) : 0, 1,
        south && north ? -(south.y - north.y) / (south.z - north.z) : 0,
      ).normalize();
      this.rotation.setFromUnitVectors(new THREE.Vector3(0, 1, 0), normal);
      this.matrix.compose(this.position, this.rotation, this.scale);
      this.scorch.setMatrixAt(scorchCount, this.matrix);
      this.scorchStrength.setX(scorchCount, isEmittingFire(trace) ? 1 : trace.state === 'Ruined' ? 0.85 : 0.55);
      scorchCount++;
    }
    this.scorch.count = scorchCount;
    this.scorch.instanceMatrix.needsUpdate = true;
    this.scorchStrength.needsUpdate = true;
    this.updateSmoke();
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    const next = environment?.available === false ? 0 : clamp01(environment?.windIntensity01);
    if (next === this.wind) return;
    this.wind = next; this.updateSmoke();
  }

  setSimulationMinute(minute: number): void {
    if (!Number.isFinite(minute) || minute === this.minute) return;
    this.minute = minute; this.updateSmoke();
  }

  private updateSmoke(): void {
    const config = WORLD_PRESENTATION.fire;
    let index = 0;
    for (const source of this.sources) {
      // The DTO exposes intensity, not direction. Seeded drift is a cosmetic
      // silhouette only; never described as an authoritative compass wind.
      const direction = source.phase * Math.PI * 2;
      for (let i = 0; i < config.smokePerFacility && index < config.smokeMaxParticles; i++, index++) {
        const phase = (source.phase + i / config.smokePerFacility + this.minute * 0.017) % 1;
        const drift = phase * phase * this.wind * config.smokeDrift;
        const sway = Math.sin(phase * Math.PI * 2 + i) * 0.15;
        this.smokePositions.set([
          source.x + Math.cos(direction) * drift + sway,
          source.y + (source.trace.facilityKind === 'Furnace' ? 1.65 : 0.55) + phase * config.smokeHeight,
          source.z + Math.sin(direction) * drift + sway * 0.5,
        ], index * 3);
        this.smokeSizes[index] = config.smokeSize * (0.4 + phase);
        this.smokeAlphas[index] = config.smokeOpacity * Math.sin(phase * Math.PI);
      }
    }
    this.smokeGeometry.setDrawRange(0, index);
    for (const name of ['position', 'smokeSize', 'smokeAlpha']) this.smokeGeometry.attributes[name].needsUpdate = true;
    this.smoke.visible = index > 0;
  }

  dispose(): void {
    this.smokeGeometry.dispose(); this.smokeMaterial.dispose();
    this.scorch.dispose(); this.scorchGeometry.dispose(); this.scorchMaterial.dispose();
    this.sources = []; this.group.clear();
  }
}
