import * as THREE from 'three';
import type { ResidentCivilizationItem } from '../runtime/core-types';
import type { ResidentAppearanceProfile } from './resident-appearance';

// No paid model or fabricated inventory outcome: this only renders Core's
// explicitly observed wornGarment, never raw material or a learned recipe.
export const AUTHORED_GARMENT_GROUP = 'LifeLensResidentAuthoredGarment';

type TorsoFit = {
  bone: THREE.Bone;
  center: THREE.Vector3;
  radiusX: number;
  radiusZ: number;
  height: number;
};

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

function fitTorso(model: THREE.Group): TorsoFit | null {
  model.updateMatrixWorld(true);
  let fit: TorsoFit | null = null;
  model.traverse(object => {
    if (fit || !(object instanceof THREE.SkinnedMesh)) return;
    const positions = object.geometry.getAttribute('position');
    const indices = object.geometry.getAttribute('skinIndex');
    const weights = object.geometry.getAttribute('skinWeight');
    if (!positions || !indices || !weights) return;
    for (const name of ['spine_03', 'spine_02', 'spine_01']) {
      const index = object.skeleton.bones.findIndex(b => b.name.toLowerCase() === name);
      if (index < 0) continue;
      const bone = object.skeleton.bones[index];
      const relative = bone.matrixWorld.clone().invert().multiply(object.matrixWorld);
      const bounds = new THREE.Box3();
      const point = new THREE.Vector3();
      let count = 0;
      for (let i = 0; i < positions.count; i++) {
        let influence = 0;
        for (let component = 0; component < 4; component++) {
          if (indices.getComponent(i, component) === index)
            influence += weights.getComponent(i, component);
        }
        if (influence < .45) continue;
        point.fromBufferAttribute(positions, i).applyMatrix4(relative);
        if (!Number.isFinite(point.x + point.y + point.z)) continue;
        bounds.expandByPoint(point);
        count++;
      }
      if (count < 12 || bounds.isEmpty()) continue;
      const size = bounds.getSize(new THREE.Vector3());
      if ([size.x, size.y, size.z].some(v => v < .025 || v > 1.4))
        continue;
      fit = {
        bone,
        center: bounds.getCenter(new THREE.Vector3()),
        radiusX: size.x * .5,
        radiusZ: size.z * .5,
        height: size.y,
      };
      return;
    }
  });
  return fit;
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

/** Generates a fitted fiber wrap on the actual resident skeleton. Does not
 * mutate Core or synthesize an unsupported clothing event. */
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
  const signature = [
    garment.item, garment.material, garment.quality, garment.durability,
    appearance.seed, appearance.shoulderScale, appearance.hipScale,
    appearance.widthScale, appearance.depthScale,
  ].join(':');
  if (old instanceof THREE.Group && old.userData.garmentSignature === signature)
    return true;
  if (old instanceof THREE.Group) disposeGarment(old);
  const fit = fitTorso(model);
  if (!fit) return false; // Fail closed on incompatible skeletons.
  const group = new THREE.Group();
  group.name = AUTHORED_GARMENT_GROUP;
  group.userData.garmentSignature = signature;

  const color = new THREE.Color(0xb6a27c);
  // Small identity-stable natural-fiber variance, never synthetic dye or fashion.
  color.offsetHSL(0, 0, ((appearance.seed % 101) / 100 - .5) * .07);
  const cloth = new THREE.MeshStandardMaterial({
    color, roughness: 1, metalness: 0, side: THREE.DoubleSide,
  });
  const geometry = new THREE.CylinderGeometry(1.06, 1.18, 1, 16, 4, true);
  const positions = geometry.getAttribute('position');
  const wear = 1 - garment.durability!;
  const individuality = (appearance.seed % 127) / 127;
  for (let i = 0; i < positions.count; i++) {
    const angle = Math.atan2(positions.getZ(i), positions.getX(i));
    const y = positions.getY(i);
    const fold = 1 + .025 * Math.sin(angle * 7 + individuality * Math.PI * 2);
    const frayedHem = y < -.35 ? wear * .065 * Math.sin(angle * 11 + individuality * 4) : 0;
    positions.setXYZ(i, positions.getX(i) * fold, y + frayedHem, positions.getZ(i) * fold);
  }
  positions.needsUpdate = true;
  geometry.computeVertexNormals();
  const wrap = new THREE.Mesh(geometry, cloth);
  wrap.name = 'LifeLensPrimitiveFiberWrap';
  wrap.position.copy(fit.center);
  wrap.position.y -= fit.height * .18;
  wrap.scale.set(Math.max(.04, fit.radiusX * 1.16),
    Math.max(.09, fit.height * 1.48),
    Math.max(.035, fit.radiusZ * 1.16));
  wrap.userData.residentProceduralAppearance = true;
  wrap.userData.residentOwnsGeometry = true;
  wrap.castShadow = true;
  group.add(wrap);

  // The binding cord was physically consumed by the garment recipe.
  const belt = new THREE.Mesh(
    new THREE.TorusGeometry(1, .028 + garment.quality! * .012, 4, 18),
    new THREE.MeshStandardMaterial({ color: 0x897450, roughness: 1 }),
  );
  belt.name = 'LifeLensPrimitiveWrapCord';
  belt.rotation.x = Math.PI / 2;
  belt.position.copy(fit.center);
  belt.position.y -= fit.height * .29;
  belt.scale.set(fit.radiusX * 1.18, fit.radiusZ * 1.18, 1);
  belt.userData.residentProceduralAppearance = true;
  belt.userData.residentOwnsGeometry = true;
  group.add(belt);
  fit.bone.add(group);
  return true;
}
