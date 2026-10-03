import * as THREE from 'three';
import { GroundDetailLayer } from '../../src/render/ground-detail-layer';
import { FacilityLayer } from '../../src/render/facility-layer';
import { FacilityEmissionLayer } from '../../src/render/facility-emission-layer';
import { HumanTraceLayer } from '../../src/render/human-trace-layer';
import { FootTrafficLayer } from '../../src/render/foot-traffic-layer';
import { SurfaceConsequenceLayer } from '../../src/render/surface-consequence-layer';
import { WeatherLayer } from '../../src/render/weather-layer';
import { createTerrainGeometryBuilder } from '../../src/render/terrain-geometry';
import { applyGroundWetness, snowPresentationCoverage, SurfaceSnowModifier } from '../../src/render/environment-surface-presentation';
import { WORLD_GRID_CONTRACT } from '../../src/runtime/lifelens-contract';
import { ObservedFootTraffic } from '../../src/render/observed-foot-traffic';
import type { TerrainWindow, DynamicEnvironment, HumanTrace } from '../../src/runtime/core-types';

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x9baca7);
scene.add(new THREE.HemisphereLight(0xe7edf5, 0x515b39, 2));
const sunlight = new THREE.DirectionalLight(0xfff1d2, 3); sunlight.position.set(-10, 30, 15); scene.add(sunlight);
const renderer = new THREE.WebGLRenderer({ antialias: true, preserveDrawingBuffer: true });
renderer.setPixelRatio(1); renderer.setSize(window.innerWidth, window.innerHeight);
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
document.body.appendChild(renderer.domElement);
const camera = new THREE.PerspectiveCamera(40, window.innerWidth / window.innerHeight, .1, 200);
camera.position.set(11, 34, 14); camera.lookAt(0, 23.3, 0);
const ground = new GroundDetailLayer(), facilities = new FacilityLayer(), emissions = new FacilityEmissionLayer();
const traces = new HumanTraceLayer(), traffic = new FootTrafficLayer(), surface = new SurfaceConsequenceLayer(), weather = new WeatherLayer();
for (const layer of [ground, facilities, emissions, traces, traffic, surface, weather]) scene.add(layer.group);
const modifier = new SurfaceSnowModifier();
const terrainMaterials: THREE.MeshStandardMaterial[] = [];
const chunks: TerrainWindow['chunks'] = [];
for (let y = -3; y <= 3; y++) for (let x = -3; x <= 3; x++) chunks.push({ x, y,
  elevation01: .48 + (x * x + y * y) * .001, waterKind: 'None', waterAvailability: 0,
  grassCoverage01: .4, shrubCoverage01: .1, forestCoverage01: 0, rockCoverage01: .7 });
