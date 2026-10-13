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
  crown.scale.set(rx, ry * (.85 + .15 * appearance.hairLengthScale), rz);
  crown.position.copy(center).add(new THREE.Vector3(
    0, radii.y * 0.06, 0,
  ));
  cap.add(crown);

  // Identity-stable silhouettes: cropped / parted / tousled / layered /
  // bob / short curl / swept / longer. The side/back locks are custom curves,
  // not a disguised downloaded wig or a painted-on hairline.
  const lockCount = [0, 1, 3, 4, 6, 5, 4, 8][style];
  const lockLength = [0, 0.18, 0.27, 0.39, 0.78, 0.26, 0.40, 1.04][style];
  const h = Math.max(radii.y * 2, 0.01) * appearance.hairLengthScale;
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


const FACE_GROUP_NAME = 'LifeLensResidentAuthoredFace';

/** A2 only places face parts when the actual imported Eye mesh gives a reliable
 * facial-front landmark. Never guess facing direction or add floating features
 * to nonhuman and unrecognized GLBs.
 */
function verifiedFaceDirection(model: THREE.Group, head: HeadFit): 1 | -1 | null {
  model.updateMatrixWorld(true);
  const local = head.bone.matrixWorld.clone().invert();
  const eyeMarkers: THREE.Vector3[] = [];
  model.traverse(object => {
    if (!(object instanceof THREE.Mesh)
        || object.userData.residentProceduralAppearance
        || !/^(Eyes|Eye[._-]?[lr])$/i.test(object.name)) return;
    if (!object.geometry.getAttribute('position')) return;
    // Eye assets in the real pinned GLB are SkinnedMesh, not rigid markers.
    // THREE.Box3.setFromObject(..., true) evaluates their actual skinned
    // vertex bounds in world coordinates, then transforms into Head space.
    // Reading raw geometry.boundingBox silently ignores the skin bind pose.
    const worldBounds = new THREE.Box3().setFromObject(object, true);
    if (worldBounds.isEmpty()) return;
    const point = worldBounds.getCenter(new THREE.Vector3()).applyMatrix4(local);
    if (Number.isFinite(point.x + point.y + point.z)) eyeMarkers.push(point);
  });
  if (!eyeMarkers.length) return null;
  const mean = eyeMarkers.reduce((sum, eye) => sum.add(eye), new THREE.Vector3())
    .multiplyScalar(1 / eyeMarkers.length);
  const depth = mean.z - head.center.z;
  // Eye centroid should lie on the head's front, not its middle or neck.
  if (Math.abs(depth) < head.radii.z * .22
      || Math.abs(depth) > head.radii.z * 2.25
      || mean.y < head.center.y - head.radii.y * .65
      || mean.y > head.center.y + head.radii.y * 1.3) return null;
  return depth > 0 ? 1 : -1;
}

/** Geometry is born from existing Core faceShape and identity; no new traits
 * are written to the Core or re-derived from an action that never occurred.
 * The mouth and nose are subtle close-range identity features only.
 */
export function syncResidentAuthoredFace(
  model: THREE.Group,
  appearance: ResidentAppearanceProfile,
): boolean {
  const previous = model.getObjectByName(FACE_GROUP_NAME);
  if (previous instanceof THREE.Group) disposeHair(previous);
  const cached = model.userData.residentHeadFit as HeadFit | undefined;
  const head = cached ?? fitHeadFromSkin(model);
  if (!head) return false;
  if (!cached) model.userData.residentHeadFit = head;
  const front = verifiedFaceDirection(model, head);
  if (!front) return false;
  const { bone, center, radii } = head;
  const group = new THREE.Group();
  group.name = FACE_GROUP_NAME;
  group.userData.residentProceduralAppearance = true;
  // Unitless local head dimensions; works for both growing child and adult
  // because the parent ResidentWorldLayer already owns world-space scaling.
  const skin = new THREE.Color(appearance.skinColor);
  const noseColor = skin.clone().multiplyScalar(.97).getHex();
  const lipColor = skin.clone().lerp(new THREE.Color(0x7d4444), .21).getHex();
  const x = Math.max(.005, radii.x), y = Math.max(.005, radii.y),
    z = Math.max(.005, radii.z);
  const nose = authoredMesh(
    new THREE.SphereGeometry(1, 9, 6), noseColor,
    'LifeLensResidentNoseBridge',
  );
  nose.scale.set(x * .135 * appearance.noseScale,
    y * .20 * appearance.noseScale,
    z * .10 * appearance.noseScale);
  nose.position.set(center.x, center.y + y * .01,
    center.z + front * z * 1.025);
  group.add(nose);

  const lipHalfWidth = x * .29 * appearance.mouthWidthScale;
  const mouthY = center.y - y * .39;
  const mouthZ = center.z + front * z * .97;
  const lip = authoredMesh(new THREE.TubeGeometry(
    new THREE.CatmullRomCurve3([
      new THREE.Vector3(center.x - lipHalfWidth, mouthY + y * .015, mouthZ),
      new THREE.Vector3(center.x, mouthY, mouthZ + front * z * .045),
      new THREE.Vector3(center.x + lipHalfWidth, mouthY + y * .015, mouthZ),
    ]), 7, Math.max(.001, y * .025), 4, false,
  ), lipColor, 'LifeLensResidentMouthContour');
  group.add(lip);

  if (appearance.ageWeathering01 > .65) {
    // Small age creases are purely a view of authoritative physical age,
    // not fabricated scars or events.
    const crease = skin.clone().multiplyScalar(.84).getHex();
    for (const side of [-1, 1]) {
      const p = center.x + side * x * .54;
      group.add(authoredMesh(new THREE.TubeGeometry(
        new THREE.CatmullRomCurve3([
          new THREE.Vector3(p, mouthY + y * .07, mouthZ),
          new THREE.Vector3(p + side * x * .035, mouthY, mouthZ),
          new THREE.Vector3(p + side * x * .045, mouthY - y * .075, mouthZ - front * z * .015),
        ]), 4, Math.max(.0007, y * .012 * appearance.ageWeathering01), 3, false,
      ), crease, 'LifeLensResidentAgeCrease'));
    }
  }
  bone.add(group);
  return true;
}

/** Far-away residents keep hairstyle silhouette but skip face detail draw calls.
 * Selection always wins; this does not alter Core/animation progression.
 */
export function setResidentFaceDetailVisibility(
  model: THREE.Group,
  visible: boolean,
): void {
  const details = model.getObjectByName(FACE_GROUP_NAME);
  if (details) details.visible = visible;
}

export function removeResidentAuthoredFace(model: THREE.Group): void {
  const previous = model.getObjectByName(FACE_GROUP_NAME);
  if (previous instanceof THREE.Group) disposeHair(previous);
}
