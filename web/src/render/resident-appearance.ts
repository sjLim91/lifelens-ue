import * as THREE from 'three';
import type { Resident } from '../runtime/core-types';
import { deriveResidentLifeVisualState } from './resident-life-presentation';

export interface ResidentAppearanceProfile {
  nativeSex?: 'Male' | 'Female' | 'Unknown';
  seed: number;
  heightWorldUnits: number;
  widthScale: number;
  depthScale: number;
  // These describe the body's actual silhouette, not invented clothes.
  waistScale: number;
  breastVolume: number;
  bodyMaturity01: number;
  skinLightnessShift: number;
  skinColor: number;
  hairStyle: number;
  hairColor: number;
  gaitRateBias: number;
  eyeColor: number;
  shoulderScale: number;
  hipScale: number;
  headWidthScale: number;
  jawScale: number;
  cheekScale: number;
  faceLengthScale: number;
  noseScale: number;
  mouthWidthScale: number;
  hairLengthScale: number;
  ageWeathering01: number;
}

const HAIR_PALETTE = [
  0x241a14,
  0x3b281c,
  0x5a3b25,
  0x1c1a19,
  0x72533b,
] as const;

function stableHash(value: string): number {
  let hash = 2166136261 >>> 0;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return hash >>> 0;
}

function hash01(seed: number, salt: number): number {
  let value = (seed ^ Math.imul(salt, 0x9e3779b1)) >>> 0;
  value ^= value >>> 16;
  value = Math.imul(value, 0x7feb352d) >>> 0;
  value ^= value >>> 15;
  value = Math.imul(value, 0x846ca68b) >>> 0;
  value ^= value >>> 16;
  return (value >>> 0) / 4294967296;
}

function phenotype(value: number | undefined, fallback: number): number {
  return typeof value === 'number' && Number.isFinite(value)
    ? THREE.MathUtils.clamp(value, 0, 1) : fallback;
}

function agedHairColor(color: number, weathering: number): number {
  // LifeCondition.appearanceAgeFactor is the Core-owned aging clock. Grey
  // emerges gradually; old observers and new births replay identically.
  const grey = THREE.MathUtils.smoothstep(weathering, .51, 1) * .62;
  return new THREE.Color(color).lerp(new THREE.Color(0xb4b1a6), grey).getHex();
}
function pigmentColor(value: number, colors: readonly number[]): number {
  const position = value * (colors.length - 1);
  const index = Math.min(colors.length - 2, Math.floor(position));
  return new THREE.Color(colors[index]).lerp(new THREE.Color(colors[index + 1]), position - index).getHex();
}

function heightForResident(
  resident: Resident,
  seed: number,
): number {
  const life = deriveResidentLifeVisualState(resident);
  const age = life.age ?? 0;
  const potential = phenotype(resident.genetics?.heightPotential, hash01(seed, 23));
  if (life.stage === 'Baby') return 0.64 + potential * 0.18 + Math.min(2, age) * 0.025;
  if (life.stage === 'Toddler') return 0.94 + potential * 0.2 + Math.max(0, Math.min(3, age - 2)) * 0.035;
  if (life.stage === 'Child') return 1.2 + potential * 0.28 + Math.max(0, Math.min(8, age - 5)) * 0.012;
  if (life.stage === 'Teen') return 1.48 + potential * 0.2 + Math.max(0, Math.min(5, age - 13)) * 0.01;

  const sexMean = resident.sex === 'Male'
    ? 1.72
    : resident.sex === 'Female'
      ? 1.65
      : 1.685;
  const individualVariation = (phenotype(resident.genetics?.heightPotential, hash01(seed, 23)) - 0.5) * 0.28;
  const elderAdjustment =
    Number.isFinite(age) && age >= 70
      ? -Math.min(0.08, (age - 70) * 0.0025)
      : 0;

  return Math.max(
    1.46,
    sexMean + individualVariation + elderAdjustment,
  );
}

