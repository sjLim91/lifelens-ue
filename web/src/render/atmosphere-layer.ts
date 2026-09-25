import * as THREE from 'three';
import type { DynamicEnvironment } from '../runtime/core-types';

export interface AtmosphereState {
  daylight01: number;
  sunElevation: number;
}

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
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
  private minuteValue = 8 * 60;
  private environment: DynamicEnvironment | null = null;

  constructor(private readonly scene: THREE.Scene) {
    this.sun.position.set(-80, 160, 110);
    this.moon.position.set(80, 100, -120);

    this.group.add(this.hemisphere);
    this.group.add(this.sun);
    this.group.add(this.moon);
    this.scene.add(this.group);
    this.scene.fog = new THREE.FogExp2(0x0b1510, 0.0032);

    this.apply();
  }

  setSimulationMinute(minuteValue: number): AtmosphereState {
    this.minuteValue = Math.max(0, Number(minuteValue) || 0);
    return this.apply();
  }

  setEnvironment(environment: DynamicEnvironment | null): AtmosphereState {
    this.environment = environment;
    return this.apply();
  }

  dispose(): void {
    this.scene.remove(this.group);
  }

  private apply(): AtmosphereState {
    const minute = this.minuteValue % 1440;
    const dayAngle = ((minute / 1440) * Math.PI * 2) - (Math.PI * 0.5);
    const solar = Math.sin(dayAngle);
    const daylight01 = clamp01((solar + 0.18) / 1.18);
    const twilight = 1 - Math.abs((daylight01 * 2) - 1);

    const summary = this.environment?.summary;
    const storm = summary === 'Storm';
    const rain = summary === 'Rain';
    const snow = summary === 'Snow';
    const reportedCloud = clamp01(this.environment?.cloudCover01);
    const reportedPrecipitation = clamp01(
      this.environment?.precipitationIntensity01,
    );
    const precipitation = Math.max(
      reportedPrecipitation,
      storm ? 0.72 : rain || snow ? 0.34 : 0,
    );
    const cloud = Math.max(
      reportedCloud,
      storm ? 0.9 : rain || snow ? 0.72 : 0,
    );
    const reportedVisibility = this.environment?.available === false
      ? 1
      : clamp01(this.environment?.visibility01 ?? 1);
    const visibility = Math.min(
      reportedVisibility,
      storm ? 0.62 : rain ? 0.78 : snow ? 0.72 : 1,
    );
    const humidity = Math.max(
      clamp01(this.environment?.humidity01),
      rain || storm || snow ? 0.76 : 0,
    );

    const sunDistance = 220;
    this.sun.position.set(
      Math.cos(dayAngle) * sunDistance,
      Math.max(-30, solar * 190),
      Math.sin(dayAngle) * sunDistance * 0.55,
    );
    this.moon.position.copy(this.sun.position).multiplyScalar(-0.8);

    const cloudLightLoss = 1 - (cloud * 0.48 + precipitation * 0.24);
    this.sun.intensity = (0.08 + daylight01 * 2.15)
      * Math.max(0.3, cloudLightLoss);
    this.moon.intensity = 0.04
      + (1 - daylight01) * 0.34 * Math.max(0.55, 1 - cloud * 0.25);
    this.hemisphere.intensity = (
      0.22 + daylight01 * 1.35
    ) * Math.max(0.48, 1 - cloud * 0.35);

    const night = new THREE.Color(0x03080d);
    const dawn = new THREE.Color(0x59483d);
    const day = new THREE.Color(0x9dc5d9);
    const overcast = new THREE.Color(
      storm ? 0x313c43 : rain ? 0x526369 : snow ? 0x6e7a7d : 0x697b7d,
    );
    const sky = night.clone().lerp(day, daylight01);
    if (daylight01 > 0.02 && daylight01 < 0.72) {
      sky.lerp(dawn, twilight * 0.22);
    }
    sky.lerp(overcast, clamp01(cloud * 0.62 + precipitation * 0.24));
    this.scene.background = sky;

    if (this.scene.fog instanceof THREE.FogExp2) {
      const fogNight = new THREE.Color(0x07100d);
      const fogDay = new THREE.Color(0x6f8e83);
      const fogWeather = new THREE.Color(
        storm ? 0x465157 : rain ? 0x64736f : snow ? 0x899496 : 0x83918c,
      );
      this.scene.fog.color
        .copy(fogNight)
        .lerp(fogDay, daylight01)
        .lerp(
          fogWeather,
          clamp01((1 - visibility) * 0.72 + humidity * 0.12),
        );

      const dayFog = 0.0046 - daylight01 * 0.0019;
      const weatherFog = (1 - visibility) * 0.015
        + precipitation * 0.004
        + Math.max(0, humidity - 0.8) * 0.006;
      this.scene.fog.density = Math.min(0.024, dayFog + weatherFog);
    }

    return {
      daylight01,
      sunElevation: solar,
    };
  }
}
