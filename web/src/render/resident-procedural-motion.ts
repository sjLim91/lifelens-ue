import * as THREE from 'three';
import type { Resident } from '../runtime/core-types';
import { ResidentRig, ResidentTwoBoneIK, type ResidentJoint } from './resident-rig';
import type { ResidentGroundSurface } from './resident-grounding';

const unit = (n: number | undefined): number => Number.isFinite(n) ? THREE.MathUtils.clamp(n!,0,1) : 0;
export interface ResidentMotionStyle { fatigue: number; injury: number; fear: number; gesture: number; }
export function residentMotionStyle(resident: Resident): ResidentMotionStyle {
  return { fatigue: unit(resident.needs?.sleep), injury: unit(resident.health?.injurySeverity),
    fear: unit(resident.emotion?.fear), gesture: 1+.2*unit(resident.emotion?.joy)-.25*unit(resident.personality?.introversion) };
}
export interface ResidentMotionFrame {
  dt: number; moving: boolean; sleeping: boolean; detailed: boolean;
  ground: ResidentGroundSurface; handTarget: THREE.Vector3 | null; gazeTarget: THREE.Vector3 | null;
}

/** Post-mixer overlays, restored before the next mixer sample. No Core writes,
 * randomness, per-frame vectors, mesh raycasts or unbounded IK iteration. */
