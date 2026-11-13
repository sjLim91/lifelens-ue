import * as THREE from 'three';
import { ResidentWorldLayer } from '../../src/render/resident-world-layer';
import { nativeResidentTemplatesReady } from '../../src/render/resident-native-body';
import { WildlifeLayer } from '../../src/render/wildlife-layer';
import { WORLD_GRID_CONTRACT } from '../../src/runtime/lifelens-contract';

const canvas = document.querySelector('#review') as HTMLCanvasElement;
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, preserveDrawingBuffer: true });
renderer.setPixelRatio(1); renderer.setSize(innerWidth, innerHeight - 70, false);
renderer.outputColorSpace = THREE.SRGBColorSpace;
const scene = new THREE.Scene(); scene.background = new THREE.Color(0xe6e5df);
scene.add(new THREE.HemisphereLight(0xffffff, 0x4f5149, 2.4));
const light = new THREE.DirectionalLight(0xfff3df, 2.5); light.position.set(4, 7, 5); scene.add(light);
const camera = new THREE.PerspectiveCamera(38, innerWidth / (innerHeight - 70), .05, 300);
const residents = new ResidentWorldLayer(), wildlife = new WildlifeLayer();
scene.add(residents.group, wildlife.group);
const plane = new THREE.Mesh(new THREE.PlaneGeometry(100, 100), new THREE.MeshStandardMaterial({ color: 0xc1c7b6, roughness: 1 }));
plane.rotation.x = -Math.PI / 2; scene.add(plane);
const chunk = { x: 0, y: 0, elevation01: .3, elevationSamples: Array(289).fill(.3), biome: 'TemperateForest',
  moisture01: .5, forestCoverage01: 0, grassCoverage01: .4, rockCoverage01: 0,
  waterKind: 'None', waterAvailability: 0 };
const terrain = (wet = false) => ({ available: true, worldSeed: '874213954', centerChunkX: 0, centerChunkY: 0,
  chunks: [{ ...chunk, ...(wet ? { waterKind: 'Lake', salinity: 'Fresh', waterAvailability: 1,
    elevationSamples: Array(289).fill(.28) } : {}) }], humanTraces: { entries: [] } });
const unit = (id: string, extra = {}) => ({ id, name: '검증용 주민', alive: true, sex: 'Female', ageYears: 30,
  lifeStage: 'Adult', hasPosition: true, gridX: 16, gridY: 16,
  genetics: { heightPotential: .5, buildPotential: .5, faceShape: .5, skinTone: .5 },
  needs: { hunger: .1, thirst: .1, sleep: .1, bladder: .1, hygiene: .1 }, ...extra });
const directive = (extra = {}) => ({ active: true, kind: 'Civilization', phase: 'Interacting',
  hasTargetGrid: true, targetGridX: 16, targetGridY: 16, issuedMinute: 480, contextActionToken: '1', ...extra });
const inventory = { inventory: [{ item: 'RawMaterial', material: 'PlantFood', quantity: 2 },
  { item: 'RawMaterial', material: 'Water', quantity: 2 }] };
