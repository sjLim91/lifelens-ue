import * as THREE from 'three';
import type { DynamicEnvironment } from '../runtime/core-types';

const MAX_RAIN_DROPS = 1100;
const MAX_SNOW_FLAKES = 800;

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function seeded01(index: number, salt: number): number {
  const value = Math.sin(
    index * 12.9898 + salt * 78.233,
  ) * 43758.5453;
  return value - Math.floor(value);
}

export class WeatherLayer {
  readonly group = new THREE.Group();

  private readonly rainPositions = new Float32Array(
    MAX_RAIN_DROPS * 2 * 3,
  );
  private readonly rainGeometry = new THREE.BufferGeometry();
  private readonly rainMaterial = new THREE.LineBasicMaterial({
    color: 0xc4dce8,
    transparent: true,
    opacity: 0.68,
    depthWrite: false,
  });
  private readonly rainLines: THREE.LineSegments;

  private readonly snowPositions = new Float32Array(
    MAX_SNOW_FLAKES * 3,
  );
  private readonly snowGeometry = new THREE.BufferGeometry();
  private readonly snowMaterial = new THREE.PointsMaterial({
    color: 0xf2f6f7,
    size: 0.82,
    transparent: true,
    opacity: 0.9,
    depthWrite: false,
    sizeAttenuation: true,
  });
  private readonly snowPoints: THREE.Points;

  private precipitation: 'None' | 'Rain' | 'Snow' = 'None';
  private intensity = 0;
  private wind = 0;
  private activeRainCount = 0;
  private activeSnowCount = 0;
  private animationTime = 0;

  constructor() {
    this.initializeRain();
    this.initializeSnow();

    this.rainGeometry.setAttribute(
      'position',
      new THREE.BufferAttribute(this.rainPositions, 3),
    );
    this.snowGeometry.setAttribute(
      'position',
      new THREE.BufferAttribute(this.snowPositions, 3),
    );

    this.rainGeometry.setDrawRange(0, 0);
    this.snowGeometry.setDrawRange(0, 0);

    this.rainLines = new THREE.LineSegments(
      this.rainGeometry,
      this.rainMaterial,
    );
    this.rainLines.frustumCulled = false;
    this.rainLines.renderOrder = 30;

    this.snowPoints = new THREE.Points(
      this.snowGeometry,
      this.snowMaterial,
    );
    this.snowPoints.frustumCulled = false;
    this.snowPoints.renderOrder = 30;

    this.group.add(this.rainLines);
    this.group.add(this.snowPoints);
    this.group.visible = false;
  }

  setAnchor(x: number, y: number, z: number): void {
    this.group.position.set(
      Number(x) || 0,
      Number(y) || 0,
      Number(z) || 0,
    );
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.precipitation = environment?.precipitationType ?? 'None';
    this.intensity = clamp01(environment?.precipitationIntensity01);
    this.wind = clamp01(environment?.windIntensity01);

    const active = this.precipitation !== 'None'
      && this.intensity >= 0.025;

    this.activeRainCount = (
      active && this.precipitation === 'Rain'
    )
      ? Math.max(
          180,
          Math.round(MAX_RAIN_DROPS * this.intensity),
        )
      : 0;

    this.activeSnowCount = (
      active && this.precipitation === 'Snow'
    )
      ? Math.max(
          120,
          Math.round(MAX_SNOW_FLAKES * this.intensity),
        )
      : 0;

    this.rainGeometry.setDrawRange(
      0,
      this.activeRainCount * 2,
    );
    this.snowGeometry.setDrawRange(
      0,
      this.activeSnowCount,
    );

    this.rainLines.visible = this.activeRainCount > 0;
    this.snowPoints.visible = this.activeSnowCount > 0;
    this.group.visible = active;

    this.rainMaterial.opacity = Math.min(
      0.9,
      0.5 + this.intensity * 0.4,
    );
    this.snowMaterial.opacity = Math.min(
      0.96,
      0.74 + this.intensity * 0.22,
    );
    this.snowMaterial.size = 0.68 + this.intensity * 0.7;

    this.rainMaterial.needsUpdate = true;
    this.snowMaterial.needsUpdate = true;
  }