export class ResidentProceduralMotion {
  readonly rig: ResidentRig;
  private readonly saved = new Map<THREE.Object3D, THREE.Quaternion>();
  private readonly ik = new ResidentTwoBoneIK(); private readonly point = new THREE.Vector3();
  private readonly target = new THREE.Vector3(); private readonly local = new THREE.Vector3();
  private readonly rotation = new THREE.Quaternion(); private readonly axis = new THREE.Vector3(1,0,0);
  private style: ResidentMotionStyle = {fatigue:0,injury:0,fear:0,gesture:1};
  private time = 0; private handWeight = 0; private gazeWeight = 0;
  private wasApplied = false;
  private readonly lastHand = new THREE.Vector3();
  private hasLastHand = false;
  private gazeYaw = 0; private gazePitch = 0;
  private readonly worldRotation = new THREE.Quaternion();
  private readonly rootRotation = new THREE.Quaternion();
  private readonly footSole = { left: .035, right: .035 };
  constructor(private readonly root: THREE.Object3D, model: THREE.Object3D) {
    this.rig = new ResidentRig(model);
    for (const object of this.rig.joints.values()) this.saved.set(object,object.quaternion.clone());
    this.setAnatomicalBody(model.getObjectByName('LifeLensAuthoredAnatomicalBody'));
  }
  /** Bind the current authored anatomy after growth or phenotype changes.
   * The skin mesh's real bare sole offsets replace the old fixed shoe clearance.
   */
  setAnatomicalBody(object: THREE.Object3D | null | undefined): void {
    const soles = object instanceof THREE.SkinnedMesh
      ? object.geometry.userData.residentFootSole as { left?: number; right?: number } | undefined
      : undefined;
    for (const side of ['left', 'right'] as const) {
      const value = soles?.[side];
      this.footSole[side] = Number.isFinite(value)
        ? THREE.MathUtils.clamp(value!, .014, .095) : .035;
    }
  }
  setFacts(resident: Resident): void { this.style = residentMotionStyle(resident); }
  beforeMotion(): void {
    if(!this.wasApplied)return;
    for(const [object,q] of this.saved)object.quaternion.copy(q);
    this.wasApplied=false;
  }
  afterMotion(frame: ResidentMotionFrame): void {
    if(frame.sleeping || !frame.detailed){this.handWeight=0;this.gazeWeight=0;this.hasLastHand=false;this.gazeYaw=0;this.gazePitch=0;return;}
    for(const [object,q] of this.saved)q.copy(object.quaternion);
    this.wasApplied=true;
    const dt=Math.max(0,frame.dt);this.time+=dt;
    const blend=1-Math.exp(-dt*8);
    if(frame.handTarget){this.lastHand.copy(frame.handTarget);this.hasLastHand=true;}
    this.handWeight+=((frame.handTarget && !frame.moving ? .9 : 0)-this.handWeight)*blend;
    this.gazeWeight+=((frame.gazeTarget ? 1 : 0)-this.gazeWeight)*blend;
    this.root.updateWorldMatrix(true,true);
    this.rotate('spine',(.06*this.style.fatigue+.04*this.style.fear)*(1-Math.exp(-this.time*3)),0,0);
    if(frame.moving && this.style.injury>0){this.rotate('leftThigh',-.08*this.style.injury*Math.sin(this.time*5),0,0);}
    // Small ordinary gestures vary only from observed emotion/personality.
    if(!frame.moving && !frame.handTarget)this.rotate('rightArm',.012*Math.sin(this.time*1.4)*this.style.gesture,0,0);
    if(frame.gazeTarget){
      this.local.copy(frame.gazeTarget);this.root.worldToLocal(this.local);
      const yaw=THREE.MathUtils.clamp(Math.atan2(this.local.x,this.local.z),-.6,.6);
      const pitch=THREE.MathUtils.clamp(Math.atan2(this.local.y-.85,Math.hypot(this.local.x,this.local.z)),-.25,.25);
      this.gazeYaw+=(yaw-this.gazeYaw)*blend;this.gazePitch+=(-pitch-this.gazePitch)*blend;
      this.rotate('head',this.gazePitch*this.gazeWeight,this.gazeYaw*this.gazeWeight,0);
    }else{this.gazeYaw*=1-blend;this.gazePitch*=1-blend;this.rotate('head',this.gazePitch*this.gazeWeight,this.gazeYaw*this.gazeWeight+.15*this.style.fear*Math.sin(this.time*.8),0);}
    this.root.updateWorldMatrix(true,true);
    for(const side of ['left','right'] as const){
      const upper=this.rig.joints.get(`${side}Thigh`),lower=this.rig.joints.get(`${side}Calf`),foot=this.rig.joints.get(`${side}Foot`);
      if(!upper || !lower || !foot)continue;
      foot.getWorldPosition(this.point);const h=frame.ground.height(this.point.x,this.point.z);
      const height=Math.max(.01,this.root.scale.y),sole=this.footSole[side]*height;
      // Swing feet keep authored clearance. Only a near-ground stance is solved.
      if(h===null || this.point.y-sole-h>.14*height || h-this.point.y>.22*height)continue;
      this.target.copy(this.point);this.target.y=h+sole;
      this.ik.solve(upper,lower,foot,this.target,.8);
      const front=frame.ground.height(this.point.x+Math.sin(this.root.rotation.y)*.08,this.point.z+Math.cos(this.root.rotation.y)*.08);
      if(front!==null)this.rotate(`${side}Foot`,THREE.MathUtils.clamp(Math.atan2(front-h,.08),-.3,.3)*.6,0,0);
    }
    if(this.hasLastHand && this.handWeight>.001){
      const upper=this.rig.joints.get('rightArm'),lower=this.rig.joints.get('rightForearm'),hand=this.rig.joints.get('rightHand');
      if(upper && lower && hand){upper.getWorldPosition(this.point);
        // Outside reach retains the clip; never relocate the actor or target.
        if(this.point.distanceTo(this.lastHand)<this.root.scale.y*.65)this.ik.solve(upper,lower,hand,this.lastHand,this.handWeight);}
    }
  }
  private rotate(joint: ResidentJoint,x:number,y:number,z:number):void {
    const object=this.rig.joints.get(joint);if(!object)return;
    // Express axes in the resident heading frame, even for differently oriented rigs.
    object.getWorldQuaternion(this.worldRotation).invert();
    this.root.getWorldQuaternion(this.rootRotation);
    this.axis.set(1,0,0).applyQuaternion(this.rootRotation).applyQuaternion(this.worldRotation);this.rotation.setFromAxisAngle(this.axis,x);object.quaternion.multiply(this.rotation);
    this.axis.set(0,1,0).applyQuaternion(this.rootRotation).applyQuaternion(this.worldRotation);this.rotation.setFromAxisAngle(this.axis,y);object.quaternion.multiply(this.rotation);
    this.axis.set(0,0,1).applyQuaternion(this.rootRotation).applyQuaternion(this.worldRotation);this.rotation.setFromAxisAngle(this.axis,z);object.quaternion.multiply(this.rotation);
  }
}
