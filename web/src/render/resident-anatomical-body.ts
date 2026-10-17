import * as THREE from 'three';
import { RESIDENT_JOINTS, type ResidentJoint } from './resident-rig';
import type { ResidentAppearanceProfile } from './resident-appearance';
import { createResidentSkinMaterial } from './resident-surface-material';
import { syncNativeResidentBody } from './resident-native-body';

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

/** The original donor joint's *bind* position in donor-local space.
 * Current animated bone.matrixWorld must never drive permanent anatomy:
 * a phenotype refresh can arrive during a walk, crouch or sleep.
 */
function bindPoint(source: THREE.SkinnedMesh, index: number): THREE.Vector3 {
  const inverse = source.skeleton.boneInverses[index];
  if (!inverse || index < 0 || index >= source.skeleton.bones.length)
    throw new Error('Unsupported anatomical bind joint');
  const local = source.bindMatrix.clone().invert().multiply(inverse.clone().invert());
  return new THREE.Vector3().setFromMatrixPosition(local);
}
function jointMap(source: THREE.SkinnedMesh): Map<ResidentJoint, Joint> {
  const joints = new Map<ResidentJoint, Joint>();
  for (const name of Object.keys(RESIDENT_JOINTS) as ResidentJoint[]) {
    const index = source.skeleton.bones.findIndex(bone =>
      (RESIDENT_JOINTS[name] as readonly string[]).includes(bone.name));
    if (index < 0) continue;
    const point = bindPoint(source, index);
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

/**
 * Retarget the new high-density anatomy to the proven original skin weights.
 * Bone-name-only binding is not sufficient: the UAL sleep clip rotates the
 * spine differently from a straight loft. Spatial donor transfer preserves
 * the original joint bend planes while keeping the NEW body geometry.
 */
function retargetSkinning(source: THREE.SkinnedMesh, geometry: THREE.BufferGeometry): boolean {
  const sourcePosition = source.geometry.getAttribute('position');
  const sourceIndex = source.geometry.getAttribute('skinIndex');
  const sourceWeight = source.geometry.getAttribute('skinWeight');
  const vertices = geometry.getAttribute('position');
  const outputIndex = geometry.getAttribute('skinIndex');
  const outputWeight = geometry.getAttribute('skinWeight');
  if (!sourcePosition || !sourceIndex || !sourceWeight
      || !vertices || !outputIndex || !outputWeight) return false;
  const height = Math.max(.01,
    source.geometry.boundingBox?.getSize(new THREE.Vector3()).y ?? 1);
  const spacing = height / 22;
  const label = (joint: number): string => {
    const name = source.skeleton.bones[joint]?.name.toLowerCase() ?? '';
    if (/^head$|^neck_/.test(name)) return 'head';
    if (/spine|pelvis|^root$/.test(name)) return 'torso';
    const side = /_l$/.test(name) ? 'left' : /_r$/.test(name) ? 'right' : '';
    const digit = /^(thumb|index|middle|ring|pinky)_/.exec(name)?.[1];
    if (digit) return side + 'Digit:' + digit;
    if (/upperarm|lowerarm|hand|clavicle/.test(name))
      return side + 'Arm';
    if (/thigh|calf|foot|ball/.test(name)) return side + 'Leg';
    return 'torso';
  };
  type Sample = { vertex: number; x: number; y: number; z: number; group: string };
  const points: Sample[] = [];
  const grid = new Map<string, Sample[]>();
  const groups = new Map<string, Sample[]>();
  const key = (group: string, x: number, y: number, z: number) =>
    `${group}/${x}/${y}/${z}`;
  for (let i = 0; i < sourcePosition.count; i++) {
    let joint = sourceIndex.getX(i), max = sourceWeight.getX(i);
    for (let k = 1; k < 4; k++) {
      if (sourceWeight.getComponent(i, k) > max) {
        joint = sourceIndex.getComponent(i, k);
        max = sourceWeight.getComponent(i, k);
      }
    }
    const group = label(joint);
    const sample = { vertex: i, x: sourcePosition.getX(i),
      y: sourcePosition.getY(i), z: sourcePosition.getZ(i), group };
    points.push(sample);
    const groupPoints = groups.get(group) ?? [];
    groupPoints.push(sample);
    groups.set(group, groupPoints);
    const address = key(group, Math.floor(sample.x / spacing),
      Math.floor(sample.y / spacing), Math.floor(sample.z / spacing));
    const cell = grid.get(address) ?? [];
    cell.push(sample);
    grid.set(address, cell);
  }
  if (!points.length) return false;
  let transferred = 0;
  for (let i = 0; i < vertices.count; i++) {
    const x = vertices.getX(i), y = vertices.getY(i), z = vertices.getZ(i);
    const base = [Math.floor(x / spacing), Math.floor(y / spacing),
      Math.floor(z / spacing)];
    const initialGroup = label(outputIndex.getX(i));
    const nearest: { sample: Sample; distance: number }[] = [];
    const add = (sample: Sample) => {
      const dx = sample.x - x, dy = sample.y - y, dz = sample.z - z;
      const distance = dx * dx + dy * dy + dz * dz;
      if (!Number.isFinite(distance)) return;
      if (nearest.length === 3 && distance >= nearest[nearest.length - 1].distance) return;
      nearest.push({ sample, distance });
      nearest.sort((a, b) => a.distance - b.distance);
      if (nearest.length > 3) nearest.pop();
    };
    for (let radius = 0; radius <= 3 && nearest.length < 3; radius++) {
      for (let ix = -radius; ix <= radius; ix++) {
        for (let iy = -radius; iy <= radius; iy++) {
          for (let iz = -radius; iz <= radius; iz++) {
            if (radius && Math.max(Math.abs(ix), Math.abs(iy), Math.abs(iz)) !== radius)
              continue;
            for (const sample of grid.get(key(initialGroup,
              base[0] + ix, base[1] + iy, base[2] + iz)) ?? []) add(sample);
          }
        }
      }
    }
    if (nearest.length < 3)
      for (const sample of groups.get(initialGroup) ?? points) add(sample);
    if (!nearest.length) return false;
    const influence = new Map<number, number>();
    let sum = 0;
    for (const n of nearest) {
      const weight = 1 / Math.max(.00001, n.distance);
      sum += weight;
      for (let component = 0; component < 4; component++) {
        const joint = sourceIndex.getComponent(n.sample.vertex, component);
        const strength = sourceWeight.getComponent(n.sample.vertex, component);
        influence.set(joint, (influence.get(joint) ?? 0) + strength * weight);
      }
    }
    const normalized = [...influence].sort((a, b) => b[1] - a[1]).slice(0, 4);
    const total = normalized.reduce((acc, entry) => acc + entry[1], 0);
    if (!(total > 0) || !(sum > 0)) return false;
    for (let k = 0; k < 4; k++) {
      outputIndex.setComponent(i, k, normalized[k]?.[0] ?? normalized[0][0]);
      outputWeight.setComponent(i, k, (normalized[k]?.[1] ?? 0) / total);
    }
    transferred++;
  }
  outputIndex.needsUpdate = true;
  outputWeight.needsUpdate = true;
  return transferred === vertices.count;
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
    ? { index: upperSpineIndex, point: bindPoint(source, upperSpineIndex) }
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
  // Resolve the eye marker into the animated head frame first, then place
  // it in the rest pose. No turning-head event may flip anterior direction.
  const eyePosition = eyeBounds && !eyeBounds.isEmpty()
    ? eyeBounds.getCenter(new THREE.Vector3())
      .applyMatrix4(source.skeleton.bones[get('head').index].matrixWorld.clone().invert())
      .applyMatrix4(source.bindMatrix.clone().invert()
        .multiply(source.skeleton.boneInverses[get('head').index].clone().invert()))
    : null;
  // The eye landmark is resolved once at actor creation. Subsequent phenotype
  // refreshes can occur while looking sideways or bending, and must not
  // reinterpret the permanent anterior axis from the CURRENT animation.
  const inferredFront = eyePosition
    && Math.abs(eyePosition.z - headBounds.getCenter(new THREE.Vector3()).z) > .005
    ? Math.sign(eyePosition.z - headBounds.getCenter(new THREE.Vector3()).z) : 0;
  const cachedFront = source.userData.residentAnatomyFrontSign;
  const front = cachedFront === 1 || cachedFront === -1
    ? cachedFront : inferredFront;
  if (front !== 1 && front !== -1) return null;
  source.userData.residentAnatomyFrontSign = front;
  const frontVector = new THREE.Vector3(0, 0, front);
  const sideVector = new THREE.Vector3(1, 0, 0);
  const body = new AnatomicalSurface();
  const female = profile.bodyMaturity01 * (profile.breastVolume > 0 ? 1 : 0);
  const male = profile.bodyMaturity01 * (profile.breastVolume === 0 ? 1 : 0);
  const halfShoulder = shoulderWidth * .5 * profile.shoulderScale;
  const halfHip = hipWidth * .70 * profile.hipScale;
  const waist = (halfShoulder * .88 + halfHip * .9) * .5 * profile.waistScale;
  const line = (t: number) => {
    const f = clamp(t, 0, 1) * (points.length - 1);
    const i = Math.min(points.length - 2, Math.floor(f));
    return interpolate(points[i].point, points[i + 1].point, f - i);
  };
  const radii = [
    { t: 0, x: halfHip * 1.05, z: halfHip * .66 },
    { t: .18, x: waist, z: halfHip * .68 },
    { t: .30, x: waist * .97, z: halfHip * .71 },
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
  body.addLoft(48, 48, t => {
    const r = radius(t);
    return { center: line(t), radialX: sideVector, radialZ: frontVector,
      radiusX: r.x, radiusZ: r.z, weights: blended(points, t),
      frontBump: (.014 + male * .012) * scale * gaussian(t, .59, .18) };
  }, (t, angle) => {
    const x = Math.sin(angle), f = Math.max(0, Math.cos(angle));
    const chest = female * profile.breastVolume * .24 * scale * gaussian(t, .65, .115)
      * gaussian(Math.abs(x), .43, .27) * f * f;
    // Symmetric anatomical landmarks are bounded and blend into the skin.
    const pectoral = male * .014 * scale * gaussian(t, .66, .15)
      * gaussian(Math.abs(x), .45, .32) * f;
    const clavicle = .011 * scale * gaussian(t, .88, .055)
      * gaussian(Math.abs(x), .62, .28) * f;
    const sternum = .007 * scale * gaussian(t, .73, .18)
      * gaussian(x, 0, .13) * f;
    const costal = .006 * scale * gaussian(t, .43, .09)
      * gaussian(Math.abs(x), .77, .22) * f;
    const oblique = .004 * scale * gaussian(t, .34, .12)
      * Math.cos(8 * angle) * f;
    const scapula = .006 * scale * gaussian(t, .78, .13)
      * gaussian(Math.abs(x), .75, .2) * Math.max(0, -Math.cos(angle));
    return frontVector.clone().multiplyScalar(
      chest + pectoral + clavicle + sternum + costal + oblique - scapula
    );
  });

  // A fully authored high-resolution cranium and jaw, with subtle nose bridge,
  // cheek planes and brows. Eyes and hair keep their existing bone attachments.
  const headCenter = headBounds.getCenter(new THREE.Vector3());
  const headSize = headBounds.getSize(new THREE.Vector3());
  body.addLoft(36, 48, t => {
    const ellipse = Math.max(.035, Math.sin(Math.PI * t) ** .48);
    const jaw = 1 - (.19 + female * .04) * (1 - THREE.MathUtils.smoothstep(t, .17, .58));
    return { center: new THREE.Vector3(headCenter.x,
      THREE.MathUtils.lerp(headBounds.min.y, headBounds.max.y, t),
      headCenter.z),
    radialX: sideVector, radialZ: frontVector,
    radiusX: headSize.x * .5 * ellipse * jaw * profile.headWidthScale
      * (1 + (profile.jawScale - 1) * gaussian(t, .23, .17)),
    radiusZ: headSize.z * .53 * ellipse * (1 + .06 * gaussian(t, .62, .19))
      * (1 + (profile.faceLengthScale - 1) * .32 * gaussian(t, .32, .3)),
    weights: [{ index: get('head').index, weight: 1 }] };
  }, (t, angle) => {
    const facing = Math.max(0, Math.cos(angle));
    const lateral = Math.abs(Math.sin(angle));
    const midline = Math.sin(angle);
    const cheek = .022 * scale * profile.cheekScale
      * gaussian(t, .48, .10) * gaussian(lateral, .56, .21);
    // Eye sockets sit below the supraorbital ridge, not a single flat plane.
    const orbital = -.012 * scale * gaussian(t, .60, .056)
      * gaussian(lateral, .42, .20);
    const brow = .013 * scale * (1 + male * .32) * gaussian(t, .655, .045)
      * gaussian(lateral, .43, .24);
    const nose = .039 * scale * profile.noseScale
      * gaussian(t, .445, .10) * gaussian(midline, 0, .16);
    const tip = .018 * scale * profile.noseScale
      * gaussian(t, .397, .040) * gaussian(midline, 0, .18);
    const alar = .010 * scale * gaussian(t, .386, .032)
      * gaussian(lateral, .22, .11);
    const philtrum = .004 * scale * gaussian(t, .323, .034)
      * gaussian(midline, 0, .065);
    const mouthWidth = Math.max(.12, profile.mouthWidthScale * .28);
    const lips = (.0085 * gaussian(t, .289, .022)
      + .010 * gaussian(t, .253, .023)) * scale
      * gaussian(midline, 0, mouthWidth);
    const mouthCrease = -.004 * scale * gaussian(t, .271, .009)
      * gaussian(midline, 0, mouthWidth);
    const chin = .012 * scale * (1 + male * .16) * gaussian(t, .165, .067)
      * gaussian(midline, 0, .31);
    return frontVector.clone().multiplyScalar(
      facing * (cheek + orbital + brow + nose + tip + alar
        + philtrum + lips + mouthCrease + chin)
    );
  });

  // Neck has its own smoothly weighted bridge across spine, neck and head.
  const neckBottom = get('neck').point.clone().lerp(upperSpine.point, .35);
  const neckTop = get('head').point.clone().lerp(get('neck').point, .36);
  body.addLoft(12, 28, t => ({
    center: interpolate(neckBottom, neckTop, t),
    radialX: sideVector, radialZ: frontVector,
    radiusX: .072 * scale * (1 - .09 * gaussian(t, .5, .4)),
    radiusZ: .060 * scale,
    frontBump: male * .007 * scale * gaussian(t, .57, .22),
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
    const hand = get(`${side}Hand`);
    const suffix = side === 'left' ? 'l' : 'r';
    const across = Math.sign(hand.point.x) || (side === 'left' ? -1 : 1);
    const named = (name: string): Joint | null => {
      const index = source.skeleton.bones.findIndex(bone => bone.name === name);
      return index < 0 ? null : { index, point: bindPoint(source, index) };
    };
    const digits = ['thumb', 'index', 'middle', 'ring', 'pinky'] as const;
    for (const digit of digits) {
      const names = [`${digit}_01_${suffix}`, `${digit}_02_${suffix}`,
        `${digit}_03_${suffix}`, `${digit}_04_leaf_${suffix}`];
      const chain = names.map(named);
      if (chain.some(j => !j)) continue; // Never invent missing finger joints.
      const joints = chain as Joint[];
      const length = joints.slice(1).reduce((acc, joint, i) =>
        acc + joint.point.distanceTo(joints[i].point), 0);
      if (!(length > scale * .008 && length < scale * .24)) continue;
      // 13 rings per actual digit, skinned to its articulated phalanges.
      body.addLoft(12, 12, t => {
        const f = t * 3, index = Math.min(2, Math.floor(f)), blend = f - index;
        const center = interpolate(joints[index].point, joints[index + 1].point, blend);
        const tangent = joints[index + 1].point.clone().sub(joints[index].point).normalize();
        const radialZ = new THREE.Vector3().crossVectors(tangent, sideVector);
        if (radialZ.lengthSq() < .0001) radialZ.crossVectors(tangent, frontVector);
        radialZ.normalize();
        const radialX = new THREE.Vector3().crossVectors(radialZ, tangent).normalize();
        const thickness = digit === 'thumb' ? 1.20 : digit === 'pinky' ? .73 : 1;
        const taper = .75 + .25 * gaussian(t, .24, .28);
        const tip = 1 - .95 * THREE.MathUtils.smoothstep(t, .91, 1);
        return { center, radialX, radialZ,
          radiusX: .010 * scale * thickness * taper * tip,
          radiusZ: .009 * scale * thickness * taper * tip,
          weights: blended(joints, t) };
      });
    }
    // The palm bridges wrist and articulated metacarpal origins.
    const palmEnds = digits.map(digit => named(`${digit}_01_${suffix}`))
      .filter((joint): joint is Joint => joint !== null);
    if (palmEnds.length === digits.length) {
      const base = new THREE.Vector3();
      for (const joint of palmEnds) base.add(joint.point);
      base.multiplyScalar(1 / palmEnds.length);
      body.addLoft(10, 24, t => ({
        center: interpolate(hand.point, base, t),
        radialX: sideVector, radialZ: frontVector,
        radiusX: scale * (.036 + .006 * Math.sin(Math.PI * t)),
        radiusZ: scale * .017,
        weights: [{ index: hand.index, weight: 1 }],
      }));
    }
    // Actual foot and ball joints preserve toe flexion in walking / crouching.
    const foot = get(`${side}Foot`);
    const ballJoint = named(`ball_${suffix}`);
    const toeJoint = named(`ball_leaf_${suffix}`);
    const end = ballJoint ?? {
      index: foot.index, point: foot.point.clone().addScaledVector(frontVector, .080 * scale),
    };
    const tip = toeJoint ?? {
      index: end.index, point: end.point.clone().addScaledVector(frontVector, .036 * scale),
    };
    const footChain = [foot, end, tip];
    body.addLoft(16, 24, t => {
      const index = t < .5 ? 0 : 1, f = t < .5 ? t * 2 : (t - .5) * 2;
      return {
        center: interpolate(footChain[index].point, footChain[index + 1].point, f),
        radialX: sideVector, radialZ: new THREE.Vector3(0, 1, 0),
        radiusX: scale * (.047 + .007 * gaussian(t, .58, .24)) * (1 - .94 * THREE.MathUtils.smoothstep(t, .92, 1)),
        radiusZ: scale * (.033 - .010 * t),
        weights: blended(footChain, t),
      };
    });
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
  if (!retargetSkinning(source, geometry)) {
    geometry.dispose();
    return null;
  }
  // Ground IK must use the *authored* sole, not an arbitrary imported shoe.
  const localPositions = geometry.getAttribute('position');
  const localIndices = geometry.getAttribute('skinIndex');
  const localWeights = geometry.getAttribute('skinWeight');
  const footSole = { left: .035, right: .035 };
  for (const side of ['left', 'right'] as const) {
    const bone = get(`${side}Foot`);
    let low = Infinity;
    for (let i = 0; i < localPositions.count; i++) {
      let weight = 0;
      for (let k = 0; k < 4; k++) {
        const name = source.skeleton.bones[localIndices.getComponent(i, k)]?.name ?? '';
        if (name === `foot_${side === 'left' ? 'l' : 'r'}`
            || name === `ball_${side === 'left' ? 'l' : 'r'}`)
          weight += localWeights.getComponent(i, k);
      }
      if (weight >= .4) low = Math.min(low, localPositions.getY(i));
    }
    if (Number.isFinite(low))
      footSole[side] = clamp(bone.point.y - low, .014, .095);
  }
  geometry.userData.residentFootSole = footSole;
  geometry.userData.anatomyVersion = 'A6-A7';
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
  if (syncNativeResidentBody(model, profile)) return true;
  const donor = model.getObjectByName('SuperHero_Male');
  if (!(donor instanceof THREE.SkinnedMesh)) return false;
  // Keep the original alive for A1/A2 face & hair attachment bounds, while
  // rendering only the authored body. No skeleton/clip remapping in Core.
  const old = model.getObjectByName(ANATOMICAL_BODY_NAME);
  if (old instanceof THREE.SkinnedMesh) removeResidentAnatomicalBody(model);
  const authored = addAnatomy(donor, profile);
  if (!authored || !donor.parent) return false;
  const material = createResidentSkinMaterial(profile);
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

