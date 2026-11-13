import { CharacterLayer } from '../../src/character-layer';

const wait = (ms: number) => new Promise(r => setTimeout(r, ms));
const check = (condition: boolean, message: string) => { if (!condition) throw Error(message); };
(window as any).m10Lifecycle = async () => {
  const results = [];
  for (let cycle = 0; cycle < 3; cycle++) {
    const canvas = document.createElement('canvas'); document.body.appendChild(canvas);
    let readyCalls = 0;
    const layer = new CharacterLayer(canvas, () => readyCalls++), internals = layer as any;
    layer.resize(400, 280, 1); layer.setSimulationSpeed(0);
    for (let i = 0; i < 250 && !layer.ready; i++) await wait(20);
    check(layer.ready, 'actual local character assets must load');
    const placement = (id: string) => ({ id, name: '표시 검증', sex: 'Female' as const, activityLabel: 'Idle', x: 200, y: 200, index: 0, heightPx: 120 });
    layer.setResidents([placement('initial')]); await wait(80);
    const first = internals.actors.get('initial'); const frozen = first.mixer.time;
    await wait(100); check(first.mixer.time === frozen, 'paused legacy skin must freeze');
    check(internals.animationFrame === null, 'paused static view must not keep a RAF loop');
    layer.setSimulationSpeed(4); await wait(100); check(first.mixer.time > frozen, 'real clip must resume at playback speed');
    layer.setOccluded(true); const hiddenTime = first.mixer.time;
    await wait(100); check(first.mixer.time === hiddenTime && internals.animationFrame === null, 'concealed legacy skin must stop');
    layer.setSimulationSpeed(0); layer.setOccluded(false);
    const originalNow = performance.now.bind(performance); let clock = originalNow();
    Object.defineProperty(performance, 'now', { configurable: true, value: () => clock });
    try {
      for (let n = 0; n < 45; n++) { clock += 20000; layer.setResidents([placement(`resident-${n}`)]); }
    } finally { Object.defineProperty(performance, 'now', { configurable: true, value: originalNow }); }
    check(internals.actors.size <= 33, 'inactive legacy skeletons must stay bounded through long sessions');
    const cached = internals.actors.size;
    layer.clearResidents(); check(internals.actors.size === 0, 'new world cannot retain old actors');
    layer.dispose(); layer.dispose(); await wait(80);
    check(internals.actors.size === 0 && internals.animationFrame === null, 'dispose must release actors and RAF');
    check(internals.renderer.getContext().isContextLost(), 'dispose must release the actual WebGL context');
    results.push({ cycle, cached, readyCalls, pause: true, occlusion: true, disposed: true }); canvas.remove();
  }
  const canvas = document.createElement('canvas'); document.body.appendChild(canvas); let lateReady = 0;
  const late = new CharacterLayer(canvas, () => lateReady++); late.dispose(); await wait(500);
  check(lateReady === 0 && (late as any).actors.size === 0, 'late asset load cannot revive an unmounted renderer');
  canvas.remove(); return { results, lateLoadResurrection: false, realWebGL: true };
};