export function createResidentAppearanceProfile(
  resident: Resident,
): ResidentAppearanceProfile {
  const seed = stableHash(resident.id);
  const life = deriveResidentLifeVisualState(resident);
  const ageFallback = life.age === null ? 0 : THREE.MathUtils.clamp((life.age - 25) / 65, 0, 1);
  const ageWeathering01 = phenotype(resident.lifeCondition?.appearanceAgeFactor, ageFallback);
  const hairLengthScale = life.stage === 'Baby' ? .25
    : life.stage === 'Toddler' ? .48
    : life.stage === 'Child' ? .72
    : life.stage === 'Teen' ? .86 : 1;
  const sexWidthBias = resident.sex === 'Male'
    ? 1.055 : resident.sex === 'Female' ? 0.95 : 1;
  const build = phenotype(resident.genetics?.buildPotential, hash01(seed, 29));
  const widthVariation = 0.86 + build * 0.26;
  const depthVariation = 0.88 + build * 0.22;
  // Secondary sex-related morphology appears gradually rather than on babies.
  // Small inherited/identity differences still overlap across individuals.
  const bodyMaturity01 = life.stage === 'Baby' || life.stage === 'Toddler' || life.stage === 'Child'
    ? 0 : life.stage === 'Teen' ? 0.6 : 1;
  const female = resident.sex === 'Female' ? bodyMaturity01 : 0;
  const male = resident.sex === 'Male' ? bodyMaturity01 : 0;
  const hairIndex = Math.min(
    HAIR_PALETTE.length - 1,
    Math.floor(hash01(seed, 41) * HAIR_PALETTE.length),
  );

  return {
    seed,
    nativeSex: resident.sex === 'Male' || resident.sex === 'Female' ? resident.sex : 'Unknown',
    heightWorldUnits: heightForResident(resident, seed),
    widthScale: sexWidthBias * widthVariation * life.bodyWidth,
    depthScale: depthVariation,
    waistScale: 1 - female * (0.15 + (1 - build) * 0.025) - male * 0.035,
    // Render-only anatomical volumes derived from Core sex and life stage.
    breastVolume: female * (0.20 + hash01(seed, 45) * 0.06),
    bodyMaturity01,
    skinLightnessShift: (hash01(seed, 47) - 0.5) * 0.055,
    skinColor: typeof resident.genetics?.skinTone === 'number' && Number.isFinite(resident.genetics.skinTone)
      ? pigmentColor(phenotype(resident.genetics.skinTone, .5), [0xe0b394, 0xc99573, 0xb47e5e, 0x946344, 0x704b38])
      : [0xe0b394, 0xc99573, 0xb47e5e, 0x946344, 0x704b38][
      Math.floor(hash01(seed, 61) * 5)
    ],
    hairStyle: Math.floor(hash01(seed, 53) * 8),
    hairColor: agedHairColor(typeof resident.genetics?.hairPigment === 'number' && Number.isFinite(resident.genetics.hairPigment)
      ? pigmentColor(phenotype(resident.genetics.hairPigment, .5), [0xc6a064, 0x805631, 0x3b281c, 0x1c1a19])
      : HAIR_PALETTE[hairIndex], ageWeathering01),
    gaitRateBias: (0.94 + hash01(seed, 59) * 0.12) * life.gaitBias,
    eyeColor: pigmentColor(phenotype(resident.genetics?.eyePigment, 1), [0x547d8c, 0x627255, 0x705336, 0x302720]),
    shoulderScale: 1 + male * .10 - female * .10 + (build - .5) * .035,
    hipScale: 1 + female * .14 - male * .02 + (build - .5) * .035,
    headWidthScale: (.94 + phenotype(resident.genetics?.faceShape, .5) * .12) * life.headScale,
    jawScale: (.83 + phenotype(resident.genetics?.faceShape, hash01(seed, 83)) * .34)
      * (1 + male * .055 - female * .045),
    cheekScale: (.90 + phenotype(resident.genetics?.faceShape, hash01(seed, 85)) * .21)
      * (1 + female * .025),
    faceLengthScale: 1.09 - phenotype(resident.genetics?.faceShape, hash01(seed, 87)) * .17,
    // Only existing inherited faceShape is genetic truth. The small GUID salt
    // gives siblings individual visual detail, never a fabricated Core allele.
    noseScale: .83 + phenotype(resident.genetics?.faceShape, hash01(seed, 91)) * .27
      + (hash01(seed, 93) - .5) * .11,
    mouthWidthScale: .83 + phenotype(resident.genetics?.faceShape, hash01(seed, 95)) * .24
      + (hash01(seed, 97) - .5) * .13,
    hairLengthScale,
    ageWeathering01,
  };
}

