import type { CivilizationWorldStorage } from '../runtime/core-types';
import { WORLD_PRESENTATION } from './world-presentation-config';

export interface StoredGoodsPile { quantity: number; material?: string; fill: number }

/** Bounded bundles of actual units, never specialization-created inventory. */
export function storedGoodsPiles(storage: CivilizationWorldStorage | undefined): StoredGoodsPile[] {
  if (!storage || !Number.isFinite(storage.totalUnits) || storage.totalUnits <= 0) return [];
  const config = WORLD_PRESENTATION.storage;
  const count = Math.min(config.maxPiles, Math.ceil(storage.totalUnits / config.unitsPerPile));
  const quantity = storage.totalUnits / count;
  const items = (storage.inventory ?? []).map(item => ({ material: item.material, quantity: Number(item.quantity) }))
    .filter(item => Number.isFinite(item.quantity) && item.quantity > 0);
  const knownTotal = items.reduce((sum, item) => sum + item.quantity, 0);
  // Legacy totals remain visible as neutral bundles. Do not invent their type.
  const known = knownTotal === storage.totalUnits;
  let offset = 0;
  const ranges = items.map(item => {
    const start = offset; offset += item.quantity;
    return { start, end: offset, material: item.material };
  });
  return Array.from({ length: count }, (_, index) => {
    const start = index * quantity, end = (index + 1) * quantity;
    const contents = known ? ranges.filter(item => item.end > start && item.start < end) : [];
    return { quantity, fill: Math.min(1, quantity / config.unitsPerPile),
      material: contents.length === 1 ? contents[0].material : undefined };
  });
}
