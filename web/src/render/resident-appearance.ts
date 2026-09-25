import * as THREE from 'three';
import type { Resident } from '../runtime/core-types';

export interface ResidentAppearanceProfile {
  seed: number;
  heightWorldUnits: number;
  widthScale: number;
  depthScale: number;
  garmentColor: number;
  garmentMix: number;
  skinLightnessShift: number;
  skinColor: number;
  hairStyle: number;
  hairColor: number;
  gaitRateBias: number;
}

const GARMENT_PALETTE = [
  0x2f5f86,
  0x8a4d35,
  0x4f7448,
  0x8a6b2f,
  0x62528c,
  0x8a5067,
  0x2f746a,
  0x6e6654,
] as const;

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

function heightForResident(
  resident: Resident,
  seed: number,
): number {
  const age = Number(resident.ageYears);
  if (Number.isFinite(age)) {
    if (age < 2) return 0.72 + hash01(seed, 11) * 0.18;
    if (age < 6) return 0.94 + hash01(seed, 13) * 0.2;
    if (age < 13) return 1.2 + hash01(seed, 17) * 0.28;
    if (age < 18) return 1.48 + hash01(seed, 19) * 0.2;
  }

  const sexMean = resident.sex === 'Male'
    ? 1.72
    : resident.sex === 'Female'
      ? 1.65
      : 1.685;
  const individualVariation = (hash01(seed, 23) - 0.5) * 0.28;
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
  const sexWidthBias = resident.sex === 'Male'
    ? 1.05
    : resident.sex === 'Female'
      ? 0.94
      : 1;
  const widthVariation = 0.86 + hash01(seed, 29) * 0.26;
  const depthVariation = 0.88 + hash01(seed, 31) * 0.22;
  const garmentIndex = Math.min(
    GARMENT_PALETTE.length - 1,
    Math.floor(hash01(seed, 37) * GARMENT_PALETTE.length),
  );
  const hairIndex = Math.min(
    HAIR_PALETTE.length - 1,
    Math.floor(hash01(seed, 41) * HAIR_PALETTE.length),
  );

  return {
    seed,
    heightWorldUnits: heightForResident(resident, seed),
    widthScale: sexWidthBias * widthVariation,
    depthScale: depthVariation,
    garmentColor: GARMENT_PALETTE[garmentIndex],
    garmentMix: 0.64 + hash01(seed, 43) * 0.18,
    skinLightnessShift: (hash01(seed, 47) - 0.5) * 0.055,
    skinColor: [0xe0b394, 0xc99573, 0xb47e5e, 0x946344, 0x704b38][
      Math.floor(hash01(seed, 61) * 5)
    ],
    hairStyle: Math.floor(hash01(seed, 53) * 4),
    hairColor: HAIR_PALETTE[hairIndex],
    gaitRateBias: 0.94 + hash01(seed, 59) * 0.12,
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
  const garmentColor = new THREE.Color(profile.garmentColor);

  root.traverse((object) => {
    if (!(object instanceof THREE.Mesh)) return;

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
        applyBodyColors(object, profile);
        cloned.color.set(0xffffff);
        cloned.vertexColors = true;
        return cloned;
      }
      if (/eyebrow/i.test(identity)) {
        cloned.color.set(profile.hairColor);
        return cloned;
      }
      if (/^eyes\b/i.test(identity)) {
        cloned.color.set(0xe8e0d4);
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

      const materialSalt = stableHash(identity);
      cloned.color.lerp(garmentColor, profile.garmentMix);
      cloned.color.offsetHSL(
        (hash01(profile.seed, materialSalt + 71) - 0.5) * 0.035,
        0.08,
        (hash01(profile.seed, materialSalt + 73) - 0.5) * 0.06,
      );
      return cloned;
    };

    object.material = Array.isArray(object.material)
      ? object.material.map(cloneMaterial)
      : cloneMaterial(object.material);
  });
}

function applyBodyColors(
  mesh: THREE.SkinnedMesh,
  profile: ResidentAppearanceProfile,
): void {
  const joints = mesh.geometry.getAttribute('skinIndex');
  const weights = mesh.geometry.getAttribute('skinWeight');
  if (!joints || !weights) return;
  // Geometry is shared by SkeletonUtils.clone; color buffers must be per person.
  mesh.geometry = mesh.geometry.clone();
  mesh.userData.residentOwnsGeometry = true;
  const skinBones = new Set(mesh.skeleton.bones.flatMap((bone, index) =>
    /^(Head|neck_01|hand_[lr]|(?:thumb|index|middle|ring|pinky)_\d+.*)$/i.test(bone.name)
      ? [index] : []));
  const colors = new Float32Array(joints.count * 3);
  const garment = new THREE.Color(profile.garmentColor);
  const skin = new THREE.Color(profile.skinColor);
  const color = new THREE.Color();
  for (let vertex = 0; vertex < joints.count; vertex++) {
    let skinWeight = 0;
    for (let component = 0; component < 4; component++) {
      if (skinBones.has(joints.getComponent(vertex, component))) {
        skinWeight += weights.getComponent(vertex, component);
      }
    }
    color.copy(garment).lerp(skin, THREE.MathUtils.smoothstep(skinWeight, 0.25, 0.75));
    color.toArray(colors, vertex * 3);
  }
  mesh.geometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));
}

function makeHairMaterial(color: number): THREE.MeshLambertMaterial {
  return new THREE.MeshLambertMaterial({ color });
}

export function addResidentHairVariant(
  normalizedModel: THREE.Group,
  profile: ResidentAppearanceProfile,
): void {
  const material = makeHairMaterial(profile.hairColor);
  const hair = new THREE.Group();
  hair.name = 'LifeLensResidentHairVariant';

  const cap = new THREE.Mesh(
    new THREE.SphereGeometry(0.105, 10, 6),
    material,
  );
  cap.position.set(0, 0.925, 0);
  cap.scale.set(1.02, 0.55, 1.02);
  hair.add(cap);

  if (profile.hairStyle === 1) {
    cap.scale.set(1.08, 0.72, 1.08);
    cap.position.y = 0.93;
  } else if (profile.hairStyle === 2) {
    const side = hash01(profile.seed, 79) > 0.5 ? 1 : -1;
    const bun = new THREE.Mesh(
      new THREE.SphereGeometry(0.055, 8, 5),
      material,
    );
    bun.position.set(side * 0.095, 0.915, -0.025);
    bun.scale.set(0.9, 1.05, 0.9);
    hair.add(bun);
  } else if (profile.hairStyle === 3) {
    const back = new THREE.Mesh(
      new THREE.SphereGeometry(0.072, 8, 5),
      material,
    );
    back.position.set(0, 0.855, -0.065);
    back.scale.set(0.82, 1.55, 0.72);
    hair.add(back);
  }

  hair.traverse((object) => {
    if (object instanceof THREE.Mesh) {
      object.castShadow = false;
      object.receiveShadow = false;
    }
  });
  normalizedModel.add(hair);
  normalizedModel.updateMatrixWorld(true);
  let head: THREE.Bone | undefined;
  normalizedModel.traverse((object) => {
    if (object instanceof THREE.Bone && /^head$/i.test(object.name)) head = object;
  });
  // Preserve the fitted bind-pose transform, then follow head animation.
  if (head) head.attach(hair);
  else {
    normalizedModel.remove(hair);
    hair.traverse((object) => {
      if (object instanceof THREE.Mesh) object.geometry.dispose();
    });
    material.dispose();
  }
  hair.traverse((object) => {
    if (object instanceof THREE.Mesh) object.userData.residentOwnsGeometry = true;
  });
}
