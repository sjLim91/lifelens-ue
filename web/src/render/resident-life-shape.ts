import * as THREE from 'three';
import type { Resident } from '../runtime/core-types';
import { deriveResidentLifeVisualState } from './resident-life-presentation';
import { WORLD_PRESENTATION } from './world-presentation-config';
const C = WORLD_PRESENTATION.residentLife;
/** Reuses each resident's existing tinted mesh/material. No pregnancy prop. */
export class ResidentLifeShape {
  private readonly belly = { value: 0 };
  private readonly targetScale = new THREE.Vector3();
  private remaining = 0;
  private lean = 0;
  private previousLean = 0;
  private spine?: THREE.Bone;
  private readonly rotation = new THREE.Quaternion();
  private readonly inverse = new THREE.Quaternion();
  private readonly axis = new THREE.Vector3(1,0,0);
  constructor(private readonly root: THREE.Group, private readonly model: THREE.Group) {
    model.traverse(o => {if (o instanceof THREE.Bone && o.name === 'spine_02') this.spine = o;});
  }
  install(): void {
    this.model.updateMatrixWorld(true);
    const referenceInverse=this.model.matrixWorld.clone().invert();
    this.model.traverse(o => {
      if (!(o instanceof THREE.SkinnedMesh)) return;
      const positions = o.geometry.getAttribute('position'), indices=o.geometry.getAttribute('skinIndex'), weights=o.geometry.getAttribute('skinWeight');
      if (!positions || !indices || !weights) return;
      const toModel=referenceInverse.clone().multiply(o.matrixWorld),fromModel=toModel.clone().invert();
      const box=new THREE.Box3();const point=new THREE.Vector3();
      for(let v=0;v<positions.count;v++)box.expandByPoint(point.fromBufferAttribute(positions,v).applyMatrix4(toModel));
      const unitHeight=Math.max(.001,box.max.y-box.min.y);
      if (!o.geometry.getAttribute('lifeTorsoMask')) {
        const mask=new Float32Array(positions.count);

        for(let v=0;v<positions.count;v++) {
          const h=(point.fromBufferAttribute(positions,v).applyMatrix4(toModel).y-box.min.y)/unitHeight;
          if(h<.4 || h>.68)continue;
          for(let i=0;i<4;i++) if (/^(spine_01|spine_02|pelvis)$/.test(o.skeleton.bones[indices.getComponent(v,i)]?.name??'')) mask[v]+=weights.getComponent(v,i);
          mask[v]*=Math.sin(Math.PI*(h-.4)/.28);
        }
        o.geometry.setAttribute('lifeTorsoMask',new THREE.BufferAttribute(mask,1));
      }
      const materials=Array.isArray(o.material)?o.material:[o.material];
      for(const material of materials) {
        if(material.userData.lifeShapeInstalled)continue;
        material.userData.lifeShapeInstalled=true;
        const prior=material.onBeforeCompile;

        material.onBeforeCompile=(shader,renderer)=>{
          prior.call(material,shader,renderer);
          shader.uniforms.lifeBelly=this.belly;shader.uniforms.lifeUnitHeight={value:unitHeight};
          shader.uniforms.lifeToModel={value:toModel};shader.uniforms.lifeFromModel={value:fromModel};
          shader.vertexShader='attribute float lifeTorsoMask;\nuniform float lifeBelly;\nuniform float lifeUnitHeight;\nuniform mat4 lifeToModel;\nuniform mat4 lifeFromModel;\n'+shader.vertexShader;
          shader.vertexShader=shader.vertexShader.replace('#include <begin_vertex>',
            '#include <begin_vertex>\nvec3 lifePoint = (lifeToModel * vec4(transformed, 1.0)).xyz;\nlifePoint.z += lifeBelly * lifeUnitHeight * lifeTorsoMask;\ntransformed = (lifeFromModel * vec4(lifePoint, 1.0)).xyz;');
        };
        material.customProgramCacheKey=()=> 'LifeLensResidentLifeShape-v1';material.needsUpdate=true;
      }
    });
  }
  setFacts(resident: Resident, scale: THREE.Vector3, initial=false): void {
    const life=deriveResidentLifeVisualState(resident);this.belly.value=life.belly;this.lean=life.elderLean;
    if(!this.targetScale.equals(scale)){this.targetScale.copy(scale);this.remaining=initial?0:C.scaleSettleSeconds;}
    if(initial)this.root.scale.copy(scale);
  }
  updateScale(dt:number): void {
    if(dt<=0 || this.remaining<=0)return;
    const step=Math.min(dt,this.remaining);this.root.scale.lerp(this.targetScale,step/this.remaining);this.remaining-=step;
  }
  beforeMotion(): void {
    if(this.spine && this.previousLean){this.inverse.setFromAxisAngle(this.axis,-this.previousLean);this.spine.quaternion.multiply(this.inverse);}
    this.previousLean=0;
  }
  afterMotion(sleeping:boolean,parenting=false): void {
    const lean=sleeping?0:this.lean+(parenting?C.parentingLeanRadians:0);
    if(this.spine && lean){this.rotation.setFromAxisAngle(this.axis,lean);this.spine.quaternion.multiply(this.rotation);this.previousLean=lean;}
  }
}
