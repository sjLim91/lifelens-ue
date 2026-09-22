import * as THREE from 'three';

export interface AtmosphereState {
  daylight01: number;
  sunElevation: number;
}

function clamp01(value: number): number {
  return Math.max(0, Math.min(1, value));
}

export class AtmosphereLayer {
  readonly group = new THREE.Group();

  private readonly hemisphere = new THREE.HemisphereLight(
    0xdcecff,
    0x233126,
    1.2,
  );
  private readonly sun = new THREE.DirectionalLight(0xfff3d6, 1.8);
  private readonly moon = new THREE.DirectionalLight(0xa9c5ff, 0.14);

  constructor(private readonly scene: THREE.Scene) {
    this.sun.position.set(-80, 160, 110);
    this.moon.position.set(80, 100, -120);

    this.group.add(this.hemisphere);
    this.group.add(this.sun);
    this.group.add(this.moon);
    this.scene.add(this.group);
    this.scene.fog = new THREE.FogExp2(0x0b1510, 0.0032);

    this.setSimulationMinute(8 * 60);
  }

  setSimulationMinute(minuteValue: number): AtmosphereState {
    const minute = Math.max(0, Number(minuteValue) || 0) % 1440;
    const dayAngle = ((minute / 1440) * Math.PI * 2) - (Math.PI * 0.5);
    const solar = Math.sin(dayAngle);
    const daylight01 = clamp01((solar + 0.18) / 1.18);
    const twilight = 1 - Math.abs((daylight01 * 2) - 1);

    const sunDistance = 220;
    this.sun.position.set(
      Math.cos(dayAngle) * sunDistance,
      Math.max(-30, solar * 190),
      Math.sin(dayAngle) * sunDistance * 0.55,
    );
    this.moon.position.copy(this.sun.position).multiplyScalar(-0.8);

    this.sun.intensity = 0.08 + daylight01 * 2.15;
    this.moon.intensity = 0.04 + (1 - daylight01) * 0.34;
    this.hemisphere.intensity = 0.22 + daylight01 * 1.35;

    const night = new THREE.Color(0x03080d);
    const dawn = new THREE.Color(0x59483d);
    const day = new THREE.Color(0x9dc5d9);
    const sky = night.clone().lerp(day, daylight01);
    if (daylight01 > 0.02 && daylight01 < 0.72) {
      sky.lerp(dawn, twilight * 0.22);
    }
    this.scene.background = sky;

    if (this.scene.fog instanceof THREE.FogExp2) {
      const fogNight = new THREE.Color(0x07100d);
      const fogDay = new THREE.Color(0x6f8e83);
      this.scene.fog.color.copy(fogNight).lerp(fogDay, daylight01);
      this.scene.fog.density = 0.0046 - daylight01 * 0.0019;
    }

    return {
      daylight01,
      sunElevation: solar,
    };
  }

  dispose(): void {
    this.scene.remove(this.group);
  }
}
