import * as THREE from 'three';
import { RESIDENT_JOINTS, type ResidentJoint } from './resident-rig';
import type { ResidentAppearanceProfile } from './resident-appearance';

// The visual body is new, LifeLens-authored parametric anatomy. The imported
// CC0 GLB contributes ONLY its existing joint positions, inverse bind matrices
// and compatible animation tracks. Every generated vertex is skinned to that
// skeleton; there are no floating post-animation overlays.
export const ANATOMICAL_BODY_NAME = 'LifeLensAuthoredAnatomicalBody';
type Joint = { index: number; point: THREE.Vector3 };
type RingWeight = { index: number; weight: number };
type Section = {
  center: THREE.Vector3;
  radialX: THREE.Vector3;
  radialZ: THREE.Vector3;
  radiusX: number;
  radiusZ: number;
  weights: RingWeight[];
  frontBump?: number;
  ridge?: number;
};
type AnatomyResult = { geometry: THREE.BufferGeometry; source: THREE.SkinnedMesh };

const clamp = THREE.MathUtils.clamp;
function gaussian(value: number, center: number, width: number): number {
  return Math.exp(-Math.pow((value - center) / width, 2));
}
function interpolate(a: THREE.Vector3, b: THREE.Vector3, t: number): THREE.Vector3 {
  return a.clone().lerp(b, t);
}

class AnatomicalSurface {
  private readonly vertices: number[] = [];
  private readonly bones: number[] = [];
  private readonly weights: number[] = [];
  private readonly faces: number[] = [];

  addLoft(
    rows: number,
    sides: number,
    section: (t: number) => Section,
    modulation?: (t: number, angle: number, section: Section) => THREE.Vector3,
  ): void {
    const offset = this.vertices.length / 3;
    for (let r = 0; r <= rows; r++) {
      const t = r / rows;
      const fit = section(t);
      for (let side = 0; side < sides; side++) {
        const angle = Math.PI * 2 * side / sides;
        const sin = Math.sin(angle), cos = Math.cos(angle);
        const vertex = fit.center.clone()
          .addScaledVector(fit.radialX, fit.radiusX * sin)
          .addScaledVector(fit.radialZ, fit.radiusZ * cos);
        if (fit.frontBump) vertex.addScaledVector(fit.radialZ,
          fit.frontBump * Math.pow(Math.max(0, cos), 4));
        if (fit.ridge) vertex.addScaledVector(fit.radialZ, fit.ridge * cos);
        if (modulation) vertex.add(modulation(t, angle, fit));
        this.vertices.push(vertex.x, vertex.y, vertex.z);
        const sorted = [...fit.weights].filter(j => j.weight > 0)
          .sort((a, b) => b.weight - a.weight).slice(0, 4);
        const sum = sorted.reduce((n, j) => n + j.weight, 0) || 1;
        for (let j = 0; j < 4; j++) {
          this.bones.push(sorted[j]?.index ?? sorted[0]?.index ?? 0);
          this.weights.push((sorted[j]?.weight ?? 0) / sum);
        }
      }
    }
    for (let r = 0; r < rows; r++) {
      for (let side = 0; side < sides; side++) {
        const next = (side + 1) % sides;
        const a = offset + r * sides + side;
        const b = offset + r * sides + next;
        const c = offset + (r + 1) * sides + side;
        const d = offset + (r + 1) * sides + next;
        this.faces.push(a, c, b, b, c, d);
      }
    }
  }
  geometry(): THREE.BufferGeometry {
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.Float32BufferAttribute(this.vertices, 3));
    geometry.setAttribute('skinIndex', new THREE.Uint16BufferAttribute(this.bones, 4));
    geometry.setAttribute('skinWeight', new THREE.Float32BufferAttribute(this.weights, 4));
    geometry.setIndex(this.faces);
    geometry.computeVertexNormals();
    geometry.computeBoundingSphere();
    return geometry;
  }
}

