import * as THREE from 'three';
import { ResidentRig, type ResidentJoint } from './resident-rig';
import { residentStandingFallbackClip } from './resident-sleep-motion';
import type { ResidentSemanticMotion } from './resident-semantic-motion';

type Pose = Partial<Record<ResidentJoint, readonly [number, number, number]>>;
interface MotionRecipe { duration: number; gait?: number; crouch?: number; reach?: number; consume?: number; gesture?: number; wash?: number; sit?: boolean; }
const RECIPES: Partial<Record<ResidentSemanticMotion, MotionRecipe>> = {
  run: {duration: .72, gait: .55}, sit: {duration: 3, sit: true},
  idle: {duration: 4}, walk: {duration: 1.1, gait: .32}, carry: {duration: 1.1, gait: .27, reach: .25},
  consume: {duration: 2.4, consume: 1}, wash: {duration: 2, crouch: .35, wash: 1},
  crouch: {duration: 3, crouch: .8}, interact: {duration: 2, reach: .65},
  talk: {duration: 3, gesture: .3}, comfort: {duration: 3, gesture: .15}, reconcile: {duration: 3, gesture: .2},
  teach: {duration: 3, gesture: .45}, care: {duration: 2.8, crouch: .4, reach: .5},
  work: {duration: 2, crouch: .6, reach: .7}, construct: {duration: 2, crouch: .6, reach: .7},
  repair: {duration: 2.5, crouch: .6, reach: .7}, craft: {duration: 2.2, reach: .6},
  gatherWood: {duration: 2.3, reach: .85}, gatherMineral: {duration: 2.5, crouch: .65, reach: .65},
  gatherClay: {duration: 2.5, crouch: .7, reach: .6}, gatherFiber: {duration: 2.4, crouch: .5, reach: .6},
  plant: {duration: 2.5, crouch: .6, reach: .6}, water: {duration: 2.4, reach: .55},
  harvest: {duration: 2.2, crouch: .45, reach: .65}, tend: {duration: 2.5, crouch: .5, reach: .6},
  store: {duration: 2.2, crouch: .3, reach: .7}, retrieve: {duration: 2.2, crouch: .3, reach: .7},
  fuel: {duration: 2.3, crouch: .6, reach: .6}, ignite: {duration: 2.8, crouch: .7, reach: .6},
  loadFurnace: {duration: 2.5, crouch: .45, reach: .6}, collect: {duration: 2.5, crouch: .45, reach: .6},
  experiment: {duration: 3, gesture: .15},
};

/** Original parameterized keys, authored locally. Vendor actions remain preferred.
 * Rig is calibrated to upright first, so bone axes never assume a T-pose or scale.
 * A future skeleton can supply its own standing clip; unsupported joints are skipped. */
