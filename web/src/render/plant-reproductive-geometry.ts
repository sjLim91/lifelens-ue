import * as THREE from 'three';
import { AssetMeshBuilder, assetRandom } from './procedural-asset-kit';

/** Decorations share the shrub's local frame. Core phenology gates instances. */
export function createFlowerGeometry(seed: string): THREE.BufferGeometry {
  const b = new AssetMeshBuilder(), random = assetRandom(seed);
  for (let head = 0; head < 7; head++) {
    const angle = head * 2.39996, radius = 0.10 + random() * 0.20;
    const c = new THREE.Vector3(Math.cos(angle) * radius, 0.80 + random() * 0.50, Math.sin(angle) * radius);
    b.tube([c.clone().add(new THREE.Vector3(0, -0.15, 0)), c], [0.01, 0.006], 4, new THREE.Color(0x537444));
    const petals = head % 2 ? 5 : 6;
    for (let petal = 0; petal < petals; petal++) {
      const a = petal / petals * Math.PI * 2;
      const tip = c.clone().add(new THREE.Vector3(Math.cos(a) * 0.075, 0.025, Math.sin(a) * 0.075));
      const side = new THREE.Vector3(-Math.sin(a) * 0.025, 0, Math.cos(a) * 0.025);
      const color = new THREE.Color(head % 3 === 0 ? 0xf0c747 : head % 3 === 1 ? 0xe7b7d1 : 0xebe8d7);
      const i0 = b.vertex(c, color), i1 = b.vertex(tip.clone().add(side), color);
      const i2 = b.vertex(tip.clone().add(new THREE.Vector3(0, 0.025, 0)), color);
      const i3 = b.vertex(tip.clone().sub(side), color);
      b.triangle(i0, i1, i2); b.triangle(i0, i2, i3);
    }
  }
  return b.geometry();
}

export function createFruitGeometry(seed: string): THREE.BufferGeometry {
  const b = new AssetMeshBuilder(), random = assetRandom(seed), color = new THREE.Color(0x913e45);
  for (let berry = 0; berry < 18; berry++) {
    const a = random() * Math.PI * 2;
    const c = new THREE.Vector3(Math.cos(a) * 0.24, 0.60 + random() * 0.50, Math.sin(a) * 0.24);
    const top = b.vertex(c.clone().add(new THREE.Vector3(0, 0.04, 0)), color);
    const bottom = b.vertex(c.clone().sub(new THREE.Vector3(0, 0.04, 0)), color);
    const ring = Array.from({ length: 6 }, (_, i) => b.vertex(c.clone().add(new THREE.Vector3(Math.cos(i * Math.PI / 3) * 0.04, 0, Math.sin(i * Math.PI / 3) * 0.04)), color));
    for (let i = 0; i < 6; i++) { b.triangle(top, ring[i], ring[(i + 1) % 6]); b.triangle(bottom, ring[(i + 1) % 6], ring[i]); }
  }
  return b.geometry();
}
