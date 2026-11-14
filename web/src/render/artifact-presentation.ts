import * as THREE from 'three';
import type { PhysicalArtifact, ResidentPresentationDirective } from '../runtime/core-types';

export function activeArtifact(p: ResidentPresentationDirective | null | undefined): PhysicalArtifact | null {
  const a = p?.artifactTool;
  if (!p?.active || p.kind !== 'Civilization' || p.phase !== 'Interacting'
    || p.civilizationIntent !== 'Gather' || !p.contextActionToken || p.contextActionToken === '0'
    || !a || a.durability <= .02 || !a.parts.length || a.parts.length > 4
    || a.parts.some(part => !Number.isInteger(part.shape) || part.shape < 0 || part.shape > 5
      || !Number.isInteger(part.size) || part.size < 1 || part.size > 3)) return null;
  return a;
}

/** Consume only explicit Core structural parts; these meshes confer no ability. */
export function createArtifactProp(a: PhysicalArtifact): THREE.Group {
  const group = new THREE.Group();
  const colors: Record<string, number> = { Wood: 0x715038, Fiber: 0xb69c6b, Hide: 0x96744c,
    Clay: 0xad7654, Flint: 0x65716d, Bronze: 0xa58648, CopperMetal: 0xb5764f };
  for (const [i, part] of a.parts.entries()) {
    let geometry: THREE.BufferGeometry;
    switch (part.shape) {
      case 0: geometry = new THREE.CylinderGeometry(.013, .012, .18 * part.size, 6); break;
      case 1: geometry = new THREE.OctahedronGeometry(.07 * part.size); geometry.scale(1, .7, .18); break;
      case 2: geometry = new THREE.ConeGeometry(.035, .16 * part.size, 5); break;
      case 3: geometry = new THREE.SphereGeometry(.055 * part.size, 8, 5, 0, Math.PI * 2, Math.PI / 2, Math.PI / 2); break;
      case 4: geometry = new THREE.CylinderGeometry(.07 * part.size, .07 * part.size, .015, 8); break;
      default: geometry = new THREE.TorusGeometry(.028, .007, 4, 10);
    }
    const mesh = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({ color: colors[part.material] ?? 0x858a89, roughness: .85 }));
    mesh.position.y = part.shape === 0 ? -.06 : i * .025;
    mesh.castShadow = true; group.add(mesh);
  }
  return group;
}