const cases: Record<string, { label: string; p?: object; extra?: object; expected?: string; moving?: boolean }> = {
  idle: { label: '대기', expected: 'idle' },
  walk: { label: '실제 관찰 위치 사이 걷기', p: directive({ phase: 'Moving' }), expected: 'walk', moving: true },
  injury: { label: '부상 상태의 이동', p: directive({ phase: 'Moving' }), extra: { health: { injurySeverity: .8, functionalCapacity: .5 } }, expected: 'walk', moving: true },
  sleep: { label: '지면 수면', p: directive({ kind: 'Physical', physicalGoal: 'Sleep' }), expected: 'sleep' },
  eat: { label: '실제 보유 음식 섭취', p: directive({ kind: 'Physical', physicalGoal: 'Eat' }), extra: { civilization: inventory }, expected: 'consume' },
  drink: { label: '실제 보유 물 마시기', p: directive({ kind: 'Physical', physicalGoal: 'Drink' }), extra: { civilization: inventory }, expected: 'consume' },
  toilet: { label: 'Core가 지정한 긴급 용변 위치', p: directive({ kind: 'Physical', physicalGoal: 'UseToilet', emergencyFallback: true }), expected: 'crouch' },
  gather: { label: '나무 채집 작업', p: directive({ civilizationIntent: 'Gather', civilizationMaterial: 'Wood', civilizationResourceNode: 'wood' }), expected: 'gatherWood' },
  mineral: { label: '돌 채집 작업', p: directive({ civilizationIntent: 'Gather', civilizationMaterial: 'Stone', civilizationResourceNode: 'stone' }), expected: 'gatherMineral' },
  craft: { label: 'Core 발행 · 시설 없는 손 제작', p: directive({ civilizationIntent: 'Craft', civilizationTechnique: 'DiggingStick', hasTargetGrid: false }), expected: 'craft' },
  talk: { label: '실제 상대와 대화·시선', p: directive({ kind: 'Social', socialIntent: 'Approach', targetResidentId: 'partner' }), expected: 'talk' },
  comfort: { label: '상대 위로', p: directive({ kind: 'Social', socialIntent: 'Comfort', targetResidentId: 'partner' }), expected: 'comfort' },
  teach: { label: '상대에게 지식 전수', p: directive({ kind: 'KnowledgeTeaching', targetResidentId: 'partner' }), expected: 'teach' },
  care: { label: '아이 돌봄', p: directive({ kind: 'Parenting', parentingAction: 'Feed', targetResidentId: 'dependent' }), expected: 'care' },
};
async function waitFor(fn: () => boolean) {
  for (let i = 0; i < 300; i++) { if (fn()) return; await new Promise(r => setTimeout(r, 20)); }
  throw Error('production assets did not finish loading');
}
function fit(box: THREE.Box3, factor = 1) {
  const center = box.getCenter(new THREE.Vector3()), size = box.getSize(new THREE.Vector3());
  const span = Math.max(size.y * 1.5, size.x / camera.aspect, 1.2) * factor;
  camera.position.set(center.x + span * .3, center.y + span * .18, center.z + span * 1.7);
  camera.lookAt(center); camera.updateMatrixWorld(true);
}
function actorBounds(actor: any) {
  actor.root.updateMatrixWorld(true);
  const box = new THREE.Box3(), point = new THREE.Vector3(); let native = false, clothing = false;
  actor.model.traverse((mesh: THREE.Object3D) => {
    if (!(mesh instanceof THREE.Mesh) || !mesh.userData.nativeResidentSource) return;
    for (let parent: THREE.Object3D | null = mesh; parent; parent = parent.parent) if (!parent.visible) return;
    native = true; clothing ||= !!mesh.geometry.getAttribute('llBasicClothing');
    if (mesh instanceof THREE.SkinnedMesh) mesh.skeleton.update();
    for (let i = 0; i < mesh.geometry.attributes.position.count; i++) {
      mesh.getVertexPosition(i, point); point.applyMatrix4(mesh.matrixWorld);
      if (!point.toArray().every(Number.isFinite)) throw Error('non-finite deformed vertex');
      box.expandByPoint(point);
    }
  });
  if (!native || !clothing) throw Error('actual native skin or basic clothing was not used');
  return box;
}
(window as any).m10Visual = {
  cases: Object.keys(cases),
  async human(name: string) {
    wildlife.group.visible = false; residents.group.visible = true;
    const config = cases[name]; if (!config) throw Error('unknown case');
    document.querySelector('#label')!.textContent = config.label;
    const t = terrain();
    residents.setCivilization({ available: true, resources: ['Wood', 'Stone'].map((material, i) => ({
      id: i ? 'stone' : 'wood', material, gridX: 16, gridY: 16, hasAccessGrid: true,
      accessGridX: 16, accessGridY: 16, quantity: 10, maxQuantity: 10,
    })), facilities: [] } as any);
    const id = (config.extra as any)?.id ?? 'actor';
    const facts = [unit(id, { ...config.extra, presentation: config.p }),
      ...(config.p && 'targetResidentId' in config.p ? [unit((config.p as any).targetResidentId, { gridX: 18, sex: 'Male',
        ...(name === 'care' ? { ageYears: 3, lifeStage: 'Toddler' } : {}) })] : [])];
    residents.setResidents(facts as any, t as any, 0, 0);
    await waitFor(() => (residents as any).ready && nativeResidentTemplatesReady());
    residents.setResidents(facts as any, t as any, 0, 0); residents.rebasePresentation(); residents.setSimulationSpeed(1);
    const actor = (residents as any).actors.get(id); plane.position.y = actor.current.y;
    fit(new THREE.Box3().setFromCenterAndSize(actor.current.clone().add(new THREE.Vector3(0, .9, 0)), new THREE.Vector3(3, 2, 2)));
    for (let i = 0; i < 80; i++) residents.update(1 / 60, camera);
    if (config.moving) {
      residents.setResidents([unit('actor', { ...config.extra, gridX: 22, presentation: config.p })] as any, t as any, 0, 0);
      for (let i = 0; i < 10; i++) residents.update(.02, camera);
    }
    const box = actorBounds(actor); fit(box, name === 'sleep' ? 1.25 : 1);
    if ((config.extra as any)?.lifeReview) { camera.position.set(2.2, actor.current.y + 1.2, 5.8); camera.lookAt(0, actor.current.y + .95, 0); }
    renderer.render(scene, camera);
    return { name, label: config.label, active: actor.active, expected: config.expected,
      native: actor.model.userData.nativeResidentBody === true, minClearance: box.min.y - actor.current.y,
      bounds: box.getSize(new THREE.Vector3()).toArray(), fixture: true };
  },
  async life(sex: string, stage: string, age: number) {
    const config = cases.idle; cases.idle = { ...config, label: `${sex} · ${stage} · ${age}세`,
      extra: { id: `life-${sex}-${stage}`, lifeReview: true, sex, lifeStage: stage, ageYears: age, genetics: { heightPotential: sex === 'Male' ? .8 : .2, buildPotential: .5, faceShape: .4, skinTone: .5 } } };
    const result = await (window as any).m10Visual.human('idle'); cases.idle = config;
    return { ...result, sex, stage, age };
  },
  async animal(species: string, behavior: string) {
    residents.group.visible = false; wildlife.group.visible = true;
    document.querySelector('#label')!.textContent = `${species} · ${behavior} · 실제 생산 rig`;
    const t = terrain(species === 'FreshwaterFish');
    const animal = { id: species, species, sex: 'Female', gridX: 16, gridY: 16, ageDays: 1000, behavior };
    wildlife.setTerrain({ ...t, wildlife: { available: true, animals: [animal] } } as any);
    const y = (wildlife as any).ground.height(0, 0) ?? 0; plane.position.y = y;
    camera.position.set(0, y + 2.4, 5.5); camera.lookAt(0, y + .5, 0);
    wildlife.setSimulationSpeed(1);
    for (let i = 0; i < 30; i++) { wildlife.update(.01, camera); await new Promise(r => setTimeout(r, 20)); }
    const pool = species === 'Tiger' ? (wildlife as any).tigers : (wildlife as any).native;
    await waitFor(() => { wildlife.update(.01, camera); return pool.actors.has(species); });
    if (['Hunt', 'Flee', 'Roam', 'Swim'].includes(behavior)) {
      wildlife.setTerrain({ ...t, wildlife: { available: true, animals: [{ ...animal, gridX: 20 }] } } as any);
      wildlife.update(.02, camera); wildlife.update(.02, camera);
    }
    const animalActor = pool.actors.get(species), animalBox = new THREE.Box3(), vertex = new THREE.Vector3();
    animalActor.root.updateMatrixWorld(true);
    animalActor.root.traverse((object: THREE.Object3D) => {
      if (!(object instanceof THREE.Mesh)) return;
      if (object instanceof THREE.SkinnedMesh) object.skeleton.update();
      for (let i = 0; i < object.geometry.attributes.position.count; i++) {
        object.getVertexPosition(i, vertex); vertex.applyMatrix4(object.matrixWorld); animalBox.expandByPoint(vertex);
      }
    });
    if (!animalBox.isEmpty()) fit(animalBox, 1.15);
    renderer.render(scene, camera);
    if (wildlife.renderedCoreIds.size !== 1) throw Error('Core identity was lost or duplicated');
    const pose = (wildlife as any).animals.get(species);
    return { species, behavior, native: pool.actors.has(species), clip: pool.actors.get(species).current ?? pool.actors.get(species).active, instances: wildlife.renderedCoreIds.size, fixture: true, calls: renderer.info.render.calls };
  },
  dispose() { residents.dispose(); wildlife.dispose(); plane.geometry.dispose(); (plane.material as THREE.Material).dispose(); renderer.dispose(); renderer.forceContextLoss(); },
};
