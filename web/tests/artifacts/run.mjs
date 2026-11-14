import assert from 'node:assert/strict';
import * as THREE from 'three';
import { source } from '../procedural-environment/compile.mjs';
const { activeArtifact, createArtifactProp } = await source('render/artifact-presentation.ts');
const { ResidentPropResources } = await source('render/resident-props.ts');
const { formatMemoryTag } = await source('localization/korean.ts');
for (let kind=17;kind<=21;++kind) {
  const translated=formatMemoryTag(`Civilization:Experiment:Artifact:${kind}:18446744073709551615`);
  assert(translated.includes('18446744073709551615'));
  assert(!translated.includes('미등록') && !translated.includes('Civilization'));
}

const artifact = { id: '18446744073709551615', designId: '12345678901234567890', durability: .6,
  parts: [{ material: 'Flint', shape: 1, joint: 0, size: 1 },
    { material: 'Wood', shape: 0, joint: 1, size: 2 },
    { material: 'Fiber', shape: 5, joint: 1, size: 1 }] };
const p = { active: true, kind: 'Civilization', phase: 'Interacting', civilizationIntent: 'Gather', contextActionToken: '999', artifactTool: artifact };
assert.equal(activeArtifact(p), artifact);
for (const bad of [{ active: false }, { phase: 'Moving' }, { phase: 'Idle' }, { kind: 'Social' },
  { civilizationIntent: 'Craft' }, { contextActionToken: '0' }, { contextActionToken: undefined },
  { artifactTool: { ...artifact, durability: 0 } }, { artifactTool: { ...artifact, parts: [] } },
  { artifactTool: { ...artifact, parts: [{ material: 'Flint', shape: 99, size: 1 }] } }])
  assert.equal(activeArtifact({ ...p, ...bad }), null);
const group = createArtifactProp(artifact);
assert.equal(group.children.length, artifact.parts.length);
assert(group.children.every(c => c instanceof THREE.Mesh));
for (const child of group.children) { child.geometry.dispose(); child.material.dispose(); }
const resources = new ResidentPropResources();
const before = resources.geometryCount;
const a = resources.getArtifact(artifact);
assert(a.getAttribute('position').count > 0);
assert.equal(resources.getArtifact(artifact), a);
assert.equal(resources.geometryCount, before + 1);
const variant = { ...artifact, parts: artifact.parts.map(p => ({ ...p, size: 3 })) };
const b = resources.getArtifact(variant);
assert.notEqual(a, b, 'same ID with different structure cannot reuse an unrelated geometry');
for (let i = 0; i < 100; ++i) resources.getArtifact({ ...artifact, designId: String(i) });
assert.equal(resources.geometryCount, before + 64, 'bounded observer geometry cache');
let disposed = 0;
a.addEventListener('dispose', () => disposed++); resources.dispose(); resources.dispose();
assert.equal(disposed, 1);
console.log('PASS artifact presentation: exact Core token, measured structure, distinct variants, bounded cache and disposal');