function materialIdentity(
  object: THREE.Object3D,
  material: THREE.Material,
): string {
  return `${object.name} ${material.name}`.toLowerCase();
}

function isSkinMaterial(identity: string): boolean {
  return /(skin|face|flesh|head|body_skin)/i.test(identity);
}

function isDetailMaterial(identity: string): boolean {
  return /(eye|iris|pupil|teeth|tooth|mouth|tongue)/i.test(identity);
}

export function applyResidentMaterialVariant(
  root: THREE.Object3D,
  profile: ResidentAppearanceProfile,
): void {
  root.updateMatrixWorld(true);
  const referenceInverse = root.matrixWorld.clone().invert();

  root.traverse((object) => {
    if (!(object instanceof THREE.Mesh)
        || object.userData.residentProceduralAppearance) return;

    // Start each update from the source, not the last tinted/deformed copy.
    // Runtime summaries may arrive before the full genetics observation.
    const previousMaterials = Array.isArray(object.material) ? object.material : [object.material];
    if (object.userData.residentSourceMaterial) {
      object.material = object.userData.residentSourceMaterial;
      for (const material of previousMaterials) material.dispose();
    } else {
      object.userData.residentSourceMaterial = object.material;
    }
    if (object.userData.residentSourceGeometry) {
      if (object.userData.residentOwnsGeometry) object.geometry.dispose();
      object.geometry = object.userData.residentSourceGeometry;
      object.userData.residentOwnsGeometry = false;
    } else {
      object.userData.residentSourceGeometry = object.geometry;
    }

    const cloneMaterial = (
      material: THREE.Material,
    ): THREE.Material => {
      const cloned = material.clone();
      if (!(cloned instanceof THREE.MeshStandardMaterial)) {
        return cloned;
      }

      cloned.roughness = Math.max(0.5, cloned.roughness);
      cloned.metalness = Math.min(0.035, cloned.metalness);
      const identity = materialIdentity(object, cloned);

      // The current pinned GLB shares one jade material across body and eyes.
      // Use its actual skinning groups to preserve exposed head/hand skin.
      if (object instanceof THREE.SkinnedMesh && object.name === 'SuperHero_Male') {
        applyBodyColors(object, profile, referenceInverse);
        cloned.color.set(0xffffff);
        cloned.vertexColors = true;
        return cloned;
      }
      if (/eyebrow/i.test(identity)) {
        cloned.color.set(profile.hairColor);
        return cloned;
      }
      if (/^eyes\b/i.test(identity)) {
        cloned.color.set(profile.eyeColor);
        return cloned;
      }

      if (isDetailMaterial(identity)) {
        return cloned;
      }

      if (isSkinMaterial(identity)) {
        cloned.color.set(profile.skinColor).offsetHSL(
          0,
          0,
          profile.skinLightnessShift,
        );
        return cloned;
      }

      // The imported base GLB is a shared rigged human body, not owned clothing.
      // The old ID-based upper/lower/shoe tints incorrectly created free outfits.
      // Model clothing only in resident-authored-garment from Core wornGarment.
      cloned.color.set(profile.skinColor);
      return cloned;
    };

    object.material = Array.isArray(object.material)
      ? object.material.map(cloneMaterial)
      : cloneMaterial(object.material);
    applyBodyProportions(object, profile, referenceInverse, root);
  });
}

