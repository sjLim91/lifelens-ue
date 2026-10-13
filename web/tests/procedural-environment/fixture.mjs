import * as THREE from 'three';
import { ProceduralTreeAsset } from './review/src/render/procedural-tree-layer.mjs';
import { VegetationLayer } from './review/src/render/vegetation-layer.mjs';
import { GroundDetailLayer } from './review/src/render/ground-detail-layer.mjs';
import { FacilityLayer } from './review/src/render/facility-layer.mjs';
import { createTerrainGeometryBuilder } from './review/src/render/terrain-geometry.mjs';
import { NaturalResourceProjection } from './review/src/render/natural-resource-projection.mjs';
import { WORLD_GRID_CONTRACT as grid } from './review/src/runtime/lifelens-contract.mjs';
import { VegetationLayer as LegacyVegetation } from './review/baseline/render/vegetation-layer.mjs';

const renderer = new THREE.WebGLRenderer({ antialias: true, preserveDrawingBuffer: true });
renderer.setSize(innerWidth, innerHeight); renderer.setPixelRatio(1);
renderer.outputColorSpace = THREE.SRGBColorSpace; renderer.setClearColor(0xb5c3cc);
document.body.append(renderer.domElement);
const scene = new THREE.Scene(); scene.fog = new THREE.Fog(0xb5c3cc, 90, 220);
scene.add(new THREE.HemisphereLight(0xe8f1ff, 0x61513c, 2.0));
const sun = new THREE.DirectionalLight(0xffedd6, 2.7); sun.position.set(-25, 35, 18); scene.add(sun);
const camera = new THREE.PerspectiveCamera(42, innerWidth / innerHeight, .1, 350);
const trees = new VegetationLayer(), legacy = new LegacyVegetation(), ground = new GroundDetailLayer(), facilities = new FacilityLayer();
const terrainGroup = new THREE.Group(), gallery = new THREE.Group();
scene.add(trees.group, legacy.group, ground.group, facilities.group, terrainGroup, gallery);
const treeSpecies = ['oak', 'birch', 'pine'];
const galleryAssets = treeSpecies.map((species, index) => {
  const a = new ProceduralTreeAsset(species, `${species}:0`, 1, false);
  const m = new THREE.Matrix4().makeScale(8, 8, 8); m.setPosition((index - 1) * 8, 0, 0);
  a.setInstances(new Float32Array(m.elements), 1, new Float32Array([.32, .56, .23]));
  a.update(camera); gallery.add(a.group); return a;
});
const chunks = Array.from({ length: 25 }, (_, i) => ({ x: i % 5 - 2, y: Math.floor(i / 5) - 2,
  elevation01: .25, waterKind: 'None', waterAvailability: .5, forestCoverage01: .75,
  grassCoverage01: .65, shrubCoverage01: .25, rockCoverage01: .18, moisture01: .7, temperature01: .5 }));
const w = { available: true, centerChunkX: 0, centerChunkY: 0, worldSeed: 'visual-life-lens', chunks };
const build = createTerrainGeometryBuilder(w);
const terrainMaterial = new THREE.MeshStandardMaterial({ vertexColors: true, roughness: .96 });
for (const chunk of chunks) {
  const mesh = new THREE.Mesh(build(chunk), terrainMaterial); mesh.position.set(chunk.x * grid.worldUnitsPerChunk, -grid.elevationScale * .25, chunk.y * grid.worldUnitsPerChunk);
  terrainGroup.add(mesh);
}
// Same rebased ground height for every production layer.
for (const group of [trees.group, legacy.group, ground.group, facilities.group]) group.position.y = -grid.elevationScale * .25;
const galleryGround = new THREE.Mesh(new THREE.PlaneGeometry(65, 60), new THREE.MeshStandardMaterial({ color: 0x626f47, roughness: 1 }));
galleryGround.rotation.x = -Math.PI / 2; galleryGround.position.y = -.035; gallery.add(galleryGround);
function environment(season = 'Summer') { return { available: true, simulationMinute: 512,
  windIntensity01: .35, calendar: { season }, precipitationType: season === 'Winter' ? 'Snow' : 'None',
  precipitationIntensity01: season === 'Winter' ? .7 : 0, airTemperatureC: season === 'Winter' ? -4 : 20 }; }
