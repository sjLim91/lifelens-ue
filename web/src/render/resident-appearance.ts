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
  hairStyle: number;
  hairColor: number;
  gaitRateBias: number;
}

const GARMENT_PALETTE = [
  0x405b70,
  0x6e493c,
  0x536445,
  0x725d38,
  0x4f496d,
  0x6b4d59,
  0x375d58,
  0x665f50,
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
  return value / 4294967295;
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
  const individualVariation = (hash01(seed, 23) - 0.5) * 0.2;
  const elderAdjustment =
    Number.isFinite(age) && age >= 70
      ? -Math.min(0.08, (age - 70) * 0.0025)
      : 0;

  return Math.max(
    1.48,
    sexMean + individualVariation + elderAdjustment,
  );
}

export function createResidentAppearanceProfile(
  resident: Resident,
): ResidentAppearanceProfile {
  const seed = stableHash(resident.id);
  const sexWidthBias = resident.sex === 'Male'
    ? 1.025
    : resident.sex === 'Female'
      ? 0.965
      : 0.995;
  const widthVariation = 0.9 + hash01(seed, 29) * 0.18;
  const depthVariation = 0.92 + hash01(seed, 31) * 0.16;
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
    garmentMix: 0.28 + hash01(seed, 43) * 0.2,
    skinLightnessShift: (hash01(seed, 47) - 0.5) * 0.055,
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

      if (isDetailMaterial(identity)) {
        return cloned;
      }

      if (isSkinMaterial(identity)) {
        cloned.color.offsetHSL(
          0,
          0,
          profile.skinLightnessShift,
        );
        return cloned;
      }

      cloned.color.lerp(garmentColor, profile.garmentMix);
      cloned.color.offsetHSL(
        (hash01(profile.seed, object.id + 71) - 0.5) * 0.035,
        0.04,
        (hash01(profile.seed, object.id + 73) - 0.5) * 0.04,
      );
      return cloned;
    };

    object.material = Array.isArray(object.material)
      ? object.material.map(cloneMaterial)
      : cloneMaterial(object.material);
  });
}

function makeHairMaterial(color: number): THREE.MeshLambertMaterial {
  return new THREE.MeshLambertMaterial({
    color,
    roughness: 1,
  } as THREE.MeshLambertMaterialParameters);
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
}
