import assert from 'node:assert/strict';
import { mkdtempSync, readFileSync, writeFileSync, rmSync, mkdirSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import * as THREE from 'three';
import ts from 'typescript';
const directory = mkdtempSync(fileURLToPath(new URL('./compiled-', import.meta.url)));
const compiled = new Set();
function compile(input) {
  const target = join(directory, input.replace(/\.ts$/, '.mjs'));
  if (compiled.has(input)) return target;
  compiled.add(input);
  let code = ts.transpileModule(readFileSync(new URL(`../../src/${input}`, import.meta.url), 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.ES2022, target: ts.ScriptTarget.ES2022 },
  }).outputText;
  code = code.replace(/(from\s+['"])(\.[^'"]+)(['"])/g, (_, start, dependency, end) => {
    compile(join(dirname(input), dependency + '.ts'));
    return start + dependency + '.mjs' + end;
  });
  mkdirSync(dirname(target), {recursive:true});writeFileSync(target, code);return target;
}
const source = input => import(pathToFileURL(compile(input)));
let passed = 0;
function test(name, fn) { fn(); passed++; console.log(`PASS ${name}`); }
try {
  const { createResidentAppearanceProfile: profile, applyResidentMaterialVariant: material } = await source('render/resident-appearance.ts');
  test('1000 identities stay within valid palette, body and hair ranges', () => {
    const styles = new Set(), colors = new Set();
    for (let i = 0; i < 1000; i++) {
      const p = profile({ id: `resident-${i}`, ageYears: 25, sex: i % 2 ? 'Male' : 'Female' });
      for (const color of [p.garmentColor, p.lowerGarmentColor, p.shoeColor, p.hairColor, p.skinColor]) assert.ok(Number.isInteger(color) && color >= 0 && color <= 0xffffff);
      assert.ok(p.hairStyle >= 0 && p.hairStyle < 8);
      assert.ok(p.garmentMix >= 0.70 && p.garmentMix <= 0.88);
      assert.ok(p.waistHeight01 >= 0.46 && p.waistHeight01 <= 0.56);
      assert.ok(p.widthScale >= 0.8 && p.widthScale <= 1.18);
      assert.ok(p.heightWorldUnits >= 1.46 && p.heightWorldUnits <= 1.86);
      styles.add(p.hairStyle); colors.add(p.garmentColor);
    }
    assert.equal(styles.size, 8); assert.equal(colors.size, 8);
  });
  test('identity appearance remains stable across reloads and ordering', () => {
    const resident = { id: 'saved-guid', ageYears: 32, sex: 'Female' };
    const before = profile(resident); profile({ id: 'another' });
    assert.deepEqual(profile(resident), before);
  });
  const { syncResidentAuthoredHair, removeResidentAuthoredHair } =
    await source('render/resident-authored-appearance.ts');
  function headFixture() {
    const model = new THREE.Group();
    const geometry = new THREE.BoxGeometry(.18, .25, .16, 3, 3, 3);
    const count = geometry.getAttribute('position').count;
    const jointIndices = new Uint16Array(count * 4);
    const jointWeights = new Float32Array(count * 4);
    for (let i = 0; i < count; i++) jointWeights[i * 4] = 1;
    geometry.setAttribute('skinIndex', new THREE.Uint16BufferAttribute(jointIndices, 4));
    geometry.setAttribute('skinWeight', new THREE.Float32BufferAttribute(jointWeights, 4));
    const mesh = new THREE.SkinnedMesh(
      geometry, new THREE.MeshStandardMaterial({color:0xe0b394}),
    );
    mesh.name = 'SuperHero_Male';
    const bone = new THREE.Bone(); bone.name = 'Head';
    mesh.add(bone);
    mesh.bind(new THREE.Skeleton([bone]));
    model.add(mesh);
    return { model, mesh, bone };
  }
  test('procedural hairstyles generate actual bone-attached silhouette geometry', () => {
    const {model,mesh,bone}=headFixture();
    const sourceVertexCount=mesh.geometry.getAttribute('position').count;
    const output=[];
    for(let style=0;style<8;style++){
      const p={...profile({id:'head-genetics',sex:'Female'}),hairStyle:style};
      assert.equal(syncResidentAuthoredHair(model,p),true);
      const hair=model.getObjectByName('LifeLensResidentAuthoredHair');
      assert.equal(hair?.parent,bone);
      const count=hair.children.length;
      const vertices=hair.children.reduce((sum,o)=>sum+o.geometry.getAttribute('position').count,0);
      assert.ok(vertices>=85);
      assert.ok(hair.children.every(o=>o.userData.residentOwnsGeometry));
      output.push(count);
    }
    assert.ok(new Set(output).size>=5, 'multiple silhouettes, not 8 palette aliases');
    assert.equal(mesh.geometry.getAttribute('position').count,sourceVertexCount);
    bone.rotation.z=0.23;
    model.updateMatrixWorld(true);
    const headHair=model.getObjectByName('LifeLensResidentAuthoredHair');
    assert.equal(headHair?.parent,bone);
    removeResidentAuthoredHair(model);
    assert.equal(model.getObjectByName('LifeLensResidentAuthoredHair'),undefined);
  });
  test('hair stays private to a skeleton, survives genetics refresh, and resists garment recolor', () => {
    const first=headFixture(),second=headFixture();
    const profile1=profile({id:'sibling-A',sex:'Male',
      genetics:{faceShape:.2,hairPigment:.1,heightPotential:.2}});
    const profile2=profile({id:'sibling-B',sex:'Male',
      genetics:{faceShape:.8,hairPigment:.9,heightPotential:.8}});
    syncResidentAuthoredHair(first.model,profile1);
    syncResidentAuthoredHair(second.model,profile2);
    const hair1=first.model.getObjectByName('LifeLensResidentAuthoredHair');
    const hair2=second.model.getObjectByName('LifeLensResidentAuthoredHair');
    assert.notEqual(hair1,hair2);
    assert.notEqual(hair1.parent,hair2.parent);
    const firstColor=hair1.children[0].material.color.getHex();
    assert.notEqual(firstColor,hair2.children[0].material.color.getHex());
    material(first.model,profile1);
    assert.equal(hair1.children[0].material.color.getHex(),firstColor);
    const last=first.model.getObjectByName('LifeLensResidentAuthoredHair');
    syncResidentAuthoredHair(first.model,profile2);
    assert.notEqual(first.model.getObjectByName('LifeLensResidentAuthoredHair'),last);
    assert.equal(second.model.getObjectByName('LifeLensResidentAuthoredHair'),hair2);
    assert.ok(profile2.jawScale>profile1.jawScale);
    assert.ok(profile2.faceLengthScale<profile1.faceLengthScale);
    removeResidentAuthoredHair(first.model);
    removeResidentAuthoredHair(second.model);
  });
  test('unknown or unskinned rigs cannot receive floating invented hair',()=>{
    const model=new THREE.Group();
    model.add(new THREE.Mesh(new THREE.BoxGeometry(.2,.3,.2)));
    assert.equal(syncResidentAuthoredHair(model,profile({id:'unknown'})),false);
    assert.equal(model.getObjectByName('LifeLensResidentAuthoredHair'),undefined);
  });
  const { syncResidentAuthoredFace, removeResidentAuthoredFace,
    setResidentFaceDetailVisibility } =
    await source('render/resident-authored-appearance.ts');
  function placeRealEyeLandmark(model, forward=1) {
    const eyes = new THREE.Mesh(
      new THREE.BoxGeometry(.08, .025, .005),
      new THREE.MeshStandardMaterial({color:0xffffff}),
    );
    eyes.name='Eyes';
    eyes.position.set(0, .05, forward * .11);
    model.add(eyes);
    return eyes;
  }
  test('A2 face requires an actual identifiable eye landmark; unsupported direction fails closed',()=>{
    const {model}=headFixture();
    const p=profile({id:'no-eyes',sex:'Female'});
    assert.equal(syncResidentAuthoredFace(model,p),false);
    assert.equal(model.getObjectByName('LifeLensResidentAuthoredFace'),undefined);
    placeRealEyeLandmark(model);
    assert.equal(syncResidentAuthoredFace(model,p),true);
    const face=model.getObjectByName('LifeLensResidentAuthoredFace');
    assert.equal(face.children.length,2);
    for(const child of face.children) {
      assert.ok(child.geometry.getAttribute('position').count>4);
      assert.equal(child.userData.residentOwnsGeometry,true);
    }
    removeResidentAuthoredFace(model);
    assert.equal(model.getObjectByName('LifeLensResidentAuthoredFace'),undefined);
  });
  test('A2 independent noses/mouth contours follow parent Head and vary by inherited face shape',()=>{
    const low=headFixture(), high=headFixture();
    placeRealEyeLandmark(low.model);
    placeRealEyeLandmark(high.model);
    const a=profile({id:'same-resident',ageYears:26,
      genetics:{faceShape:0,hairPigment:.3}});
    const b=profile({id:'same-resident',ageYears:26,
      genetics:{faceShape:1,hairPigment:.3}});
    assert.ok(b.noseScale > a.noseScale);
    assert.ok(b.mouthWidthScale > a.mouthWidthScale);
    assert.equal(syncResidentAuthoredFace(low.model,a),true);
    assert.equal(syncResidentAuthoredFace(high.model,b),true);
    const first=low.model.getObjectByName('LifeLensResidentAuthoredFace');
    const second=high.model.getObjectByName('LifeLensResidentAuthoredFace');
    assert.equal(first.parent,low.bone);
    assert.equal(second.parent,high.bone);
    const n1=first.getObjectByName('LifeLensResidentNoseBridge');
    const n2=second.getObjectByName('LifeLensResidentNoseBridge');
    assert.ok(n2.scale.x>n1.scale.x);
    const mouthA=first.getObjectByName('LifeLensResidentMouthContour');
    const mouthB=second.getObjectByName('LifeLensResidentMouthContour');
    assert.notDeepEqual([...mouthA.geometry.getAttribute('position').array],
      [...mouthB.geometry.getAttribute('position').array]);
    setResidentFaceDetailVisibility(low.model,false);
    assert.equal(first.visible,false);
    assert.equal(second.visible,true);
    setResidentFaceDetailVisibility(low.model,true);
    assert.equal(first.visible,true);
    low.bone.rotation.y=.4;low.model.updateMatrixWorld(true);
    assert.equal(first.parent,low.bone);
    removeResidentAuthoredFace(low.model);
    removeResidentAuthoredFace(high.model);
  });
  test('A2 authoritative age transitions grey gradually and change only visual hair length',()=>{
    const adult={id:'age-guid',sex:'Female',ageYears:32,lifeStage:'Adult',
      genetics:{hairPigment:.75,faceShape:.3},lifeCondition:{appearanceAgeFactor:.32}};
    const mid=profile({...adult,ageYears:55,lifeStage:'MiddleAge',
      lifeCondition:{appearanceAgeFactor:.68}});
    const old=profile({...adult,ageYears:81,lifeStage:'Elderly',
      lifeCondition:{appearanceAgeFactor:.95}});
    const young=profile(adult);
    assert.equal(young.seed,mid.seed);
    assert.equal(mid.seed,old.seed);
    assert.notEqual(young.hairColor,old.hairColor);
    const distanceToGrey = (hex) => {
      const c=new THREE.Color(hex),g=new THREE.Color(0xb4b1a6);
      return Math.hypot(c.r-g.r,c.g-g.g,c.b-g.b);
    };
    assert.ok(distanceToGrey(old.hairColor)<distanceToGrey(mid.hairColor));
    assert.ok(distanceToGrey(mid.hairColor)<distanceToGrey(young.hairColor));
    const child=profile({...adult,ageYears:7,lifeStage:'Child'});
    const baby=profile({...adult,ageYears:0,lifeStage:'Baby'});
    assert.ok(baby.hairLengthScale<child.hairLengthScale);
    assert.ok(child.hairLengthScale<young.hairLengthScale);
    const {model}=headFixture();
    const hairSegments=[];
    for(const p of [baby,child,young]) {
      syncResidentAuthoredHair(model,{...p,hairStyle:7});
      const locks=model.getObjectByName('LifeLensResidentAuthoredHair');
      const last=locks.children.at(-1).geometry.getAttribute('position');
      let minY=Infinity;
      for(let i=0;i<last.count;i++) minY=Math.min(minY,last.getY(i));
      hairSegments.push(minY);
    }
    assert.ok(hairSegments[0]>hairSegments[1]);
    assert.ok(hairSegments[1]>hairSegments[2]);
    removeResidentAuthoredHair(model);
  });
  test('A2 old-age creases track actual Core age and refresh without extra bone meshes',()=>{
    const {model}=headFixture();placeRealEyeLandmark(model);
    const younger=profile({id:'aging',ageYears:22,lifeStage:'Adult',
      lifeCondition:{appearanceAgeFactor:.23}});
    const elderly=profile({id:'aging',ageYears:81,lifeStage:'Elderly',
      lifeCondition:{appearanceAgeFactor:.93}});
    syncResidentAuthoredFace(model,younger);
    const previous=model.getObjectByName('LifeLensResidentAuthoredFace');
    assert.equal(previous.children.length,2);
    syncResidentAuthoredFace(model,elderly);
    const aged=model.getObjectByName('LifeLensResidentAuthoredFace');
    assert.equal(aged.children.length,4);
    assert.notEqual(aged,previous);
    assert.equal(previous.parent,null);
    const firstPositions=[...aged.children[0].geometry.getAttribute('position').array];
    syncResidentAuthoredFace(model,elderly);
    const stable=model.getObjectByName('LifeLensResidentAuthoredFace');
    assert.deepEqual([...stable.children[0].geometry.getAttribute('position').array],
      firstPositions);
    removeResidentAuthoredFace(model);
  });
  test('shared jade body gets separate skin upper lower and shoe colors without changing other residents', () => {
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.Float32BufferAttribute([
      0,3,0,
      0,2.0,0,
      0,0.9,0,
      0,0,0,
    ],3));
    geometry.setAttribute('skinIndex', new THREE.Uint16BufferAttribute([
      0,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
    ],4));
    geometry.setAttribute('skinWeight', new THREE.Float32BufferAttribute([
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
      1,0,0,0,
    ],4));
    const shared = new THREE.MeshStandardMaterial({ color: 0x77a88d });
    const mesh = new THREE.SkinnedMesh(geometry, shared); mesh.name = 'SuperHero_Male';
    const head = new THREE.Bone(), pelvis = new THREE.Bone(); head.name = 'Head'; pelvis.name = 'pelvis';
    mesh.bind(new THREE.Skeleton([head, pelvis]));
    const root = new THREE.Group(); root.add(mesh);
    const p = profile({ id: 'skin-test' }); material(root, p);
    assert.notEqual(mesh.geometry, geometry); assert.equal(geometry.getAttribute('color'), undefined);
    assert.notEqual(mesh.material, shared); assert.equal(mesh.material.vertexColors, true);
    const colors = mesh.geometry.getAttribute('color');
    const skin = new THREE.Color(p.skinColor);
    const upper = new THREE.Color(p.garmentColor);
    const lower = new THREE.Color(p.lowerGarmentColor);
    const shoe = new THREE.Color(p.shoeColor);
    assert.ok(Math.abs(colors.getX(0)-skin.r) < 1e-6);
    assert.ok(Math.abs(colors.getX(1)-upper.r) < 1e-6);
    assert.ok(Math.abs(colors.getX(2)-lower.r) < 1e-6);
    assert.ok(Math.abs(colors.getX(3)-shoe.r) < 1e-6);
    assert.equal(mesh.userData.residentOwnsGeometry, true);
  });
  test('scalp follows actual head vertices, with no separate collar geometry', () => {
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.Float32BufferAttribute([0,2,0, 0,1.8,0, 0,1.5,0, 0,0,0],3));
    geometry.setAttribute('skinIndex', new THREE.Uint16BufferAttribute([0,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,0,0],4));
    geometry.setAttribute('skinWeight', new THREE.Float32BufferAttribute([1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0],4));
    const mesh = new THREE.SkinnedMesh(geometry, new THREE.MeshStandardMaterial()); mesh.name='SuperHero_Male';
    const head=new THREE.Bone(), pelvis=new THREE.Bone();head.name='Head';pelvis.name='pelvis';
    mesh.bind(new THREE.Skeleton([head,pelvis]));const root=new THREE.Group();root.add(mesh);
    const p=profile({id:'scalp'});material(root,p);const colors=mesh.geometry.getAttribute('color');
    const hairColor=new THREE.Color(p.hairColor),skinColor=new THREE.Color(p.skinColor);
    assert.ok(Math.abs(colors.getX(0)-hairColor.r)<1e-6);
    assert.ok(Math.abs(colors.getX(2)-skinColor.r)<1e-6);
    assert.equal(root.children.length,1);assert.equal(root.getObjectByName('LifeLensResidentHairVariant'),undefined);
    // A1 deliberately sculpts the cloned HEAD rest mesh. The shared source
    // remains untouched, and reapplying the same phenotype must not drift.
    const sculpted=[...mesh.geometry.getAttribute('position').array];
    assert.notDeepEqual(sculpted,[...geometry.getAttribute('position').array]);
    material(root,p);
    assert.deepEqual([...mesh.geometry.getAttribute('position').array],sculpted);
  });
  test('eyes and eyebrows no longer inherit the garment tint', () => {
    const root = new THREE.Group();
    for (const name of ['Eyes', 'Eyebrows']) { const mesh = new THREE.Mesh(new THREE.BufferGeometry(), new THREE.MeshStandardMaterial({ color: 0x77a88d })); mesh.name = name; root.add(mesh); }
    const p = profile({ id: 'face-test' }); material(root,p);
    assert.equal(root.children[0].material.color.getHex(), 0x302720);
    assert.equal(root.children[1].material.color.getHex(), p.hairColor);
  });
  test('phenotype endpoints affect body and pigment independently of identity', () => {
    const resident = { id: 'phenotype', sex: 'Female', ageYears: 30 };
    const low = profile({ ...resident, genetics: { heightPotential:0, buildPotential:0, skinTone:0, hairPigment:0, eyePigment:0, faceShape:0 } });
    const high = profile({ ...resident, genetics: { heightPotential:1, buildPotential:1, skinTone:1, hairPigment:1, eyePigment:1, faceShape:1 } });
    assert.ok(high.heightWorldUnits > low.heightWorldUnits);
    assert.ok(high.widthScale > low.widthScale && high.depthScale > low.depthScale);
    assert.ok(high.headWidthScale > low.headWidthScale);
    for (const key of ['skinColor','hairColor','eyeColor']) assert.notEqual(high[key],low[key]);
    assert.equal(high.garmentColor,low.garmentColor);
    assert.deepEqual(profile({...resident, genetics:{heightPotential:NaN}}), profile(resident));
    assert.equal(profile({...resident, genetics:{heightPotential:2}}).heightWorldUnits, high.heightWorldUnits);
  });
  test('late phenotype updates do not accumulate deformation or depend on animated bone positions', () => {
    const root = new THREE.Group();
    const geometry = new THREE.BoxGeometry(1,2,.4);
    const count=geometry.getAttribute('position').count;
    geometry.setAttribute('skinIndex',new THREE.Uint16BufferAttribute(new Uint16Array(count*4),4));
    const weights=new Float32Array(count*4);for(let i=0;i<count;i++)weights[i*4]=1;
    geometry.setAttribute('skinWeight',new THREE.Float32BufferAttribute(weights,4));
    const bone=new THREE.Bone();bone.name='Head';
    const mesh=new THREE.SkinnedMesh(geometry,new THREE.MeshStandardMaterial());mesh.name='SuperHero_Male';
    mesh.add(bone);mesh.bind(new THREE.Skeleton([bone]));root.add(mesh);
    const p=profile({id:'late',sex:'Female',genetics:{faceShape:1}});
    material(root,p);const first=[...mesh.geometry.getAttribute('position').array];
    bone.position.x=3;root.updateMatrixWorld(true);material(root,p);
    assert.deepEqual([...mesh.geometry.getAttribute('position').array],first);
    assert.equal(geometry.getAttribute('color'),undefined);
    assert.notEqual(mesh.geometry,geometry);
  });
  const {residentVisibleItems,createResidentProp,ResidentInventoryProps,residentCarriedMaterial,residentPouringWater}=await source('render/resident-props.ts');
  test('tools require actual positive inventory and have bounded stable selection',()=>{
    assert.deepEqual(residentVisibleItems(undefined),[]);
    assert.deepEqual(residentVisibleItems([{item:'DiggingStick',quantity:0},{item:'BronzeAxe',quantity:NaN}]),[]);
    const items=[{item:'SharpFlake',quantity:1},{item:'SimpleContainer',quantity:1},{item:'DiggingStick',quantity:1},{item:'StoneHammer',quantity:2}];
    assert.deepEqual(residentVisibleItems(items),residentVisibleItems([...items].reverse()));
    assert.equal(residentVisibleItems(items).length,3);
    // Worn-out possessions still exist; this is not an equipped/usable tool flag.
    assert.deepEqual(residentVisibleItems([{item:'SharpFlake',quantity:1,durability:0}]),['SharpFlake']);
    for(const item of ['SharpFlake','SimpleContainer','DiggingStick','StoneHammer','StoneCuttingTool','BronzeAxe','BronzePick','Cordage','FuelBundle','RecordTablet']) {
      const model=createResidentProp(item);const size=new THREE.Box3().setFromObject(model).getSize(new THREE.Vector3());
      assert.ok(size.x>0 && size.y>0 && size.z>0 && size.length()<.8,item);
    }
  });
  test('inventory changes hide consumed props while pooled geometry remains stable',()=>{
    const visual=new THREE.Group(), model=new THREE.Group(),pelvis=new THREE.Bone();pelvis.name='pelvis';model.add(pelvis);visual.add(model);
    const props=new ResidentInventoryProps(visual,model);props.setInventory([{item:'DiggingStick',quantity:1}]);
    let disposed=0;props.root.traverse(o=>{if(o.isMesh)o.geometry.addEventListener('dispose',()=>disposed++);});
    pelvis.position.y=.5;props.update();assert.equal(props.root.position.y,.5);
    const slots=[...props.root.children];props.setInventory([]);assert(!props.root.visible);assert.deepEqual(props.root.children,slots);assert.equal(disposed,0);props.dispose();assert.ok(disposed>0);props.dispose();
  });
  test('carry and pouring visuals require existing resources, never the future gather result',()=>{
    const inventory=[{item:'SimpleContainer',quantity:1},{item:'RawMaterial',material:'Water',quantity:1},{item:'RawMaterial',material:'Wood',quantity:2}];
    const moving={active:true,kind:'Civilization',phase:'Moving',facilityAction:'DeliverMaterial',civilizationMaterial:'Wood'};
    assert.equal(residentCarriedMaterial(inventory,moving),'Wood');
    assert.equal(residentCarriedMaterial([],moving),null);
    assert.equal(residentCarriedMaterial(inventory,{...moving,facilityAction:'None',civilizationIntent:'Gather'}),'Wood');
    assert.equal(residentCarriedMaterial(inventory,{...moving,phase:'Interacting'}),'Wood');
    assert.equal(residentCarriedMaterial([{item:'RawMaterial',material:'Wood',quantity:3}],{active:true,kind:'Trade',phase:'Moving'}),'Wood');
    assert.equal(residentCarriedMaterial([],{active:true,kind:'Trade',phase:'Moving'}),null);
    const watering={active:true,kind:'Civilization',phase:'Interacting',facilityKind:'CultivatedPlot',facilityAction:'Water'};
    assert.equal(residentPouringWater(inventory,watering),true);
    assert.equal(residentPouringWater(inventory.slice(1),watering),false);
    assert.equal(residentPouringWater([inventory[0]],watering),false);
    assert.equal(residentPouringWater(inventory,{...watering,phase:'Moving'}),false);
  });
  console.log(`${passed} character regression checks passed`);
} finally { rmSync(directory,{recursive:true,force:true}); }