/** Small rest-mesh adjustments; no invented asset gender or skeleton changes. */
export function applyBodyProportions(mesh: THREE.Mesh, profile: ResidentAppearanceProfile, referenceInverse: THREE.Matrix4, root: THREE.Object3D): void {
  if (!(mesh instanceof THREE.SkinnedMesh)) return;
  const joints = mesh.geometry.getAttribute('skinIndex');
  const weights = mesh.geometry.getAttribute('skinWeight');
  const positions = mesh.geometry.getAttribute('position');
  if (!joints || !weights || !positions) return;
  if (profile.shoulderScale === 1 && profile.hipScale === 1 && profile.waistScale === 1
      && profile.breastVolume === 0 && profile.headWidthScale === 1
      && profile.jawScale === 1 && profile.cheekScale === 1 && profile.faceLengthScale === 1) return;
  if (!mesh.userData.residentOwnsGeometry) mesh.geometry = mesh.geometry.clone();
  mesh.userData.residentOwnsGeometry = true;
  const output = mesh.geometry.getAttribute('position');
  const point = new THREE.Vector3();
  const relative = referenceInverse.clone().multiply(mesh.matrixWorld);
  const inverse = relative.clone().invert();
  // Derive anatomy from the actual *rest* vertices influenced by the body's
  // spine. This is deterministic and independent of a later animated pose.
  let minHeadY = Infinity, maxHeadY = -Infinity;
  const torsoBox = new THREE.Box3();
  let torsoCount = 0, headZ = 0, headCount = 0;
  for (let vertex = 0; vertex < positions.count; vertex++) {
    let headInfluence = 0, torsoInfluence = 0;
    for (let i = 0; i < 4; i++) {
      const name = mesh.skeleton.bones[joints.getComponent(vertex, i)]?.name ?? '';
      const weight = weights.getComponent(vertex, i);
      if (/^head$/i.test(name)) headInfluence += weight;
      if (/^spine_0[123]$/i.test(name)) torsoInfluence += weight;
    }
    point.fromBufferAttribute(positions, vertex).applyMatrix4(relative);
    if (headInfluence >= .5) {
      minHeadY = Math.min(minHeadY, point.y);
      maxHeadY = Math.max(maxHeadY, point.y);
      headZ += point.z; headCount++;
    }
    if (torsoInfluence >= .5) { torsoBox.expandByPoint(point); torsoCount++; }
  }
  const headHeight = Math.max(0.0001, maxHeadY - minHeadY);
  const validHeadRange = Number.isFinite(headHeight)
    && Number.isFinite(maxHeadY) && Number.isFinite(minHeadY);
  const torsoSize = torsoBox.getSize(new THREE.Vector3());
  const torsoValid = torsoCount >= 12 && !torsoBox.isEmpty()
    && torsoSize.y > .08 && torsoSize.z > .025 && torsoSize.x > .055;
  // Eye landmark establishes the real rig's anterior direction. Never guess
  // the orientation of an unsupported imported skeleton.
  const eyes = root.getObjectByName('Eyes');
  const eyesWorld = eyes ? new THREE.Box3().setFromObject(eyes) : null;
  const eyesZ = eyesWorld && !eyesWorld.isEmpty()
    ? eyesWorld.getCenter(new THREE.Vector3()).applyMatrix4(referenceInverse).z : NaN;
  const eyeDepth = headCount > 0 ? eyesZ - headZ / headCount : 0;
  const frontSign = Number.isFinite(eyeDepth) && Math.abs(eyeDepth) > torsoSize.z * .06
    ? Math.sign(eyeDepth) : 0;
  // The normalized source is centered on X. Use rest vertices, never animated
  // bone positions: a late genetics observation must not bake the current pose.
  for (let vertex = 0; vertex < output.count; vertex++) {
    let headWeight = 0, torsoWeight = 0, hipWeight = 0, midTorsoWeight = 0;
    for (let i = 0; i < 4; i++) {
      const name = mesh.skeleton.bones[joints.getComponent(vertex, i)]?.name ?? '';
      const weight = weights.getComponent(vertex, i);
      if (/^head$/i.test(name)) headWeight += weight;
      if (/^spine_0[23]$/i.test(name)) torsoWeight += weight;
      if (/^spine_0[123]$/i.test(name)) midTorsoWeight += weight;
      if (/^pelvis$/i.test(name)) hipWeight += weight;
    }
    point.fromBufferAttribute(positions, vertex).applyMatrix4(relative);
    point.x += point.x * ((profile.shoulderScale - 1) * torsoWeight + (profile.hipScale - 1) * hipWeight);
    if (torsoValid && midTorsoWeight > 0) {
      const ribHeight = (point.y - torsoBox.min.y) / torsoSize.y;
      // Waist narrowing is smooth across bones to prevent seams in motion.
      const waistMask = Math.exp(-Math.pow((ribHeight - .28) / .25, 2));
      point.x += point.x * (profile.waistScale - 1) * waistMask * midTorsoWeight;
      if (frontSign !== 0 && profile.breastVolume > 0) {
        const midX = (torsoBox.min.x + torsoBox.max.x) * .5;
        const radiusX = torsoSize.x * .5;
        const relativeX = Math.abs(point.x - midX) / Math.max(.001, radiusX);
        const bustMask = Math.exp(-Math.pow((ribHeight - .67) / .18, 2))
          * Math.exp(-Math.pow((relativeX - .43) / .33, 2));
        const depthFromBack = frontSign > 0
          ? point.z - torsoBox.min.z : torsoBox.max.z - point.z;
        const frontMask = THREE.MathUtils.smoothstep(depthFromBack / torsoSize.z, .42, .78);
        point.z += frontSign * torsoSize.z * profile.breastVolume
          * bustMask * frontMask * midTorsoWeight;
      }
    }
    point.x += point.x * (profile.headWidthScale - 1) * headWeight;
    if (headWeight > 0 && validHeadRange) {
      const faceHeight01 = THREE.MathUtils.clamp(
        (point.y - minHeadY) / headHeight, 0, 1,
      );
      const jawMask = 1 - THREE.MathUtils.smoothstep(faceHeight01, .20, .65);
      const cheekMask = 1 - Math.min(1, Math.abs(faceHeight01 - .57) * 3.5);
      point.x += point.x * headWeight * (
        (profile.jawScale - 1) * jawMask
        + (profile.cheekScale - 1) * cheekMask
      );
      point.y += (point.y - maxHeadY) * (profile.faceLengthScale - 1)
        * jawMask * headWeight;
    }
    point.applyMatrix4(inverse);
    output.setXYZ(vertex, point.x, point.y, point.z);
  }
  output.needsUpdate = true;
  mesh.geometry.computeVertexNormals();
  mesh.geometry.computeBoundingBox();
  mesh.geometry.computeBoundingSphere();
}