function jointMap(source: THREE.SkinnedMesh): Map<ResidentJoint, Joint> {
  const joints = new Map<ResidentJoint, Joint>();
  const inverse = source.matrixWorld.clone().invert();
  for (const name of Object.keys(RESIDENT_JOINTS) as ResidentJoint[]) {
    const index = source.skeleton.bones.findIndex(bone =>
      (RESIDENT_JOINTS[name] as readonly string[]).includes(bone.name));
    if (index < 0) continue;
    const point = source.skeleton.bones[index].getWorldPosition(new THREE.Vector3())
      .applyMatrix4(inverse);
    if (Number.isFinite(point.x + point.y + point.z))
      joints.set(name, { index, point });
  }
  return joints;
}

function blended(joints: Joint[], t: number): RingWeight[] {
  const f = clamp(t, 0, 1) * (joints.length - 1);
  const index = Math.min(joints.length - 2, Math.floor(f));
  const mix = f - index;
  return [{ index: joints[index].index, weight: 1 - mix },
    { index: joints[index + 1].index, weight: mix }];
}

function fitHead(source: THREE.SkinnedMesh, head: Joint): THREE.Box3 | null {
  const positions = source.geometry.getAttribute('position');
  const skinIndex = source.geometry.getAttribute('skinIndex');
  const skinWeight = source.geometry.getAttribute('skinWeight');
  if (!positions || !skinIndex || !skinWeight) return null;
  const bounds = new THREE.Box3();
  let count = 0;
  for (let i = 0; i < positions.count; i++) {
    let weight = 0;
    for (let j = 0; j < 4; j++) if (skinIndex.getComponent(i, j) === head.index)
      weight += skinWeight.getComponent(i, j);
    if (weight < .65) continue;
    bounds.expandByPoint(new THREE.Vector3().fromBufferAttribute(positions, i));
    count++;
  }
  return count >= 12 && !bounds.isEmpty() ? bounds : null;
}