export function createResidentAuthoredMotions(model: THREE.Object3D,
  standing = residentStandingFallbackClip()): Map<ResidentSemanticMotion, THREE.AnimationClip> {
  const mixer = new THREE.AnimationMixer(model);
  mixer.clipAction(standing).play(); mixer.update(0); model.updateMatrixWorld(true);
  const rig = new ResidentRig(model), motions = new Map<ResidentSemanticMotion, THREE.AnimationClip>();
  const boneWorld = new THREE.Quaternion(), axis = new THREE.Vector3(), rotation = new THREE.Quaternion(), q = new THREE.Quaternion();
  const axes = new Map<ResidentJoint, THREE.Vector3[]>();
  for (const [joint, object] of rig.joints) {
    object.getWorldQuaternion(boneWorld).invert();
    axes.set(joint, [new THREE.Vector3(1,0,0),new THREE.Vector3(0,1,0),new THREE.Vector3(0,0,1)].map(v=>v.applyQuaternion(boneWorld)));
  }
  for (const [name, recipe] of Object.entries(RECIPES) as [ResidentSemanticMotion, MotionRecipe][]) {
    if ((recipe.gait && (!rig.joints.has('leftThigh') || !rig.joints.has('rightThigh'))) || rig.joints.size < 4) continue;
    const frames = 17, times = Array.from({length:frames},(_,i)=>i*recipe.duration/(frames-1));
    const poses: Pose[] = times.map(t=> {
      const phase = t/recipe.duration*Math.PI*2, wave = Math.sin(phase), pulse = .5-.5*Math.cos(phase);
      const gait = recipe.gait ?? 0, crouch = recipe.crouch ?? 0, reach = recipe.reach ?? 0;
      const p: Pose = { spine: [-.12*crouch+.008*wave,0,0], head:[0,0,.01*wave],
        leftThigh:[-crouch*.65-gait*wave,0,0],rightThigh:[-crouch*.65+gait*wave,0,0],
        leftCalf:[crouch*1.2+gait*Math.max(0,-wave)*1.4,0,0], rightCalf:[crouch*1.2+gait*Math.max(0,wave)*1.4,0,0],
        leftFoot:[-crouch*.45,0,0],rightFoot:[-crouch*.45,0,0],
        leftArm:[gait*wave-reach*.55,0,0],rightArm:[-gait*wave-reach*(.6+.2*pulse),0,0],
        leftForearm:[-.25*reach,0,0],rightForearm:[-.55*reach,0,0] };
      if(recipe.sit){p.leftThigh=[-Math.PI/2,0,0];p.rightThigh=[-Math.PI/2,0,0];p.leftCalf=[Math.PI/2,0,0];p.rightCalf=[Math.PI/2,0,0];p.leftFoot=[0,0,0];p.rightFoot=[0,0,0];}
      if(recipe.consume){p.rightArm=[-.5,0,0];p.rightForearm=[-1.35-.15*pulse,0,0];p.head=[-.035*pulse,0,0];}
      if(recipe.gesture){p.rightArm=[-.2-.25*pulse,0,-recipe.gesture*pulse];p.rightForearm=[-.5-.2*pulse,0,0];}
      if(recipe.wash){p.leftArm=[-.65-.12*wave,0,0];p.rightArm=[-.65+.12*wave,0,0];p.leftForearm=[-.8,0,0];p.rightForearm=[-.8,0,0];}
      return p;
    });
    // Preserve all upright baseline tracks, including fingers/root transforms.
    const tracks = standing.tracks.map(track=> {
      const size = track.getValueSize();
      return new (track.constructor as typeof THREE.KeyframeTrack)(track.name, times, times.flatMap(()=>Array.from(track.values.slice(0,size))));
    });
    for (const [joint, object] of rig.joints) {
      const rest = rig.rest.get(joint)!, basis = axes.get(joint)!;
      const values: number[] = [];
      for(const pose of poses){q.copy(rest);const angles=pose[joint]??[0,0,0];for(let a=0;a<3;a++){axis.copy(basis[a]);rotation.setFromAxisAngle(axis,angles[a]);q.multiply(rotation);}values.push(q.x,q.y,q.z,q.w);}
      const track = new THREE.QuaternionKeyframeTrack(`${object.name}.quaternion`,times,values);
      const old=tracks.findIndex(t=>t.name===track.name);if(old>=0)tracks[old]=track;else tracks.push(track);
    }
    if(recipe.crouch || recipe.sit){
      const pelvis=rig.joints.get('pelvis'),left=rig.joints.get('leftThigh'),foot=rig.joints.get('leftFoot');
      if(pelvis?.parent && left && foot){
        const p0=left.getWorldPosition(new THREE.Vector3()),p1=foot.getWorldPosition(new THREE.Vector3());
        const drop=p0.distanceTo(p1)*(recipe.sit ? .5 : 1-Math.cos(recipe.crouch!*.65));
        const rest=pelvis.position.clone(),origin=pelvis.getWorldPosition(new THREE.Vector3());
        origin.y-=drop;pelvis.parent.worldToLocal(origin);
        const values=times.flatMap(()=>origin.toArray());
        const track=new THREE.VectorKeyframeTrack(`${pelvis.name}.position`,times,values);
        const old=tracks.findIndex(t=>t.name===track.name);if(old>=0)tracks[old]=track;else tracks.push(track);
        pelvis.position.copy(rest);
      }
    }
    motions.set(name,new THREE.AnimationClip(`LifeLens_Authored_${name}`,recipe.duration,tracks));
  }
  mixer.stopAllAction();mixer.uncacheRoot(model);
  return motions;
}
