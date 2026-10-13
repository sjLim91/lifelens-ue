import * as THREE from 'three';
import type { ResidentAppearanceProfile } from './resident-appearance';

// LifeLens-owned geometry: no model downloads, paid assets, GLB extensions,
// random() calls or edits to Core genetics. Every attachment belongs to a
// particular cloned resident skeleton, and follows the real animated Head.
const HAIR_GROUP_NAME = 'LifeLensResidentAuthoredHair';
type HeadFit = {
  bone: THREE.Bone;
  center: THREE.Vector3;
  radii: THREE.Vector3;
};

function fitHeadFromSkin(model: THREE.Group): HeadFit | null {
  model.updateMatrixWorld(true);
  let found: HeadFit | null = null;
  model.traverse(object => {
    if (found || !(object instanceof THREE.SkinnedMesh)) return;
    const joints = object.geometry.getAttribute('skinIndex');
    const weights = object.geometry.getAttribute('skinWeight');
    const vertices = object.geometry.getAttribute('position');
    if (!joints || !weights || !vertices) return;
    const headIndex = object.skeleton.bones.findIndex(b => /^head$/i.test(b.name));
    if (headIndex < 0) return;
    const bone = object.skeleton.bones[headIndex];
    const local = bone.matrixWorld.clone().invert().multiply(object.matrixWorld);
    const bounds = new THREE.Box3();
    const position = new THREE.Vector3();
    let count = 0;
    for (let i = 0; i < vertices.count; i++) {
      let influence = 0;
      for (let j = 0; j < 4; j++) {
        if (joints.getComponent(i, j) === headIndex)
          influence += weights.getComponent(i, j);
      }
      if (influence < 0.65) continue;
      position.fromBufferAttribute(vertices, i).applyMatrix4(local);
      if (!Number.isFinite(position.x + position.y + position.z)) continue;
      bounds.expandByPoint(position);
      count++;
    }
    if (count < 8 || bounds.isEmpty()) return;
    const dimensions = bounds.getSize(new THREE.Vector3());
    // Reject missing/degenerate, mirrored or incompatible rigs. Never attach
    // floating hair to a guessed head location.
    if (dimensions.x < 0.01 || dimensions.y < 0.01 || dimensions.z < 0.01
        || dimensions.x > 1.5 || dimensions.y > 1.5 || dimensions.z > 1.5)
      return;
    found = {
      bone,
      center: bounds.getCenter(new THREE.Vector3()),
      radii: dimensions.multiplyScalar(0.5),
    };
  });
  return found;
}

function disposeHair(group: THREE.Group): void {
  group.traverse(object => {
    if (!(object instanceof THREE.Mesh)) return;
    object.geometry.dispose();
    const materials = Array.isArray(object.material)
      ? object.material : [object.material];
    for (const material of materials) material.dispose();
  });
  group.removeFromParent();
}

function authoredMesh(
  geometry: THREE.BufferGeometry,
  color: number,
  label: string,
): THREE.Mesh {
  const mesh = new THREE.Mesh(
    geometry,
    new THREE.MeshStandardMaterial({
      color, roughness: 0.94, metalness: 0.0,
      side: THREE.DoubleSide,
    }),
  );
  mesh.name = label;
  mesh.userData.residentProceduralAppearance = true;
  mesh.userData.residentOwnsGeometry = true;
  mesh.castShadow = false;
  mesh.receiveShadow = false;
  return mesh;
}

function strand(
  parent: THREE.Group,
  materialColor: number,
  start: THREE.Vector3,
  middle: THREE.Vector3,
  end: THREE.Vector3,
  thickness: number,
): void {
  const curve = new THREE.CatmullRomCurve3([start, middle, end]);
  const mesh = authoredMesh(
    new THREE.TubeGeometry(curve, 7, thickness, 5, false),
    materialColor, 'LifeLensResidentHairLock',
  );
  parent.add(mesh);
}

/** Adds one self-authored head-bone attachment, with identity-stable hairstyles.
 * A return of false is a deliberate no-op for unknown/unsupported skeletons.
 */
export function syncResidentAuthoredHair(
  model: THREE.Group,
  appearance: ResidentAppearanceProfile,
): boolean {
  const previous = model.getObjectByName(HAIR_GROUP_NAME);
  if (previous instanceof THREE.Group) disposeHair(previous);
  const cached = model.userData.residentHeadFit as HeadFit | undefined;
  const head = cached ?? fitHeadFromSkin(model);
  if (!head) return false;
  if (!cached) model.userData.residentHeadFit = head;

  const { bone, center, radii } = head;
  const cap = new THREE.Group();
  cap.name = HAIR_GROUP_NAME;
  cap.userData.residentProceduralAppearance = true;
  const style = appearance.hairStyle % 8;
  // Compact sculpted dome. Long styles add separate side/back locks below.
  const thickness = 1.05 + (appearance.seed % 7) * 0.004;
  const rx = radii.x * thickness * appearance.headWidthScale;
  const ry = radii.y * 1.08;
  const rz = radii.z * thickness;
  const crown = authoredMesh(
    new THREE.SphereGeometry(
      1, 14, 6, 0, Math.PI * 2, 0,
      [1.06, 1.18, 1.30, 1.40, 1.52, 1.26, 1.37, 1.52][style],
    ),
    appearance.hairColor, 'LifeLensResidentHairCrown',
  );
  crown.scale.set(rx, ry, rz);
  crown.position.copy(center).add(new THREE.Vector3(
    0, radii.y * 0.06, 0,
  ));
  cap.add(crown);

  // Identity-stable silhouettes: cropped / parted / tousled / layered /
  // bob / short curl / swept / longer. The side/back locks are custom curves,
  // not a disguised downloaded wig or a painted-on hairline.
  const lockCount = [0, 1, 3, 4, 6, 5, 4, 8][style];
  const lockLength = [0, 0.18, 0.27, 0.39, 0.78, 0.26, 0.40, 1.04][style];
  const h = Math.max(radii.y * 2, 0.01);
  for (let i = 0; i < lockCount; i++) {
    const side = i % 2 === 0 ? -1 : 1;
    const depthSlot = Math.floor(i / 2);
    const t = lockCount < 3 ? 0.0 : depthSlot / Math.max(1, Math.ceil(lockCount / 2) - 1);
    const start = new THREE.Vector3(
      center.x + side * rx * (0.73 + 0.12 * t),
      center.y + ry * (0.35 - t * 0.15),
      center.z + rz * (0.42 - t * 1.1),
    );
    const drop = h * lockLength * (0.75 + 0.1 * t);
    const curl = (0.04 + 0.1 * t) * radii.x * (i % 3 === 0 ? -1 : 1);
    const middle = start.clone().add(new THREE.Vector3(
      side * radii.x * 0.18, -drop * 0.55, curl,
    ));
    const end = start.clone().add(new THREE.Vector3(
      side * radii.x * (style === 7 ? -0.05 : 0.1),
      -drop, curl * 1.8,
    ));
    strand(cap, appearance.hairColor, start, middle, end,
      Math.max(0.0025, radii.x * (style === 5 ? 0.13 : 0.17)));
  }
  bone.add(cap);
  return true;
}

export function removeResidentAuthoredHair(model: THREE.Group): void {
  const previous = model.getObjectByName(HAIR_GROUP_NAME);
  if (previous instanceof THREE.Group) disposeHair(previous);
}