  update(deltaSeconds: number): void {
    if (!this.group.visible) return;

    const dt = Math.min(0.05, Math.max(0, deltaSeconds));
    this.animationTime += dt;

    if (this.activeRainCount > 0) {
      this.updateRain(dt);
    }
    if (this.activeSnowCount > 0) {
      this.updateSnow(dt);
    }
  }

  dispose(): void {
    this.rainGeometry.dispose();
    this.rainMaterial.dispose();
    this.snowGeometry.dispose();
    this.snowMaterial.dispose();
  }

  private initializeRain(): void {
    for (let index = 0; index < MAX_RAIN_DROPS; index += 1) {
      const x = (seeded01(index, 1) - 0.5) * 120;
      const y = 6 + seeded01(index, 2) * 78;
      const z = (seeded01(index, 3) - 0.5) * 120;
      const length = 1.2 + seeded01(index, 4) * 2.1;

      const base = index * 6;
      this.rainPositions[base] = x;
      this.rainPositions[base + 1] = y;
      this.rainPositions[base + 2] = z;
      this.rainPositions[base + 3] = x;
      this.rainPositions[base + 4] = y - length;
      this.rainPositions[base + 5] = z;
    }
  }

  private initializeSnow(): void {
    for (let index = 0; index < MAX_SNOW_FLAKES; index += 1) {
      const base = index * 3;
      this.snowPositions[base] = (
        seeded01(index, 11) - 0.5
      ) * 120;
      this.snowPositions[base + 1] = (
        5 + seeded01(index, 12) * 72
      );
      this.snowPositions[base + 2] = (
        seeded01(index, 13) - 0.5
      ) * 120;
    }
  }

  private updateRain(dt: number): void {
    const fallSpeed = 38 + this.intensity * 58;
    const windX = (this.wind - 0.18) * 16;

    for (let index = 0; index < this.activeRainCount; index += 1) {
      const base = index * 6;
      let x = this.rainPositions[base];
      let y = this.rainPositions[base + 1];
      let z = this.rainPositions[base + 2];

      x += windX * dt;
      z += windX * 0.22 * dt;
      y -= fallSpeed * dt;

      if (y < -1.5) {
        x = (seeded01(index, 21) - 0.5) * 120;
        y = 62 + seeded01(index, 22) * 28;
        z = (seeded01(index, 23) - 0.5) * 120;
      }

      if (x > 64) x = -64;
      if (x < -64) x = 64;
      if (z > 64) z = -64;
      if (z < -64) z = 64;

      const length = 1.4 + this.intensity * 2.8;
      const slant = windX * 0.035;

      this.rainPositions[base] = x;
      this.rainPositions[base + 1] = y;
      this.rainPositions[base + 2] = z;
      this.rainPositions[base + 3] = x - slant;
      this.rainPositions[base + 4] = y - length;
      this.rainPositions[base + 5] = z - slant * 0.18;
    }

    const attribute = this.rainGeometry.getAttribute('position');
    if (attribute instanceof THREE.BufferAttribute) {
      attribute.needsUpdate = true;
    }
  }

  private updateSnow(dt: number): void {
    const fallSpeed = 3.4 + this.intensity * 5.2;
    const windX = (this.wind - 0.16) * 5.5;

    for (let index = 0; index < this.activeSnowCount; index += 1) {
      const base = index * 3;
      let x = this.snowPositions[base];
      let y = this.snowPositions[base + 1];
      let z = this.snowPositions[base + 2];

      const phase = this.animationTime * 0.8
        + index * 0.37;

      x += (
        windX
        + Math.sin(phase) * 0.75
      ) * dt;
      z += Math.cos(phase * 0.73) * 0.6 * dt;
      y -= fallSpeed * dt;

      if (y < -1) {
        x = (seeded01(index, 31) - 0.5) * 120;
        y = 58 + seeded01(index, 32) * 26;
        z = (seeded01(index, 33) - 0.5) * 120;
      }

      if (x > 64) x = -64;
      if (x < -64) x = 64;
      if (z > 64) z = -64;
      if (z < -64) z = 64;

      this.snowPositions[base] = x;
      this.snowPositions[base + 1] = y;
      this.snowPositions[base + 2] = z;
    }

    const attribute = this.snowGeometry.getAttribute('position');
    if (attribute instanceof THREE.BufferAttribute) {
      attribute.needsUpdate = true;
    }
  }
}
