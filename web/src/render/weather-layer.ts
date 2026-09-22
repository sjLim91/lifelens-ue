import * as THREE from 'three';
import type { DynamicEnvironment } from '../runtime/core-types';

const MAX_PARTICLES = 900;

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

export class WeatherLayer {
  readonly group = new THREE.Group();

  private readonly positions = new Float32Array(MAX_PARTICLES * 3);
  private readonly geometry = new THREE.BufferGeometry();
  private readonly material = new THREE.PointsMaterial({
    color: 0xc7deea,
    size: 0.34,
    transparent: true,
    opacity: 0.72,
    depthWrite: false,
    sizeAttenuation: true,
  });
  private readonly particles: THREE.Points;
  private activeCount = 0;
  private precipitation: 'None' | 'Rain' | 'Snow' = 'None';
  private intensity = 0;
  private wind = 0;

  constructor() {
    for (let index = 0; index < MAX_PARTICLES; index += 1) {
      const offset = index * 3;
      const a = ((index * 16807) % 2147483647) / 2147483647;
      const b = ((index * 48271 + 17) % 2147483647) / 2147483647;
      const c = ((index * 69621 + 31) % 2147483647) / 2147483647;
      this.positions[offset] = (a - 0.5) * 150;
      this.positions[offset + 1] = 8 + b * 85;
      this.positions[offset + 2] = (c - 0.5) * 150;
    }

    this.geometry.setAttribute(
      'position',
      new THREE.BufferAttribute(this.positions, 3),
    );
    this.geometry.setDrawRange(0, 0);
    this.particles = new THREE.Points(this.geometry, this.material);
    this.particles.frustumCulled = false;
    this.group.add(this.particles);
    this.group.visible = false;
  }

  setEnvironment(environment: DynamicEnvironment | null): void {
    this.precipitation = environment?.precipitationType ?? 'None';
    this.intensity = clamp01(environment?.precipitationIntensity01);
    this.wind = clamp01(environment?.windIntensity01);

    const active = this.precipitation !== 'None' && this.intensity >= 0.03;
    this.activeCount = active
      ? Math.max(90, Math.round(MAX_PARTICLES * this.intensity))
      : 0;
    this.geometry.setDrawRange(0, this.activeCount);
    this.group.visible = active;

    if (this.precipitation === 'Snow') {
      this.material.color.set(0xe8f0f3);
      this.material.size = 0.72;
      this.material.opacity = 0.82;
    } else {
      this.material.color.set(0xaecfdf);
      this.material.size = 0.28;
      this.material.opacity = 0.66 + this.intensity * 0.2;
    }
    this.material.needsUpdate = true;
  }

  update(deltaSeconds: number): void {
    if (!this.group.visible || this.activeCount <= 0) return;

    const dt = Math.min(0.05, Math.max(0, deltaSeconds));
    const fallSpeed = this.precipitation === 'Snow'
      ? 5 + this.intensity * 4
      : 34 + this.intensity * 44;
    const drift = (this.wind - 0.25) * (
      this.precipitation === 'Snow' ? 7 : 13
    );

    for (let index = 0; index < this.activeCount; index += 1) {
      const offset = index * 3;
      this.positions[offset] += drift * dt;
      this.positions[offset + 1] -= fallSpeed * dt;

      if (this.positions[offset + 1] < -2) {
        this.positions[offset + 1] = 72 + ((index * 37) % 23);
        this.positions[offset] = (((index * 97) % 1000) / 1000 - 0.5) * 150;
      }

      if (this.positions[offset] > 82) this.positions[offset] = -82;
      if (this.positions[offset] < -82) this.positions[offset] = 82;
    }

    const attribute = this.geometry.getAttribute('position');
    if (attribute instanceof THREE.BufferAttribute) {
      attribute.needsUpdate = true;
    }
  }

  dispose(): void {
    this.geometry.dispose();
    this.material.dispose();
  }
}
