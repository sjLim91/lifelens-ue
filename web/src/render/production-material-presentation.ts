import type { ResidentCivilizationItem } from '../runtime/core-types';

/** Visual families for the exact MaterialKind entries in Core Civilization.h.
 * Unknown/future values never acquire a resource or a prop here. */
export const PRODUCTION_MATERIALS = {
  Wood: { shape: 'wood', color: 0x715038 },
  Stone: { shape: 'stone', color: 0x858a89 },
  Flint: { shape: 'stone', color: 0x4d514f },
  Fiber: { shape: 'fiber', color: 0xb69c6b },
  Clay: { shape: 'stone', color: 0xa16e4d },
  Water: { shape: 'water', color: 0x739aa3 },
  PlantFood: { shape: 'food', color: 0x648448 },
  Bone: { shape: 'bone', color: 0xc4bba2 },
  Hide: { shape: 'hide', color: 0x896f50 },
  CopperOre: { shape: 'stone', color: 0x8e684b },
  TinOre: { shape: 'stone', color: 0x858a8c },
  IronOre: { shape: 'stone', color: 0x64544e },
  Charcoal: { shape: 'stone', color: 0x393a34 },
  CopperMetal: { shape: 'metal', color: 0xb67d53 },
  TinMetal: { shape: 'metal', color: 0xaab0b0 },
  Bronze: { shape: 'metal', color: 0xa58648 },
} as const;
export type ProductionMaterial = keyof typeof PRODUCTION_MATERIALS;
export function knownProductionMaterial(value: unknown): value is ProductionMaterial {
  return typeof value === 'string' && Object.prototype.hasOwnProperty.call(PRODUCTION_MATERIALS, value);
}
export function ownedMaterialUnits(inventory: ResidentCivilizationItem[] | undefined, material: string): number {
  if (!knownProductionMaterial(material)) return 0;
  return (inventory ?? []).reduce((sum, stack) => sum + (stack.item === 'RawMaterial'
    && stack.material === material && Number.isFinite(stack.quantity) && (stack.quantity ?? 0) > 0
    ? stack.quantity! : 0), 0);
}
