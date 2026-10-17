import * as THREE from 'three';
import type { GLTFLoader, GLTF } from 'three/addons/loaders/GLTFLoader.js';
import type { ResidentAppearanceProfile } from './resident-appearance';
import { applyBodyProportions } from './resident-appearance';
import { createResidentSkinMaterial } from './resident-surface-material';

const GROUP = 'LifeLensNativeResident';
const BODY = 'LifeLensAuthoredAnatomicalBody';
const templates = new Map<string, GLTF>();
let loading: Promise<void> | undefined;

/** Load verified, locally published assets once. An unavailable asset retains
 * the established resident presentation; it never replaces Core observations. */
export function loadNativeResidentTemplates(loader: GLTFLoader): Promise<void> {
  if (loading) return loading;
  loading = (async () => {
    const staged = new Map<string, GLTF>();
    for (const sex of ['female', 'male']) {
      for (const tier of ['near', 'mid', 'far']) {
        const suffix = tier === 'near' ? '' : '-' + tier;
        const gltf = await loader.loadAsync(`${import.meta.env.BASE_URL}vendor/characters/resident-${sex}${suffix}.glb`);
        const body = gltf.scene.getObjectByName('NativeResidentBody');
        if (!(body instanceof THREE.SkinnedMesh) || !body.geometry.getAttribute('uv'))
          throw new Error('Native resident asset lacks its skinned UV surface');
        staged.set(sex + '/' + tier, gltf);
      }
    }
    for (const [key, gltf] of staged) templates.set(key, gltf);
  })().catch(error => { console.warn('LifeLens native resident assets unavailable; retaining existing presentation', error); });
  return loading;
}

function disposeNative(group: THREE.Object3D): void {
  group.traverse(object => {
    if (!(object instanceof THREE.Mesh)) return;
    object.geometry.dispose();
    for (const material of Array.isArray(object.material) ? object.material : [object.material]) material.dispose();
  });
  group.removeFromParent();
}

/** The new surfaces use the actual existing actor skeleton, not a second
 * animation system. Shared source textures stay alive across actor disposal. */
