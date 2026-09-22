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
    0x27382d,
    1.2,
  );
  private readonly sun = new THREE.DirectionalLight(0xfff0cf, 1.8);
  private readonly moon = new THREE.DirectionalLight(0xa9c5ff, 0.24);
  private readonly sunDisc = new THREE.Sprite(
    new THREE.SpriteMaterial({
      color: 0xffe2a1,
      transparent: true,
      opacity: 0.86,
      depthTest: false,
      depthWrite: false,
    }),
  );
  private readonly moonDisc = new THREE.Sprite(
    new THREE.SpriteMaterial({
      color: 0xcad7ff,
      transparent: true,
      opacity: 0.72,
      depthTest: false,
      depthWrite: false,
    }),
  );
  private readonly cloudGeometry = new THREE.SphereGeometry(1, 10, 7);
  private readonly cloudMaterial = new THREE.MeshBasicMaterial({
    color: 0xe1e5e2,
    transparent: true,
    opacity: 0,
    depthWrite: false,
    fog: false,
  });
  private readonly cloudClusters: THREE.Group[] = [];
  private cloudTime = 0;
  private minuteValue = 8 * 60;
  private environment: DynamicEnvironment | null = null;

  constructor(private readonly scene: THREE.Scene) {
    this.sun.position.set(-80, 160, 110);
    this.sun.castShadow = true;
    this.sun.shadow.mapSize.set(1024, 1024);
    this.sun.shadow.camera.near = 8;
    this.sun.shadow.camera.far = 420;
    this.sun.shadow.camera.left = -110;
    this.sun.shadow.camera.right = 110;
    this.sun.shadow.camera.top = 110;
    this.sun.shadow.camera.bottom = -110;
    this.sun.shadow.bias = -0.00035;
    this.sun.shadow.normalBias = 0.035;
    this.moon.position.set(80, 100, -120);

    this.sunDisc.scale.set(18, 18, 1);
    this.moonDisc.scale.set(10, 10, 1);
    this.sunDisc.renderOrder = -10;
    this.moonDisc.renderOrder = -10;

    this.group.add(this.hemisphere);
    this.group.add(this.sun);
    this.group.add(this.moon);
    this.group.add(this.sunDisc);
    this.group.add(this.moonDisc);
    this.createCloudField();
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

  update(deltaSeconds: number): void {
    const dt = Math.min(0.05, Math.max(0, deltaSeconds));
    this.cloudTime += dt;
    const cloud = clamp01(this.environment?.cloudCover01);
    const wind = clamp01(this.environment?.windIntensity01);
    if (cloud <= 0.02) return;

    const drift = 0.8 + wind * 3.6;
    for (let index = 0; index < this.cloudClusters.length; index += 1) {
      const cluster = this.cloudClusters[index];
      cluster.position.x += dt * drift * (0.65 + (index % 4) * 0.08);
      cluster.position.z += dt * drift * 0.24;
      cluster.position.y += Math.sin(
        this.cloudTime * 0.17 + index * 1.37,
      ) * dt * 0.08;
      if (cluster.position.x > 410) cluster.position.x = -410;
      if (cluster.position.z > 300) cluster.position.z = -300;
    }
  }

  dispose(): void {
    this.sunDisc.material.dispose();
    this.moonDisc.material.dispose();
    this.cloudGeometry.dispose();
    this.cloudMaterial.dispose();
    this.scene.remove(this.group);
  }

  private createCloudField(): void {
    for (let index = 0; index < 18; index += 1) {
      const cluster = new THREE.Group();
      const angle = (index / 18) * Math.PI * 2;
      const radius = 220 + (index % 5) * 36;
      cluster.position.set(
        Math.cos(angle) * radius,
        92 + (index % 4) * 15,
        Math.sin(angle) * radius * 0.72,
      );
      const clusterScale = 14 + (index % 6) * 2.4;
      cluster.scale.set(
        clusterScale,
        clusterScale * 0.38,
        clusterScale * 0.78,
      );

      const puffCount = 3 + (index % 3);
      for (let puffIndex = 0; puffIndex < puffCount; puffIndex += 1) {
        const puff = new THREE.Mesh(
          this.cloudGeometry,
          this.cloudMaterial,
        );
        puff.position.set(
          (puffIndex - (puffCount - 1) * 0.5) * 0.82,
          (puffIndex % 2) * 0.28,
          ((puffIndex * 37) % 3 - 1) * 0.34,
        );
        puff.scale.set(
          1 + (puffIndex % 2) * 0.38,
          0.72 + (puffIndex % 3) * 0.12,
          0.9 + ((puffIndex + 1) % 2) * 0.34,
        );
        cluster.add(puff);
      }

      cluster.visible = false;
      this.cloudClusters.push(cluster);
      this.group.add(cluster);
    }
  }

  private apply(): AtmosphereState {
    const minute = this.minuteValue % 1440;
    const dayAngle = ((minute / 1440) * Math.PI * 2) - (Math.PI * 0.5);
    const solar = Math.sin(dayAngle);
    const daylight01 = clamp01((solar + 0.18) / 1.18);
    const twilight = 1 - Math.abs((daylight01 * 2) - 1);

    const cloud = clamp01(this.environment?.cloudCover01);
    const precipitation = clamp01(
      this.environment?.precipitationIntensity01,
    );
    const visibility = this.environment?.available === false
      ? 1
      : clamp01(this.environment?.visibility01 ?? 1);
    const humidity = clamp01(this.environment?.humidity01);
    const storm = this.environment?.summary === 'Storm';

    const sunDistance = 220;
    this.sun.position.set(
      Math.cos(dayAngle) * sunDistance,
      Math.max(-30, solar * 190),
      Math.sin(dayAngle) * sunDistance * 0.55,
    );
    this.moon.position.copy(this.sun.position).multiplyScalar(-0.8);
    this.sunDisc.position.copy(this.sun.position).normalize().multiplyScalar(780);
    this.moonDisc.position.copy(this.moon.position).normalize().multiplyScalar(760);

    const cloudLightLoss = 1 - (cloud * 0.48 + precipitation * 0.24);
    this.sun.intensity = (0.08 + daylight01 * 2.15)
      * Math.max(0.3, cloudLightLoss);
    this.moon.intensity = 0.1
      + (1 - daylight01) * 0.52 * Math.max(0.58, 1 - cloud * 0.25);
    const sunMaterial = this.sunDisc.material;
    const moonMaterial = this.moonDisc.material;
    sunMaterial.opacity = Math.max(
      0,
      daylight01 * (0.92 - cloud * 0.62 - precipitation * 0.16),
    );
    moonMaterial.opacity = Math.max(
      0,
      (1 - daylight01) * (0.76 - cloud * 0.35),
    );
    this.sunDisc.visible = sunMaterial.opacity > 0.02;
    this.moonDisc.visible = moonMaterial.opacity > 0.02;
    this.hemisphere.intensity = (
      0.38 + daylight01 * 1.22
    ) * Math.max(0.54, 1 - cloud * 0.34);

    const night = new THREE.Color(0x0b1420);
    const dawn = new THREE.Color(0x6a5040);
    const day = new THREE.Color(0xa8cad9);
    const overcast = new THREE.Color(storm ? 0x38434a : 0x697b7d);
    const sky = night.clone().lerp(day, daylight01);
    if (daylight01 > 0.02 && daylight01 < 0.72) {
      sky.lerp(dawn, twilight * 0.22);
    }
    sky.lerp(overcast, clamp01(cloud * 0.62 + precipitation * 0.24));
    this.scene.background = sky;

    const visibleClouds = Math.round(cloud * this.cloudClusters.length);
    const cloudColor = new THREE.Color(0xe1e5e2)
      .lerp(
        new THREE.Color(storm ? 0x6d777b : 0xa8b2b0),
        clamp01(cloud * 0.72 + precipitation * 0.25),
      );
    this.cloudMaterial.color.copy(cloudColor);
    this.cloudMaterial.opacity = Math.max(
      0,
      Math.min(
        0.56,
        0.12 + cloud * 0.32 + precipitation * 0.12,
      ),
    );
    for (let index = 0; index < this.cloudClusters.length; index += 1) {
      const cluster = this.cloudClusters[index];
      cluster.visible = index < visibleClouds;
      const depthScale = 0.9 + cloud * 0.22 + (index % 4) * 0.025;
      cluster.scale.multiplyScalar(
        depthScale / Math.max(0.001, Number(cluster.userData.lastCloudScale) || 1),
      );
      cluster.userData.lastCloudScale = depthScale;
    }

    if (this.scene.fog instanceof THREE.FogExp2) {
      const fogNight = new THREE.Color(0x101c20);
      const fogDay = new THREE.Color(0x728f83);
      const fogWeather = new THREE.Color(
        storm ? 0x495458 : 0x83918c,
      );
      this.scene.fog.color
        .copy(fogNight)
        .lerp(fogDay, daylight01)
        .lerp(
          fogWeather,
          clamp01((1 - visibility) * 0.72 + humidity * 0.12),
        );

      const dayFog = 0.0042 - daylight01 * 0.0017;
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
