import {observerStore} from '../../src/state/observer-store.ts';
import {observerActions} from '../../src/state/observer-actions.ts';
import {WorldScene} from '../../src/render/world-scene.ts';
import {WorldRenderer} from '../../src/render/world-renderer.ts';
const qa=window.__nativeResidentObserver={store:observerStore,actions:observerActions,world:null,renderer:null,view:'front',focusId:null};
const apply=WorldScene.prototype.applyCamera;
WorldScene.prototype.applyCamera=function(state){
 apply.call(this,state);qa.world=this;
 const actor=this.residentLayer.actors.get(qa.focusId);if(!actor)return;
 const p=actor.root.position;const height=actor.root.scale.y;
 this.camera.position.set(p.x+(qa.view==='side'?height*2.5:0),p.y+height*.65,p.z+(qa.view==='side'?0:height*2.5));
 this.camera.lookAt(p.x,p.y+height*.5,p.z);this.camera.updateMatrixWorld();
};
const camera=WorldRenderer.prototype.setCamera;
WorldRenderer.prototype.setCamera=function(state){qa.renderer=this;return camera.call(this,state);};
qa.sample=()=>({runtime:observerStore.getSnapshot().runtime,minute:observerStore.getSnapshot().world.minute,
 actors:[...(qa.world?.residentLayer.actors??[])].map(([id,a])=>({id,sex:a.appearanceFacts.sex,age:a.appearanceFacts.ageYears,
 native:!!a.model.getObjectByName('LifeLensNativeResident'),version:a.model.getObjectByName('LifeLensAuthoredAnatomicalBody')?.geometry.userData.anatomyVersion,
 action:a.active,position:a.root.position.toArray(),coreGrid:[a.appearanceFacts.gridX,a.appearanceFacts.gridY]}))});
qa.capture=async()=>{
 qa.renderer.stop();qa.world.update(.016);await qa.renderer.renderer.compileAsync(qa.world.scene,qa.world.camera);
 qa.renderer.renderer.render(qa.world.scene,qa.world.camera);
 const canvas=qa.renderer.renderer.domElement;if(canvas.id!=='threeWorldCanvas'||qa.renderer.renderer.getContext().isContextLost())throw Error('Invalid actual Observer canvas');
 return canvas.toDataURL('image/jpeg',.9);
};
