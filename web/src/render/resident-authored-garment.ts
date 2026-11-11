import * as THREE from 'three';
import type { ResidentCivilizationItem } from '../runtime/core-types';
import type { ResidentAppearanceProfile } from './resident-appearance';

// Only Core's explicitly observed wornGarment creates clothing. The shape
// comes from the resident's own skinned body, never an unbound cylinder.
export const AUTHORED_GARMENT_GROUP = 'LifeLensResidentAuthoredGarment';

export function validWornPrimitiveWrap(
  garment: ResidentCivilizationItem | null | undefined,
): garment is ResidentCivilizationItem {
  return garment?.item === 'PrimitiveWrap'
    && garment.material === 'Fiber'
    && typeof garment.quantity === 'number' && Number.isFinite(garment.quantity)
    && garment.quantity > 0
    && typeof garment.quality === 'number' && Number.isFinite(garment.quality)
    && garment.quality >= 0 && garment.quality <= 1
    && typeof garment.durability === 'number' && Number.isFinite(garment.durability)
    && garment.durability > 0 && garment.durability <= 1;
}

/** Extract only the chest, waist and pelvis surface of the actual resident.
 * Copy the source skin indices/weights, so this primitive fiber tunic follows
 * spine and hips through walking, sitting and interaction poses. No rigid
 * torso tube, separate belt ring, invented equipment or shader-only hiding. */
function createBodyFittedWrap(
  body: THREE.SkinnedMesh,
  quality: number,
  durability: number,
  seed: number,
): THREE.BufferGeometry | null {
  const source = body.geometry;
  const positions = source.getAttribute('position');
  const normals = source.getAttribute('normal');
  const joints = source.getAttribute('skinIndex');
  const weights = source.getAttribute('skinWeight');
  if (!positions || !normals || !joints || !weights
    || joints.count !== positions.count || weights.count !== positions.count)
    return null;

  const torsoBones = new Set<number>();
  body.skeleton.bones.forEach((bone, index) => {
    if (/^(pelvis|spine_0[123])$/i.test(bone.name)) torsoBones.add(index);
  });
  if (torsoBones.size < 2) return null;

  const influence = new Float32Array(positions.count);
  for (let i = 0; i < positions.count; i++) {
    let weight = 0;
    for (let j = 0; j < 4; j++)
      if (torsoBones.has(joints.getComponent(i, j)))
        weight += weights.getComponent(i, j);
    influence[i] = weight;
  }

  const srcIndex = source.getIndex();
  const faceVertices = srcIndex ? srcIndex.count : positions.count;
  const remap = new Map<number, number>();
  const outPositions: number[] = [], outNormals: number[] = [];
  const outJoints: number[] = [], outWeights: number[] = [];
  const outTriangles: number[] = [];
  const sway = (seed % 101) * 0.061;
  const offsetBase = 0.004 + quality * 0.0015;

  const addVertex = (old: number): number => {
    const present = remap.get(old);
    if (present !== undefined) return present;
    const next = outPositions.length / 3;
    remap.set(old, next);
    const x = positions.getX(old), y = positions.getY(old), z = positions.getZ(old);
    const nx = normals.getX(old), ny = normals.getY(old), nz = normals.getZ(old);
    // Subtle deterministic cloth thickness; no free-floating cloth strips.
    const fold = 0.0015 * (1 - quality) * Math.sin(x * 31 + y * 17 + sway);
    const lift = offsetBase + fold + (1 - durability) * 0.001;
    outPositions.push(x + nx * lift, y + ny * lift, z + nz * lift);
    outNormals.push(nx, ny, nz);
    for (let j = 0; j < 4; j++) {
      outJoints.push(joints.getComponent(old, j));
      outWeights.push(weights.getComponent(old, j));
    }
    return next;
  };

  for (let i = 0; i + 2 < faceVertices; i += 3) {
    const a = srcIndex ? srcIndex.getX(i) : i;
    const b = srcIndex ? srcIndex.getX(i + 1) : i + 1;
    const c = srcIndex ? srcIndex.getX(i + 2) : i + 2;
    if (influence[a] < .6 || influence[b] < .6 || influence[c] < .6)
      continue;
    outTriangles.push(addVertex(a), addVertex(b), addVertex(c));
  }
  if (outTriangles.length < 36) return null;

  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('position', new THREE.Float32BufferAttribute(outPositions, 3));
  geometry.setAttribute('normal', new THREE.Float32BufferAttribute(outNormals, 3));
  geometry.setAttribute('skinIndex', new THREE.Uint16BufferAttribute(outJoints, 4));
  geometry.setAttribute('skinWeight', new THREE.Float32BufferAttribute(outWeights, 4));
  geometry.setIndex(outTriangles);
  geometry.computeBoundingSphere();
  return geometry;
}

