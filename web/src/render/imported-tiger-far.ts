import * as THREE from 'three';
import type { GLTF } from 'three/addons/loaders/GLTFLoader.js';
import type { TigerPose } from './imported-tiger';

type Ground = (x: number, z: number) => number | null;
type Pose = 'idle' | 'rest' | 'forage';
type Batch = {
  mesh: THREE.InstancedMesh; phase: THREE.InstancedBufferAttribute;
  motion: THREE.InstancedBufferAttribute; ids: string[];
};
const LIMIT = 128;

/** Three baked poses of the original source; one bounded draw per used pose. */
export class ImportedTigerFar {
  readonly group = new THREE.Group();
  private readonly batches = new Map<Pose, Batch>();
  private readonly phases = new Map<string, number>();
  private readonly used = new Set<string>();
  private readonly transform = new THREE.Object3D();
  private readonly up = new THREE.Vector3(0, 1, 0);
  private readonly normal = new THREE.Vector3();
  private readonly heading = new THREE.Quaternion();
  private readonly groundRotation = new THREE.Quaternion();
  private count = 0;
  private disposed = false;

  constructor(gltf: GLTF) {
    this.group.name = 'LifeLensTigerFarInstances';
    this.group.userData.coreIds = [] as string[];
    for (const pose of ['idle', 'rest', 'forage'] as const) {
      const name = `LifeLensTigerFar${pose[0].toUpperCase()}${pose.slice(1)}`;
      const source = gltf.scenes.map(scene => scene.getObjectByName(name))
        .find(object => object instanceof THREE.Mesh) as THREE.Mesh | undefined;
      if (!source || !(source.material instanceof THREE.MeshStandardMaterial)
        || !source.geometry.getAttribute('_lifelens_leg')) throw new Error('Native far pose missing');
      const geometry = source.geometry.clone();
      geometry.setAttribute('lifelensLeg', geometry.getAttribute('_lifelens_leg'));
      const phase = new THREE.InstancedBufferAttribute(new Float32Array(LIMIT), 1);
      const motion = new THREE.InstancedBufferAttribute(new Float32Array(LIMIT), 1);
      phase.setUsage(THREE.DynamicDrawUsage); motion.setUsage(THREE.DynamicDrawUsage);
      geometry.setAttribute('lifelensPhase', phase); geometry.setAttribute('lifelensMotion', motion);
      const material = source.material.clone();
      material.onBeforeCompile = shader => {
        shader.vertexShader = `attribute vec4 lifelensLeg;\nattribute float lifelensPhase;\nattribute float lifelensMotion;\n${shader.vertexShader}`;
        shader.vertexShader = shader.vertexShader.replace('#include <begin_vertex>', `
          #include <begin_vertex>
          vec4 tigerWave = sin(vec4(lifelensPhase) + vec4(1.5707963, 4.7123890, 0.0, 3.1415927));
          transformed.z += dot(lifelensLeg, tigerWave) * 0.035 * lifelensMotion;
          transformed.y += dot(lifelensLeg, max(tigerWave, vec4(0.0))) * 0.025 * lifelensMotion;
        `);
      };
      material.customProgramCacheKey = () => 'lifelens-native-tiger-far-v1';
      const mesh = new THREE.InstancedMesh(geometry, material, LIMIT);
      mesh.name = name; mesh.count = 0; mesh.frustumCulled = false;
      mesh.castShadow = true; mesh.receiveShadow = true;
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      const ids: string[] = []; mesh.userData.coreIds = ids;
      this.batches.set(pose, { mesh, phase, motion, ids }); this.group.add(mesh);
    }
  }

  begin(): void {
    this.count = 0; this.used.clear();
    (this.group.userData.coreIds as string[]).length = 0;
    for (const batch of this.batches.values()) { batch.mesh.count = 0; batch.ids.length = 0; }
  }

  draw(pose: TigerPose, dt: number, ground?: Ground): boolean {
    if (this.disposed || this.count >= LIMIT) return false;
    const moving = pose.speed > .005;
    const state: Pose = moving ? 'idle' : pose.behavior === 'Rest' ? 'rest'
      : pose.behavior === 'Forage' || pose.behavior === 'Drink' ? 'forage' : 'idle';
    const batch = this.batches.get(state)!;
    const index = batch.mesh.count;
    const height = ground?.(pose.x, pose.z) ?? pose.y;
    const span = Math.max(.01, .4 * pose.scale);
    const left = ground?.(pose.x - span, pose.z), right = ground?.(pose.x + span, pose.z);
    const back = ground?.(pose.x, pose.z - span), front = ground?.(pose.x, pose.z + span);
    let slopeX = left != null && right != null ? (right - left) / (2 * span) : 0;
    let slopeZ = back != null && front != null ? (front - back) / (2 * span) : 0;
    const limit = Math.min(1, Math.tan(25 * Math.PI / 180) / Math.max(1e-6, Math.hypot(slopeX, slopeZ)));
    slopeX *= limit; slopeZ *= limit;
    this.normal.set(-slopeX, 1, -slopeZ).normalize();
    this.heading.setFromAxisAngle(this.up, pose.heading);
    this.groundRotation.setFromUnitVectors(this.up, this.normal).multiply(this.heading);
    this.transform.position.set(pose.x, height, pose.z);
    this.transform.quaternion.copy(this.groundRotation); this.transform.scale.setScalar(pose.scale);
    this.transform.updateMatrix(); batch.mesh.setMatrixAt(index, this.transform.matrix);
    const cycleDistance = pose.speed / pose.scale > 1.5 ? 2 : pose.behavior === 'Hunt' ? .16 / .70 : .32 / .62;
    const phase = ((this.phases.get(pose.id) ?? pose.phase)
      + Math.max(0, dt) * pose.speed / pose.scale / cycleDistance * Math.PI * 2) % (Math.PI * 2);
    this.phases.set(pose.id, phase); this.used.add(pose.id);
    batch.phase.setX(index, phase); batch.motion.setX(index, moving ? 1 : 0);
    batch.ids.push(pose.id); batch.mesh.count++;
    (this.group.userData.coreIds as string[]).push(pose.id); this.count++;
    return true;
  }

  end(): void {
    for (const id of this.phases.keys()) if (!this.used.has(id)) this.phases.delete(id);
    for (const batch of this.batches.values()) {
      batch.mesh.instanceMatrix.needsUpdate = true;
      batch.phase.needsUpdate = true; batch.motion.needsUpdate = true;
    }
  }

  phaseFor(id: string): number | undefined { const value = this.phases.get(id); return value === undefined ? undefined : value / (Math.PI * 2); }

  get activeCount(): number { return this.count; }

  dispose(): void {
    this.disposed = true; this.count = 0;
    for (const batch of this.batches.values()) {
      batch.mesh.geometry.dispose(); (batch.mesh.material as THREE.Material).dispose();
    }
    this.phases.clear(); this.batches.clear(); this.group.clear(); this.group.removeFromParent();
  }
}