const base: TerrainWindow = { available: true, worldSeed: 'environment-42', centerChunkX: 0, centerChunkY: 0, chunks };
const build = createTerrainGeometryBuilder(base);
for (const chunk of chunks) {
  const material = new THREE.MeshStandardMaterial({ vertexColors: true, color: 0xffffff, roughness: .92 });
  modifier.install(material); terrainMaterials.push(material);
  const mesh = new THREE.Mesh(build(chunk), material); mesh.position.set(chunk.x * 8, 0, chunk.y * 8); scene.add(mesh);
}
const fire = (id: string, kind: string, x: number, z: number, lit: boolean): HumanTrace => ({
  id, kind: 'Facility', gridX: 16 + x * 4, gridY: 16 + z * 4, sourceResidentId: 'fixture',
  facilityKind: kind, state: 'Operational', progress01: 1, deliveredMaterialUnits: 10, requiredMaterialUnits: 10,
  active: true, lit, cropPlanted: false, cropGrowth01: 0, cropMoisture01: 0, cropCare01: 0, cropHarvestUnits: 0,
});
// Explicit synthetic observer samples for review only; invoke the existing
// visual-memory class. This fixture never starts or writes a Core world/save.
const history = (traffic as unknown as { history: ObservedFootTraffic }).history;
for (let minute = 0; minute < 500; minute++) {
  const step = minute % 40, x = 6 + (step <= 20 ? step : 40 - step);
  const residents = [12, 13, 14].map(y => ({ id: 'fixture-walker-' + y, alive: true, hasPosition: true, gridX: x, gridY: y }));
  history.observe(residents, base.worldSeed, minute, minute * 100);
}
const labels: Record<string, string> = {
  dry: 'A · 맑고 건조 / 관찰된 흙길', rain: 'B · 비와 젖은 땅 / 낮은 지형 puddle',
  mud: 'C · 비와 반복 보행 / 같은 관찰 흔적의 진흙 표현', snow: 'D · 차가운 눈 / 지면·바위·시설 윗면',
  residue: 'E · 실제 residue DTO / 넓은 오염과 작은 containment', fire: 'F · active + lit FirePit/Furnace / smoke·scorch',
};
function renderCase(name: string) {
  const rainy = name === 'rain' || name === 'mud', snowy = name === 'snow';
  const environment: DynamicEnvironment = { available: true, precipitationType: snowy ? 'Snow' : rainy ? 'Rain' : 'None',
    summary: snowy ? 'Snow' : rainy ? 'Rain' : 'Clear', precipitationIntensity01: snowy || rainy ? 1 : 0,
    airTemperatureC: snowy ? -8 : 18, surfaceWetness01: rainy || snowy ? 1 : 0, windIntensity01: .6 };
  const wet = environment.surfaceWetness01!, snow = snowPresentationCoverage(environment);
  const entries = [fire('facility:shelter', 'Shelter', 2.2, -2.2, false),
    fire('facility:fire', 'FirePit', 2, 1.8, name === 'fire'), fire('facility:furnace', 'Furnace', -2.2, 1.8, name === 'fire')];
  if (name === 'residue') entries.push(
    { id: 'residue:open', kind: 'Residue', gridX: 10, gridY: 17, sourceResidentId: 'fixture', amount: 12, intensity: .9, radiusTiles: 6 },
    { id: 'residue:contained', kind: 'Residue', gridX: 5, gridY: 22, sourceResidentId: 'fixture', amount: 12, intensity: .16, radiusTiles: 1 },
  );
  const terrain = { ...base, humanTraces: { total: entries.length, entries } };
  ground.setTerrain(terrain); ground.setWetness(wet); ground.setSnowCoverage(snow, 4, 4);
  facilities.setTerrain(terrain); facilities.setSurfaceWeather(wet, snow, 4, 4);
  emissions.setTerrain(terrain); emissions.setEnvironment(environment); emissions.setSimulationMinute(500);
  surface.setTerrain(terrain); surface.setWetness(wet, snow);
  traces.setTerrain(terrain);
  traffic.observe([], terrain, 500); traffic.setWetness(wet);
  traffic.group.visible = name === 'dry' || name === 'mud';
  modifier.setState({ snow, originX: 4, originZ: 4 });
  terrainMaterials.forEach(material => applyGroundWetness(material, wet));
  weather.setEnvironment(environment);
  // Stable diagnostic precipitation phase (existing pool, not a second system).
  renderer.render(scene, camera);
  const count = scene.children.length, geometryCount = renderer.info.memory.geometries;
  for (let i = 0; i < 5; i++) { ground.setTerrain({ ...terrain }); facilities.setTerrain({ ...terrain }); surface.setTerrain({ ...terrain }); emissions.setTerrain({ ...terrain }); }
  renderer.render(scene, camera);
  const calls = renderer.info.render.calls;
  const stableGeometry = renderer.info.memory.geometries === geometryCount;
  surface.group.visible = emissions.group.visible = false;
  renderer.render(scene, camera);
  const withoutNewPools = renderer.info.render.calls;
  surface.group.visible = emissions.group.visible = true;
  renderer.render(scene, camera);
  document.querySelector('#caption')!.textContent = labels[name] + ' · read-only fixture';
  return { case: name, drawCalls: calls, withoutNewPools, additionalPoolDrawCalls: calls - withoutNewPools, stableGeometry, triangles: renderer.info.render.triangles,
    points: renderer.info.render.points, puddles: surface.candidateCount, snowCoverage: snow,
    stableChildren: scene.children.length === count, geometries: renderer.info.memory.geometries };
}
Object.assign(window, { environmentReview: { renderCase, ready: true } });
renderCase('dry');
