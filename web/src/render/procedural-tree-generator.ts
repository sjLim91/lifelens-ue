import * as THREE from 'three';
import { AssetMeshBuilder, assetRandom } from './procedural-asset-kit';

export const TREE_TEMPLATES = [
  { id: 'oak', branches: 7, spread: 0.29, trunk: 0.65, radius: 0.034, lean: 0.065, evergreen: false },
  { id: 'birch', branches: 6, spread: 0.19, trunk: 0.91, radius: 0.021, lean: 0.025, evergreen: false },
  { id: 'pine', branches: 25, spread: 0.25, trunk: 1, radius: 0.028, lean: 0.018, evergreen: true },
] as const;
export type TreeSpecies = typeof TREE_TEMPLATES[number]['id'];
export interface TreeGeometry { bark: THREE.BufferGeometry; foliage: THREE.BufferGeometry; evergreen: boolean }

const geometryCache = new Map<string, { value: TreeGeometry; references: number }>();
export function acquireTreeGeometry(species: TreeSpecies, seed: string, lowDetail: boolean) {
  const key = `${species}:${seed}:${lowDetail}`;
  let entry = geometryCache.get(key);
  if (!entry) {
    entry = { value: generateTree(species, seed, lowDetail), references: 0 };
    geometryCache.set(key, entry);
  }
  entry.references++;
  let released = false;
  return { geometry: entry.value, release: () => {
    if (released) return; released = true;
    if (--entry!.references === 0) {
      entry!.value.bark.dispose(); entry!.value.foliage.dispose(); geometryCache.delete(key);
    }
  } };
}

/** Unit-height trees. Growth/stock scale is supplied by the authoritative projection. */
export function generateTree(species: TreeSpecies, seed: string, lowDetail = false): TreeGeometry {
  const template = TREE_TEMPLATES.find(t => t.id === species)!;
  const random = assetRandom(`${species}:${seed}`);
  const bark = new AssetMeshBuilder(), foliage = new AssetMeshBuilder();
  const barkColor = new THREE.Color(species === 'birch' ? 0xc4bfa7 : species === 'pine' ? 0x625044 : 0x69513a);
  const lean = (random() - 0.5) * template.lean;
  const trunk = Array.from({ length: 6 }, (_, i) => {
    const y = i / 5 * template.trunk;
    return new THREE.Vector3(lean * y + Math.sin(i * 1.6) * template.lean * 0.15, y, Math.sin(i * 0.8) * template.lean * 0.25);
  });
  trunk[0].set(0, 0, 0);
  bark.tube(trunk, trunk.map((_, i) => template.radius * (1 - i / 5 * 0.85)), lowDetail ? 5 : 8, barkColor);
  // Buttress roots stay above the shared terrain surface; no hidden collider.
  if (!lowDetail) for (let i = 0; i < 4; i++) {
    const a = i * Math.PI / 2 + assetRandom(`${seed}:root:${i}`)();
    bark.tube([new THREE.Vector3(Math.cos(a) * 0.065, 0.002, Math.sin(a) * 0.065), new THREE.Vector3(0, 0.09, 0)],
      [template.radius * 0.28, template.radius * 0.65], 5, barkColor);
  }

  const leaves = (tip: THREE.Vector3, spread: number, yaw: number) => {
    const leafRandom = assetRandom(`${seed}:leaf:${tip.x}:${tip.y}:${tip.z}`);
    const count = lowDetail ? 8 : 26;
    for (let i = 0; i < count; i++) {
      const a = leafRandom() * Math.PI * 2, radius = Math.sqrt(leafRandom()) * spread;
      const center = tip.clone().add(new THREE.Vector3(Math.cos(a) * radius,
        (leafRandom() - 0.5) * spread * 0.9, Math.sin(a) * radius));
      center.y = THREE.MathUtils.clamp(center.y, 0.16, 0.97);
      const length = (lowDetail ? 0.155 : 0.088) * (0.75 + leafRandom() * 0.5);
      foliage.leaf(center, template.evergreen ? length * 1.8 : length,
        template.evergreen ? length * 0.62 : length * 0.8,
        yaw + leafRandom() * 2, (leafRandom() - 0.5) * 0.9,
        new THREE.Color().setScalar(0.75 + leafRandom() * 0.35), leafRandom());
    }
  };
  const branch = (start: THREE.Vector3, end: THREE.Vector3, radius: number, depth: number, yaw: number) => {
    const mid = start.clone().lerp(end, 0.52); mid.y -= 0.012;
    bark.tube([start, mid, end], [radius, radius * 0.62, radius * 0.19], lowDetail ? 4 : 5, barkColor);
    if (depth === 0) { leaves(end, template.evergreen ? 0.073 : 0.105, yaw); return; }
    for (let i = 0; i < 2; i++) {
      const a = yaw + (i === 0 ? -0.75 : 0.75) + (random() - 0.5) * 0.5;
      const length = start.distanceTo(end) * (0.45 + random() * 0.25);
      const next = end.clone().add(new THREE.Vector3(Math.cos(a) * length,
        template.evergreen ? 0.005 : length * 0.75, Math.sin(a) * length));
      next.y = Math.min(0.93, next.y);
      branch(end, next, radius * 0.47, depth - 1, a);
    }
    leaves(end, template.evergreen ? 0.07 : 0.10, yaw);
  };
  for (let i = 0; i < template.branches; i++) {
    const fraction = template.evergreen ? Math.floor(i / 5) / 5 : i / template.branches;
    const y = template.evergreen ? 0.23 + fraction * 0.65 : 0.27 + fraction * template.trunk * 0.6;
    const yaw = template.evergreen ? i % 5 * Math.PI * 2 / 5 + Math.floor(i / 5) * 0.55 : i * 2.39996 + random() * 0.6;
    const length = template.spread * (template.evergreen ? 1 - fraction * 0.75 : 0.72 + random() * 0.28);
    const start = new THREE.Vector3(lean * y, y, 0);
    const end = start.clone().add(new THREE.Vector3(Math.cos(yaw) * length,
      template.evergreen ? 0.03 : length * (species === 'birch' ? 1.15 : 0.8), Math.sin(yaw) * length));
    branch(start, end, template.radius * 0.47 * (1 - fraction * 0.5), 1, yaw);
  }
  leaves(trunk[5], 0.085, 0);
  return { bark: bark.geometry(), foliage: foliage.geometry(), evergreen: template.evergreen };
}

export function createShrubGeometry(seed: string): THREE.BufferGeometry {
  const random = assetRandom(seed), b = new AssetMeshBuilder();
  // Normalized centered bounds keep existing shrub grounding/stock scaling.
  for (let stem = 0; stem < 7; stem++) {
    const yaw = stem * 2.39996, height = 0.55 + random() * 0.45;
    for (let leaf = 0; leaf < 12; leaf++) {
      const t = leaf / 12, radius = Math.sin(t * Math.PI) * 0.7;
      const a = yaw + leaf * 1.3;
      b.leaf(new THREE.Vector3(Math.cos(a) * radius, -0.68 + height * t * 1.45, Math.sin(a) * radius),
        0.27, 0.15, a, 0.25, new THREE.Color().setScalar(0.8 + random() * 0.3), random());
    }
  }
  return b.geometry();
}