function applyBodyColors(
  mesh: THREE.SkinnedMesh,
  profile: ResidentAppearanceProfile,
  referenceInverse: THREE.Matrix4,
): void {
  const joints = mesh.geometry.getAttribute('skinIndex');
  const weights = mesh.geometry.getAttribute('skinWeight');
  if (!joints || !weights) return;
  // Geometry is shared by SkeletonUtils.clone; skin buffers must be per person.
  mesh.geometry = mesh.geometry.clone();
  mesh.userData.residentOwnsGeometry = true;
  const headIndex = mesh.skeleton.bones.findIndex(bone => /^head$/i.test(bone.name));
  const positions = mesh.geometry.getAttribute('position');
  const headWeights = new Float32Array(joints.count);
  const heights = new Float32Array(joints.count);
  const point = new THREE.Vector3();
  const relative = referenceInverse.clone().multiply(mesh.matrixWorld);
  let headBottom = Infinity;
  let headTop = -Infinity;
  for (let vertex = 0; vertex < joints.count; vertex++) {
    for (let component = 0; component < 4; component++) {
      if (joints.getComponent(vertex, component) === headIndex) {
        headWeights[vertex] += weights.getComponent(vertex, component);
      }
    }
    point.fromBufferAttribute(positions, vertex).applyMatrix4(relative);
    heights[vertex] = point.y;
    if (headWeights[vertex] > 0.5) {
      headBottom = Math.min(headBottom, point.y);
      headTop = Math.max(headTop, point.y);
    }
  }
  const headHeight = headTop - headBottom;
  const hairColor = new THREE.Color(profile.hairColor);
  // Color the actual skinned scalp, not a fixed-height sphere that can become
  // a collar when the imported rig changes pose. Keep face and neck uncovered.
  const hairline = 0.64 + profile.hairStyle * 0.025;
  const colors = new Float32Array(joints.count * 3);
  const skin = new THREE.Color(profile.skinColor);
  const color = new THREE.Color();
  for (let vertex = 0; vertex < joints.count; vertex++) {
    // Every body vertex is naked skin until a real Core garment is owned.
    // No shoe/trouser/tunic palette can be inferred from a GUID or sex.
    color.copy(skin);

    if (
      headWeights[vertex] > 0.5
      && Number.isFinite(headHeight)
      && headHeight > 0.0001
    ) {
      const scalpHeight = (heights[vertex] - headBottom) / headHeight;
      color.lerp(
        hairColor,
        THREE.MathUtils.smoothstep(
          scalpHeight,
          hairline,
          hairline + 0.05,
        ),
      );
    }
    color.toArray(colors, vertex * 3);
  }
  mesh.geometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));
}