function addAnatomy(source: THREE.SkinnedMesh, profile: ResidentAppearanceProfile): AnatomyResult | null {
  source.updateWorldMatrix(true, true);
  const bones = jointMap(source);
  const required: ResidentJoint[] = [
    'pelvis', 'spine', 'neck', 'head',
    'leftShoulder', 'rightShoulder',
    'leftArm', 'leftForearm', 'leftHand',
    'rightArm', 'rightForearm', 'rightHand',
    'leftThigh', 'leftCalf', 'leftFoot',
    'rightThigh', 'rightCalf', 'rightFoot',
  ];
  if (required.some(name => !bones.has(name))) return null;
  const get = (name: ResidentJoint): Joint => bones.get(name)!;
  // The shared UAL rig has a second spine level; use the spine_03 joint if
  // present, otherwise remain on the known semantic spine. Never invent joints.
  const upperSpineIndex = source.skeleton.bones.findIndex(b => b.name === 'spine_03');
  const upperSpine = upperSpineIndex >= 0
    ? { index: upperSpineIndex, point: source.skeleton.bones[upperSpineIndex]
      .getWorldPosition(new THREE.Vector3()).applyMatrix4(source.matrixWorld.clone().invert()) }
    : get('spine');
  const points = [get('pelvis'), get('spine'), upperSpine, get('neck')];
  const headBounds = fitHead(source, get('head'));
  if (!headBounds) return null;
  const height = Math.abs(headBounds.max.y - get('pelvis').point.y);
  const shoulderWidth = Math.abs(get('leftArm').point.x - get('rightArm').point.x);
  const hipWidth = Math.abs(get('leftThigh').point.x - get('rightThigh').point.x);
  if (!(height > .3 && height < 15 && shoulderWidth > .07 && hipWidth > .04)) return null;
  const scale = height;
  const eyes = source.parent?.getObjectByName('Eyes');
  const eyeBounds = eyes ? new THREE.Box3().setFromObject(eyes) : null;
  const eyePosition = eyeBounds && !eyeBounds.isEmpty()
    ? eyeBounds.getCenter(new THREE.Vector3()).applyMatrix4(source.matrixWorld.clone().invert())
    : null;
  const front = eyePosition && Math.abs(eyePosition.z - headBounds.getCenter(new THREE.Vector3()).z) > .005
    ? Math.sign(eyePosition.z - headBounds.getCenter(new THREE.Vector3()).z) : 1;
  const frontVector = new THREE.Vector3(0, 0, front);
  const sideVector = new THREE.Vector3(1, 0, 0);
  const body = new AnatomicalSurface();
  const female = profile.bodyMaturity01 * (profile.breastVolume > 0 ? 1 : 0);
  const male = profile.bodyMaturity01 * (profile.breastVolume === 0 ? 1 : 0);
  const halfShoulder = shoulderWidth * .5;
  const halfHip = hipWidth * .70;
  const waist = (halfShoulder * .88 + halfHip * .9) * .5 * profile.waistScale;
  const line = (t: number) => {
    const f = clamp(t, 0, 1) * (points.length - 1);
    const i = Math.min(points.length - 2, Math.floor(f));
    return interpolate(points[i].point, points[i + 1].point, f - i);
  };
  const radii = [
    { t: 0, x: halfHip * 1.05, z: halfHip * .66 },
    { t: .18, x: waist, z: halfHip * .68 },
    { t: .42, x: halfShoulder * .84, z: halfShoulder * .58 },
    { t: .68, x: halfShoulder * (.99 + male * .08), z: halfShoulder * .62 },
    { t: .83, x: halfShoulder * (1.06 + male * .04), z: halfShoulder * .54 },
    { t: 1, x: halfShoulder * .32, z: halfShoulder * .30 },
  ];
  function radius(t: number) {
    const i = Math.min(radii.length - 2, Math.max(0, radii.findIndex(r => r.t >= t) - 1));
    const first = radii[i], last = radii[i + 1];
    const f = THREE.MathUtils.smoothstep(t, first.t, last.t);
    return { x: THREE.MathUtils.lerp(first.x, last.x, f),
      z: THREE.MathUtils.lerp(first.z, last.z, f) };
  }
  // 41 rib/abdominal/shoulder cross sections, with independent shoulder,
  // waist, clavicle, serratus and paired pectoral contours.
  body.addLoft(40, 40, t => {
    const r = radius(t);
    return { center: line(t), radialX: sideVector, radialZ: frontVector,
      radiusX: r.x, radiusZ: r.z, weights: blended(points, t),
      frontBump: (.014 + male * .012) * scale * gaussian(t, .59, .18) };
  }, (t, angle) => {
    const x = Math.sin(angle), f = Math.max(0, Math.cos(angle));
    const chest = female * profile.breastVolume * .11 * scale * gaussian(t, .64, .105)
      * gaussian(Math.abs(x), .40, .27) * f * f;
    const clavicle = .011 * scale * gaussian(t, .87, .055)
      * gaussian(Math.abs(x), .58, .31) * f;
    const oblique = .004 * scale * gaussian(t, .34, .12)
      * Math.cos(8 * angle) * f;
    return frontVector.clone().multiplyScalar(chest + clavicle + oblique);
  });

  // A fully authored high-resolution cranium and jaw, with subtle nose bridge,
  // cheek planes and brows. Eyes and hair keep their existing bone attachments.
  const headCenter = headBounds.getCenter(new THREE.Vector3());
  const headSize = headBounds.getSize(new THREE.Vector3());
  body.addLoft(26, 36, t => {
    const ellipse = Math.max(.035, Math.sin(Math.PI * t) ** .48);
    const jaw = 1 - (.19 + female * .04) * (1 - THREE.MathUtils.smoothstep(t, .17, .58));
    return { center: new THREE.Vector3(headCenter.x,
      THREE.MathUtils.lerp(headBounds.min.y, headBounds.max.y, t),
      headCenter.z),
    radialX: sideVector, radialZ: frontVector,
    radiusX: headSize.x * .5 * ellipse * jaw * profile.headWidthScale,
    radiusZ: headSize.z * .53 * ellipse * (1 + .06 * gaussian(t, .62, .19)),
    weights: [{ index: get('head').index, weight: 1 }] };
  }, (t, angle) => {
    const facing = Math.max(0, Math.cos(angle));
    const cheek = .022 * scale * gaussian(t, .46, .10) * gaussian(Math.abs(Math.sin(angle)), .57, .21);
    const nose = .040 * scale * gaussian(t, .45, .075) * gaussian(Math.sin(angle), 0, .18);
    const brow = .014 * scale * (1 + male * .32) * gaussian(t, .62, .046)
      * gaussian(Math.abs(Math.sin(angle)), .45, .26);
    return frontVector.clone().multiplyScalar(facing * (cheek + nose + brow));
  });

  // Neck has its own smoothly weighted bridge across spine, neck and head.
  const neckBottom = get('neck').point.clone().lerp(upperSpine.point, .35);
  const neckTop = get('head').point.clone().lerp(get('neck').point, .36);
  body.addLoft(10, 24, t => ({
    center: interpolate(neckBottom, neckTop, t),
    radialX: sideVector, radialZ: frontVector,
    radiusX: .072 * scale * (1 - .09 * gaussian(t, .5, .4)),
    radiusZ: .060 * scale,
    weights: [{ index: get('neck').index, weight: 1 - t },
      { index: get('head').index, weight: t }],
  }));

  function limb(start: Joint, middle: Joint, end: Joint, type: 'arm' | 'leg') {
    const upper = start.point.distanceTo(middle.point);
    const lower = middle.point.distanceTo(end.point);
    if (!(upper > scale * .06 && lower > scale * .06)) return;
    const baseRadius = (type === 'arm' ? .060 : .093) * scale;
    // 32 rings per limb, with taper/growth at deltoids, biceps, elbows,
    // forearms, kneecaps and gastrocnemius calves.
    body.addLoft(32, 24, t => {
      const p = t < .5 ? interpolate(start.point, middle.point, t * 2)
        : interpolate(middle.point, end.point, (t - .5) * 2);
      const tangent = t < .5 ? middle.point.clone().sub(start.point)
        : end.point.clone().sub(middle.point);
      tangent.normalize();
      const radialZ = new THREE.Vector3().crossVectors(tangent, sideVector);
      if (radialZ.lengthSq() < .01) radialZ.crossVectors(tangent, frontVector);
      radialZ.normalize();
      const radialX = new THREE.Vector3().crossVectors(radialZ, tangent).normalize();
      const contour = type === 'arm'
        ? .88 + .27 * gaussian(t, .22, .18) - .18 * gaussian(t, .51, .09)
          + .08 * gaussian(t, .72, .14) - .28 * THREE.MathUtils.smoothstep(t, .83, 1)
        : 1.05 + .20 * gaussian(t, .22, .2) - .10 * gaussian(t, .52, .09)
          + .15 * gaussian(t, .67, .14) - .39 * THREE.MathUtils.smoothstep(t, .78, 1);
      const strength = type === 'arm' ? 1 + male * .10 : 1 + female * .08;
      return { center: p, radialX, radialZ,
        radiusX: baseRadius * contour * strength,
        radiusZ: baseRadius * contour * (type === 'leg' ? .90 : .85),
        weights: t < .5
          ? [{ index: start.index, weight: 1 - THREE.MathUtils.smoothstep(t, .25, .57) },
            { index: middle.index, weight: THREE.MathUtils.smoothstep(t, .25, .57) }]
          : [{ index: middle.index, weight: 1 - THREE.MathUtils.smoothstep(t, .68, .96) },
            { index: end.index, weight: THREE.MathUtils.smoothstep(t, .68, .96) }] };
    });
  }
  for (const side of ['left', 'right'] as const) {
    limb(get(`${side}Arm`), get(`${side}Forearm`), get(`${side}Hand`), 'arm');
    limb(get(`${side}Thigh`), get(`${side}Calf`), get(`${side}Foot`), 'leg');
    const hand = get(`${side}Hand`), forearm = get(`${side}Forearm`);
    const fingerDirection = hand.point.clone().sub(forearm.point).normalize();
    const across = Math.sign(hand.point.x) || 1;
    for (let finger = 0; finger < 5; finger++) {
      const offset = (finger - 2) * .015 * scale;
      const start = hand.point.clone().add(new THREE.Vector3(0, 0, offset));
      const end = start.clone().addScaledVector(fingerDirection,
        scale * (.065 + (finger === 0 ? -.019 : -.009 * Math.abs(finger - 2))));
      body.addLoft(6, 10, t => ({
        center: interpolate(start, end, t),
        radialX: sideVector,
        radialZ: frontVector,
        radiusX: .011 * scale * (1 - .78 * t),
        radiusZ: .011 * scale * (1 - .78 * t),
        weights: [{ index: hand.index, weight: 1 }],
      }));
    }
    // Anatomically distinct heel and forefoot, both follow the actual foot.
    const foot = get(`${side}Foot`);
    const ball = foot.point.clone().addScaledVector(frontVector, .080 * scale);
    body.addLoft(12, 22, t => ({
      center: interpolate(foot.point, ball, t),
      radialX: sideVector, radialZ: new THREE.Vector3(0, 1, 0),
      radiusX: scale * (.047 + .006 * Math.sin(Math.PI * t)),
      radiusZ: scale * (.033 - .013 * t),
      weights: [{ index: foot.index, weight: 1 }],
    }));
    // Ears sit on the skull, not independently floating world-space sprites.
    for (const earSide of [across]) {
      const ear = headCenter.clone().add(new THREE.Vector3(
        earSide * headSize.x * .52, headSize.y * .025, 0));
      body.addLoft(8, 12, t => ({
        center: ear.clone().add(new THREE.Vector3(0, (t - .5) * headSize.y * .22, 0)),
        radialX: sideVector, radialZ: frontVector,
        radiusX: headSize.x * .047 * Math.sin(Math.PI * t) + .001,
        radiusZ: headSize.z * .12 * Math.sin(Math.PI * t) + .001,
        weights: [{ index: get('head').index, weight: 1 }],
      }));
    }
  }
  const geometry = body.geometry();
  if (!Number.isFinite(geometry.boundingSphere?.radius)
      || geometry.getAttribute('position').count < 3500) {
    geometry.dispose();
    return null;
  }
  return { geometry, source };
}