function activeResourceBudget() {
  const geometries = new Set(), textures = new Set();
  let geometryBytes = 0, instanceBytes = 0, textureRgbaBytes = 0;
  scene.traverseVisible(object => {
    if (!object.isMesh || object.isInstancedMesh && object.count === 0) return;
    if (!geometries.has(object.geometry)) {
      geometries.add(object.geometry);
      for (const attribute of Object.values(object.geometry.attributes)) geometryBytes += attribute.array.byteLength;
      geometryBytes += object.geometry.index?.array.byteLength ?? 0;
    }
    if (object.isInstancedMesh) instanceBytes += object.instanceMatrix.array.byteLength + (object.instanceColor?.array.byteLength ?? 0);
    for (const material of [object.material].flat()) for (const value of Object.values(material)) {
      if (value?.isTexture && !textures.has(value)) {
        textures.add(value); textureRgbaBytes += (value.image?.width ?? 0) * (value.image?.height ?? 0) * 4;
      }
    }
  });
  return { activeGeometryBytes: geometryBytes, activeInstanceBytes: instanceBytes,
    activeTextureRgbaBytes: textureRgbaBytes };
}
async function renderCase(name) {
  trees.group.visible = legacy.group.visible = ground.group.visible = facilities.group.visible = terrainGroup.visible = gallery.visible = false;
  const e = environment(name === 'autumn' ? 'Autumn' : name === 'winter' ? 'Winter' : 'Summer');
  if (name === 'gallery') {
    camera.position.set(14, 9, 30); camera.lookAt(0, 3.8, 0); gallery.visible = true;
    for (const a of galleryAssets) { a.weather.setEnvironment(e); a.update(camera); }
  } else {
    camera.position.set(27, 19, 34); camera.lookAt(0, 2, 0);
    terrainGroup.visible = ground.group.visible = true;
    const count = name === 'harvest' ? 0 : 10;
    const resources = [{ id: 'wood', material: 'Wood', gridX: grid.gridCellsPerChunk / 2,
      gridY: grid.gridCellsPerChunk / 2, quantity: count, maxQuantity: 10, renewable: true, regenerationPerDay: 1 }];
    const p = new NaturalResourceProjection({ available: true, resources }, w);
    trees.setTerrain(w, p); ground.setTerrain(w, p); trees.setEnvironment(e); ground.setEnvironment(e);
    if (name === 'baseline') {
      legacy.setTerrain(w, p); legacy.group.visible = true;
      await Promise.all(legacy.actualTrees.map(a => new Promise((resolve, reject) => {
        const start = performance.now(); const check = () => a.isReady ? resolve() : performance.now() - start > 15000 ? reject(new Error('legacy GLB timed out')) : setTimeout(check, 30); check();
      })));
    } else {
      trees.group.visible = true; trees.update(camera);
    }
    if (['construction', 'operational', 'ruin'].includes(name)) {
      trees.group.visible = false; facilities.group.visible = true;
      const entries = ['Shelter', 'WorkSurface', 'SleepingPlace', 'PrimitiveStorage', 'CultivatedPlot', 'FirePit', 'Furnace'].map((facilityKind, i) => ({
        id: `facility:${i + 1}`, kind: 'Facility', facilityKind, gridX: grid.gridCellsPerChunk / 2 + (i % 4 - 1.5) * 12,
        gridY: grid.gridCellsPerChunk / 2 + Math.floor(i / 4) * 16, state: name === 'ruin' ? 'Ruined' : name === 'operational' ? 'Operational' : 'UnderConstruction',
        progress01: name === 'construction' ? .52 : 1, deliveredMaterialUnits: 10, requiredMaterialUnits: 10,
        cropPlanted: true, cropGrowth01: .8, cropMoisture01: .6, cropCare01: .7, cropHarvestUnits: 0,
      }));
      facilities.setTerrain({ ...w, humanTraces: { total: entries.length, entries } });
    }
  }
  camera.updateMatrixWorld(); renderer.compile(scene, camera); renderer.render(scene, camera);
  const samples = [];
  const gl = renderer.getContext(), pixel = new Uint8Array(4);
  for (let i = 0; i < 5; i++) {
    const start = performance.now(); renderer.render(scene, camera);
    // WebGL finish alone may only flush the browser's command transport.
    // Readback forces completion before recording a software-frame duration.
    gl.readPixels(0, 0, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, pixel);
    samples.push(performance.now() - start);
  }
  document.getElementById('label').textContent = `${name} · ${innerWidth}×${innerHeight}`;
  return { name, calls: renderer.info.render.calls, triangles: renderer.info.render.triangles,
    ...activeResourceBudget(),
    geometries: renderer.info.memory.geometries, textures: renderer.info.memory.textures,
    frameMedianMs: samples.sort((a, b) => a - b)[Math.floor(samples.length / 2)], trees: trees.instanceCount };
}
window.proceduralReview = { renderCase, ready: true };
await renderCase('gallery');
