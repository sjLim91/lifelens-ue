import { writeFileSync } from 'node:fs';
import { source } from './compile.mjs';
const { generateTree } = await source('render/procedural-tree-generator.ts');
const data = ['oak', 'birch', 'pine'].map(species => {
  const g = generateTree(species, `${species}:0`);
  const parts = ['bark', 'foliage'].map(name => ({ name,
    positions: Array.from(g[name].getAttribute('position').array),
    colors: Array.from(g[name].getAttribute('color').array), indices: Array.from(g[name].index.array) }));
  g.bark.dispose(); g.foliage.dispose(); return { species, parts };
});
writeFileSync(new URL('./review/tree-geometry.json', import.meta.url), JSON.stringify(data));