function disposeGarment(group: THREE.Group): void {
  group.traverse(object => {
    if (!(object instanceof THREE.Mesh)) return;
    object.geometry.dispose();
    for (const material of Array.isArray(object.material) ? object.material : [object.material])
      material.dispose();
  });
  group.removeFromParent();
}

export function removeResidentAuthoredGarment(model: THREE.Group): void {
  const group = model.getObjectByName(AUTHORED_GARMENT_GROUP);
  if (group instanceof THREE.Group) disposeGarment(group);
}

export function syncResidentAuthoredGarment(
  model: THREE.Group,
  garment: ResidentCivilizationItem | null | undefined,
  appearance: ResidentAppearanceProfile,
): boolean {
  const old = model.getObjectByName(AUTHORED_GARMENT_GROUP);
  if (!validWornPrimitiveWrap(garment)) {
    if (old instanceof THREE.Group) disposeGarment(old);
    return false;
  }

  const body = model.getObjectByName('LifeLensAuthoredAnatomicalBody');
  if (!(body instanceof THREE.SkinnedMesh) || !body.parent) {
    if (old instanceof THREE.Group) disposeGarment(old);
    return false;
  }
  // A native-body swap replaces the anatomy geometry even if Core's worn item
  // is unchanged. Include that source identity to avoid stale cloth fitting.
  const signature = [
    garment.item, garment.material, garment.quality, garment.durability,
    appearance.seed, body.geometry.uuid,
  ].join(':');
  if (old instanceof THREE.Group && old.userData.garmentSignature === signature)
    return true;
  if (old instanceof THREE.Group) disposeGarment(old);

  const geometry = createBodyFittedWrap(
    body, garment.quality!, garment.durability!, appearance.seed,
  );
  if (!geometry) return false; // Incompatible skeleton: no fake garment.

  const color = new THREE.Color(0xb6a27c);
  color.offsetHSL(0, 0, ((appearance.seed % 101) / 100 - .5) * .07);
  const material = new THREE.MeshStandardMaterial({
    color, roughness: 1, metalness: 0, side: THREE.DoubleSide,
  });
  const mesh = new THREE.SkinnedMesh(geometry, material);
  mesh.name = 'LifeLensPrimitiveFiberWrap';
  mesh.userData.residentProceduralAppearance = true;
  mesh.userData.residentOwnsGeometry = true;
  mesh.position.copy(body.position);
  mesh.quaternion.copy(body.quaternion);
  mesh.scale.copy(body.scale);
  mesh.bindMode = body.bindMode;
  mesh.bind(body.skeleton, body.bindMatrix.clone());
  mesh.bindMatrixInverse.copy(body.bindMatrixInverse);
  mesh.frustumCulled = false;
  mesh.receiveShadow = true;

  const group = new THREE.Group();
  group.name = AUTHORED_GARMENT_GROUP;
  group.userData.garmentSignature = signature;
  group.add(mesh);
  // The native near LOD is transient. Mount next to the persistent donor
  // skeleton so clothing does not disappear when camera LOD changes.
  const donorParent = model.getObjectByName('SuperHero_Male')?.parent;
  (donorParent ?? body.parent).add(group);
  return true;
}
