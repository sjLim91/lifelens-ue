import assert from 'node:assert/strict';
import { mkdtempSync, readFileSync, writeFileSync, rmSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import * as THREE from 'three';
import ts from 'typescript';
const directory = mkdtempSync(fileURLToPath(new URL('./compiled-', import.meta.url)));
let passed = 0;
function test(name, fn) { fn(); passed++; console.log(`PASS ${name}`); }
try {
  const path = join(directory, 'appearance.mjs');
  writeFileSync(path, ts.transpileModule(readFileSync(new URL('../../src/render/resident-appearance.ts', import.meta.url), 'utf8'), {
    compilerOptions: { module: ts.ModuleKind.ES2022, target: ts.ScriptTarget.ES2022 },
  }).outputText);
  const { createResidentAppearanceProfile: profile, applyResidentMaterialVariant: material } = await import(pathToFileURL(path));
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
  console.log(`${passed} character regression checks passed`);
} finally { rmSync(directory,{recursive:true,force:true}); }
