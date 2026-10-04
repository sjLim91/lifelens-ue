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
      assert.ok(p.hairStyle >= 0 && p.hairStyle < 4);
      assert.ok(p.garmentMix >= 0.70 && p.garmentMix <= 0.88);
      assert.ok(p.waistHeight01 >= 0.46 && p.waistHeight01 <= 0.56);
      assert.ok(p.widthScale >= 0.8 && p.widthScale <= 1.18);
      assert.ok(p.heightWorldUnits >= 1.46 && p.heightWorldUnits <= 1.86);
      styles.add(p.hairStyle); colors.add(p.garmentColor);
    }
    assert.equal(styles.size, 4); assert.equal(colors.size, 8);
  });
  test('identity appearance remains stable across reloads and ordering', () => {
    const resident = { id: 'saved-guid', ageYears: 32, sex: 'Female' };
    const before = profile(resident); profile({ id: 'another' });
    assert.deepEqual(profile(resident), before);
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
    assert.deepEqual([...mesh.geometry.getAttribute('position').array],[...geometry.getAttribute('position').array]);
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