export function syncNativeResidentBody(model: THREE.Group, profile: ResidentAppearanceProfile): boolean {
  const sex = profile.nativeSex === 'Female' ? 'female' : profile.nativeSex === 'Male' ? 'male' : null;
  const donor = model.getObjectByName('SuperHero_Male');
  if (!sex || !(donor instanceof THREE.SkinnedMesh) || !donor.parent || !templates.has(sex + '/near')) return false;
  const old = model.getObjectByName(GROUP);
  if (old) disposeNative(old);
  const previous = model.getObjectByName(BODY);
  if (previous instanceof THREE.Mesh) {
    previous.geometry.dispose();
    for (const material of Array.isArray(previous.material) ? previous.material : [previous.material]) material.dispose();
    previous.removeFromParent();
  }
  const group = new THREE.Group(); group.name = GROUP;
  const donorIndices = new Map(donor.skeleton.bones.map((bone, i) => [bone.name, i]));
  try {
    for (const tier of ['near', 'mid', 'far']) {
      const gltf = templates.get(sex + '/' + tier);
      if (!gltf) throw new Error('Incomplete native resident LOD set');
      const level = new THREE.Group(); level.name = 'NativeResidentLOD:' + tier;
      level.visible = tier === 'near'; group.add(level);
      gltf.scene.traverse(source => {
        if (!(source instanceof THREE.SkinnedMesh) || !source.name.startsWith('NativeResident')) return;
        const geometry = source.geometry.clone();
        const indices = geometry.getAttribute('skinIndex');
        const remapped = new Uint16Array(indices.count * 4);
        for (let v = 0; v < indices.count; v++) for (let k = 0; k < 4; k++) {
          const name = source.skeleton.bones[indices.getComponent(v, k)]?.name;
          const index = donorIndices.get(name ?? '');
          if (index === undefined) { geometry.dispose(); throw new Error('Native resident has an incompatible joint: ' + name); }
          remapped[v * 4 + k] = index;
        }
        geometry.setAttribute('skinIndex', new THREE.Uint16BufferAttribute(remapped, 4));
        geometry.applyMatrix4(donor.bindMatrix.clone().invert().multiply(source.bindMatrix));
        const originalMaterial = Array.isArray(source.material) ? source.material[0] : source.material;
        if (!(originalMaterial instanceof THREE.MeshStandardMaterial)) { geometry.dispose(); throw new Error('Native resident requires a standard PBR material'); }
        const isBody = source.name === 'NativeResidentBody';
        const material = isBody ? createResidentSkinMaterial(profile) : originalMaterial.clone();
        material.map = originalMaterial.map;
        material.alphaMap = originalMaterial.alphaMap;
        if (!isBody && source.name === 'NativeResidentHair') material.color.set(profile.hairColor);
        if (source.name === 'NativeResidentEyes') {
          material.onBeforeCompile = shader => {
            shader.uniforms.nativeEyeTint = { value: new THREE.Color(profile.eyeColor) };
            shader.fragmentShader = 'uniform vec3 nativeEyeTint;\n' + shader.fragmentShader;
            shader.fragmentShader = shader.fragmentShader.replace('#include <color_fragment>',
              '#include <color_fragment>\nfloat eyeLuma=dot(diffuseColor.rgb,vec3(.2126,.7152,.0722));\nfloat irisMask=smoothstep(.025,.09,eyeLuma)*(1.0-smoothstep(.5,.8,eyeLuma));\ndiffuseColor.rgb=mix(diffuseColor.rgb,nativeEyeTint*(.6+eyeLuma),irisMask);');
          };
          material.customProgramCacheKey = () => 'LifeLensNativeEyePigment-v1';
        }
        const mesh = new THREE.SkinnedMesh(geometry, material);
        mesh.name = isBody && tier === 'near' ? BODY : source.name + ':' + tier;
        mesh.position.copy(donor.position); mesh.quaternion.copy(donor.quaternion); mesh.scale.copy(donor.scale);
        mesh.bindMode = donor.bindMode; mesh.bind(donor.skeleton, donor.bindMatrix.clone());
        mesh.bindMatrixInverse.copy(donor.bindMatrixInverse);
        mesh.userData.residentProceduralAppearance = true; mesh.userData.residentOwnsGeometry = true;
        mesh.userData.nativeResidentSource = 'MakeHuman CC0';
        mesh.frustumCulled = false; mesh.castShadow = isBody; mesh.receiveShadow = true;
        level.add(mesh);
        // The source already contains the observed sex morphology. Reuse the
        // existing face/build genetics without applying that sex bias twice.
        const phenotype = { ...profile, shoulderScale: 1 + (profile.shoulderScale - (sex === 'male' ? 1.10 : .90)) * .5,
          hipScale: 1 + (profile.hipScale - (sex === 'female' ? 1.14 : .98)) * .5, breastVolume: 0, waistScale: 1 };
        applyBodyProportions(mesh, phenotype, model.matrixWorld.clone().invert(), model);
        if (isBody) {
          const sole = { left: .035, right: .035 };
          const p = mesh.geometry.getAttribute('position'), j = mesh.geometry.getAttribute('skinIndex'), w = mesh.geometry.getAttribute('skinWeight');
          for (const side of ['left', 'right'] as const) {
            const suffix = side === 'left' ? '_l' : '_r'; let low = Infinity;
            for (let v = 0; v < p.count; v++) {
              let weight = 0;
              for (let k = 0; k < 4; k++) if (['foot' + suffix, 'ball' + suffix].includes(donor.skeleton.bones[j.getComponent(v, k)]?.name)) weight += w.getComponent(v, k);
              if (weight >= .4) low = Math.min(low, p.getY(v));
            }
            const foot = donorIndices.get('foot' + suffix);
            if (foot !== undefined && Number.isFinite(low)) {
              const local = donor.bindMatrix.clone().invert().multiply(donor.skeleton.boneInverses[foot].clone().invert());
              sole[side] = THREE.MathUtils.clamp(new THREE.Vector3().setFromMatrixPosition(local).y - low, .014, .095);
            }
          }
          geometry.userData.residentFootSole = sole;
          mesh.geometry.userData.residentFootSole = sole;
          mesh.geometry.userData.anatomyVersion = 'MakeHuman-native-v1';
        }
      });
    }
  } catch (error) { disposeNative(group); console.warn('Native resident binding rejected', error); return false; }
  donor.parent.add(group); model.userData.nativeResidentBody = true;
  model.traverse(object => {
    if (!(object instanceof THREE.Mesh) || object.userData.nativeResidentSource) return;
    if (['SuperHero_Male', 'Eyes', 'Eyebrows'].includes(object.name)) {
      for (const material of Array.isArray(object.material) ? object.material : [object.material]) material.visible = false;
      object.castShadow = false;
    }
  });
  return true;
}

export function setNativeResidentLOD(model: THREE.Object3D, distance: number): void {
  const group = model.getObjectByName(GROUP); if (!group) return;
  const tier = distance < 7 ? 'near' : distance < 18 ? 'mid' : 'far';
  for (const child of group.children) child.visible = child.name === 'NativeResidentLOD:' + tier;
}