export function removeResidentAnatomicalBody(model: THREE.Group): void {
  const old = model.getObjectByName(ANATOMICAL_BODY_NAME);
  if (old instanceof THREE.SkinnedMesh) {
    old.removeFromParent();
    old.geometry.dispose();
    for (const material of Array.isArray(old.material) ? old.material : [old.material])
      material.dispose();
  }
  const donor = model.getObjectByName('SuperHero_Male');
  if (donor instanceof THREE.SkinnedMesh) {
    for (const material of Array.isArray(donor.material) ? donor.material : [donor.material])
      material.visible = true;
  }
}

export function syncResidentAnatomicalBody(
  model: THREE.Group,
  profile: ResidentAppearanceProfile,
): boolean {
  const donor = model.getObjectByName('SuperHero_Male');
  if (!(donor instanceof THREE.SkinnedMesh)) return false;
  // Keep the original alive for A1/A2 face & hair attachment bounds, while
  // rendering only the authored body. No skeleton/clip remapping in Core.
  const old = model.getObjectByName(ANATOMICAL_BODY_NAME);
  if (old instanceof THREE.SkinnedMesh) removeResidentAnatomicalBody(model);
  const authored = addAnatomy(donor, profile);
  if (!authored || !donor.parent) return false;
  const material = new THREE.MeshStandardMaterial({
    color: profile.skinColor, roughness: .92, metalness: 0,
    side: THREE.DoubleSide,
  });
  const body = new THREE.SkinnedMesh(authored.geometry, material);
  body.name = ANATOMICAL_BODY_NAME;
  body.userData.residentProceduralAppearance = true;
  body.userData.residentOwnsGeometry = true;
  body.position.copy(donor.position);
  body.quaternion.copy(donor.quaternion);
  body.scale.copy(donor.scale);
  body.bindMode = donor.bindMode;
  body.bind(donor.skeleton, donor.bindMatrix.clone());
  body.bindMatrixInverse.copy(donor.bindMatrixInverse);
  body.frustumCulled = false;
  body.castShadow = true;
  body.receiveShadow = true;
  donor.parent.add(body);
  for (const sourceMaterial of Array.isArray(donor.material) ? donor.material : [donor.material])
    sourceMaterial.visible = false;
  donor.castShadow = false;
  return true;
}
