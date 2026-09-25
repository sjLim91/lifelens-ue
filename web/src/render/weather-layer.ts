import * as THREE from 'three';
import type { DynamicEnvironment } from '../runtime/core-types';

const MAX_RAIN_STREAKS = 1100;
const MAX_SNOW_PARTICLES = 700;
const WEATHER_SPAN = 120;
const WEATHER_TOP = 72;

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function seeded01(index: number, salt: number): number {
  let value = Math.imul(index + 1, 1103515245) + salt * 12345;
  value ^= value >>> 16;
  return (value >>> 0) / 4294967295;
}

export class WeatherLayer {
  readonly group = new THREE.Group();

  private readonly rainPositions =
    new Float32Array(MAX_RAIN_STREAKS * 2 * 3);
  private readonly rainGeometry = new THREE.BufferGeometry();
  private readonly rainMaterial = new THREE.LineBasicMaterial({
    color: 0xb9d7e7,
    transparent: true,
    opacity: 0.76,
    depthWrite: false,
  });
  private readonly rain: THREE.LineSegments;

  private readonly snowPositions =
    new Float32Array(MAX_SNOW_PARTICLES * 3);
  private readonly snowGeometry = new THREE.BufferGeometry();
  private readonly snowMaterial = new THREE.PointsMaterial({
    color: 0xf0f5f7,
    size: 0.62,
    transparent: true,
    opacity: 0.86,
    depthWrite: false,
    sizeAttenuation: true,
  });
  private readonly snow: THREE.Points;

  private precipitation: 'None' | 'Rain' | 'Snow' = 'None';
  private intensity = 0;
  private wind = 0;
  private rainCount = 0;
  private snowCount = 0;

  constructor() {
    for (let index = 0; index < MAX_RAIN_STREAKS; index += 1) {
      this.resetRainStreak(index, seeded01(index, 3) * WEATHER_TOP);
    }
    this.rainGeometry.setAttribute(
      'position',
      new THREE.BufferAttribute(this.rainPositions, 3),
    );
    this.rainGeometry.setDrawRange(0, 0);
    this.rain = new THREE.LineSegments(
      this.rainGeometry,
      this.rainMaterial,
    );
    this.rain.frustumCulled = false;
    this.rain.renderOrder = 5;

    for (let index = 0; index < MAX_SNOW_PARTICLES; index += 1) {
      const offset = index * 3;
      this.snowPositions[offset] =
        (seeded01(index, 11) - 0.5) * WEATHER_SPAN;
      this.snowPositions[offset + 1] =
        5 + seeded01(index, 13) * WEATHER_TOP;
      this.snowPositions[offset + 2] =
        (seeded01(index, 17) - 0.5) * WEATHER_SPAN;
    }
    this.snowGeometry.setAttribute(
      'position',
      new THREE.BufferAttribute(this.snowPositions, 3),
    );
    this.snowGeometry.setDrawRange(0, 0);
    this.snow = new THREE.Points(
      this.snowGeometry,
      this.snowMaterial,
    );
    this.snow.frustumCulled = false;
    this.snow.renderOrder = 5;

    this.group.add(this.rain);
    this.group.add(this.snow);
    this.group.visible = false;
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    const summary = environment?.summary;
    const declaredType = environment?.precipitationType ?? 'None';
    const summaryType: 'None' | 'Rain' | 'Snow' =
      summary === 'Snow'
        ? 'Snow'
        : summary === 'Rain' || summary === 'Storm'
          ? 'Rain'
          : 'None';

    this.precipitation =
      declaredType !== 'None' ? declaredType : summaryType;

    const reportedIntensity = clamp01(
      environment?.precipitationIntensity01,
    );
    const visibleFloor =
      summary === 'Storm'
        ? 0.72
        : summary === 'Rain' || summary === 'Snow'
          ? 0.34
          : this.precipitation !== 'None'
            ? 0.18
            : 0;

    this.intensity = this.precipitation === 'None'
      ? 0
      : Math.max(reportedIntensity, visibleFloor);
    this.wind = clamp01(environment?.windIntensity01);

    this.rainCount = this.precipitation === 'Rain'
      ? Math.max(
        220,
        Math.round(MAX_RAIN_STREAKS * this.intensity),
      )
      : 0;
    this.snowCount = this.precipitation === 'Snow'
      ? Math.max(
        140,
        Math.round(MAX_SNOW_PARTICLES * this.intensity),
      )
      : 0;

    this.rainGeometry.setDrawRange(0, this.rainCount * 2);
    this.snowGeometry.setDrawRange(0, this.snowCount);
    this.rain.visible = this.rainCount > 0;
    this.snow.visible = this.snowCount > 0;
    this.group.visible = this.rain.visible || this.snow.visible;

    this.rainMaterial.opacity = 0.62 + this.intensity * 0.3;
    this.rainMaterial.color.set(
      summary === 'Storm' ? 0xaec8d6 : 0xc3dce8,
    );
    this.snowMaterial.opacity = 0.76 + this.intensity * 0.18;
  }

  setFocus(worldX: number, worldZ: number): void {
    this.group.position.set(
      Number(worldX) || 0,
      0,
      Number(worldZ) || 0,
    );
  }

  update(deltaSeconds: number): void {
    if (!this.group.visible) return;

    const dt = Math.min(0.05, Math.max(0, deltaSeconds));
    if (this.rainCount > 0) this.updateRain(dt);
    if (this.snowCount > 0) this.updateSnow(dt);
  }

  dispose(): void {
    this.rainGeometry.dispose();
    this.rainMaterial.dispose();
    this.snowGeometry.dispose();
    this.snowMaterial.dispose();
  }

  private updateRain(dt: number): void {
    const fallSpeed = 38 + this.intensity * 42;
    const drift = (this.wind - 0.25) * 15;

    for (let index = 0; index < this.rainCount; index += 1) {
      const offset = index * 6;
      this.rainPositions[offset] += drift * dt;
      this.rainPositions[offset + 3] += drift * dt;
      this.rainPositions[offset + 1] -= fallSpeed * dt;
      this.rainPositions[offset + 4] -= fallSpeed * dt;

      if (this.rainPositions[offset + 1] < -3) {
        this.resetRainStreak(
          index,
          WEATHER_TOP + seeded01(index, 23) * 18,
        );
      }

      const x = this.rainPositions[offset];
      if (x > WEATHER_SPAN * 0.55) {
        this.shiftRainStreakX(index, -WEATHER_SPAN);
      } else if (x < -WEATHER_SPAN * 0.55) {
        this.shiftRainStreakX(index, WEATHER_SPAN);
      }
    }

    const attribute = this.rainGeometry.getAttribute('position');
    if (attribute instanceof THREE.BufferAttribute) {
      attribute.needsUpdate = true;
    }
  }

  private updateSnow(dt: number): void {
    const fallSpeed = 4.5 + this.intensity * 5;
    const drift = (this.wind - 0.2) * 7;

    for (let index = 0; index < this.snowCount; index += 1) {
      const offset = index * 3;
      this.snowPositions[offset] += drift * dt;
      this.snowPositions[offset + 1] -= fallSpeed * dt;

      if (this.snowPositions[offset + 1] < -2) {
        this.snowPositions[offset] =
          (seeded01(index, 29) - 0.5) * WEATHER_SPAN;
        this.snowPositions[offset + 1] =
          WEATHER_TOP + seeded01(index, 31) * 16;
        this.snowPositions[offset + 2] =
          (seeded01(index, 37) - 0.5) * WEATHER_SPAN;
      }

      if (this.snowPositions[offset] > WEATHER_SPAN * 0.55) {
        this.snowPositions[offset] -= WEATHER_SPAN;
      } else if (
        this.snowPositions[offset] < -WEATHER_SPAN * 0.55
      ) {
        this.snowPositions[offset] += WEATHER_SPAN;
      }
    }

    const attribute = this.snowGeometry.getAttribute('position');
    if (attribute instanceof THREE.BufferAttribute) {
      attribute.needsUpdate = true;
    }
  }

  private resetRainStreak(index: number, y: number): void {
    const offset = index * 6;
    const x = (seeded01(index, 41) - 0.5) * WEATHER_SPAN;
    const z = (seeded01(index, 43) - 0.5) * WEATHER_SPAN;
    const length = 1.2 + seeded01(index, 47) * 2.2;

    this.rainPositions[offset] = x;
    this.rainPositions[offset + 1] = y;
    this.rainPositions[offset + 2] = z;
    this.rainPositions[offset + 3] = x;
    this.rainPositions[offset + 4] = y - length;
    this.rainPositions[offset + 5] = z;
  }

  private shiftRainStreakX(index: number, deltaX: number): void {
    const offset = index * 6;
    this.rainPositions[offset] += deltaX;
    this.rainPositions[offset + 3] += deltaX;
  }
}
