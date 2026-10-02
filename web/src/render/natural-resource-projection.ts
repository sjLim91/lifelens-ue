import type { CivilizationWorldPayload, CivilizationWorldResource, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { WORLD_PRESENTATION } from './world-presentation-config';

export type DressingKind = 'trees' | 'grass' | 'shrubs' | 'rocks';
export interface DressingCandidate {
  x: number;
  z: number;
  scale: number;
  material?: string;
}
const kinds: Record<string, DressingKind> = {
  Wood: 'trees', Fiber: 'grass', PlantFood: 'shrubs', Stone: 'rocks',
  Flint: 'rocks', CopperOre: 'rocks', TinOre: 'rocks', Clay: 'rocks',
};
const profile = WORLD_PRESENTATION.naturalResources;
const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;

function unit(token: string): number {
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < token.length; i++) {
    hash ^= token.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967296;
}
export function resourceQuantityRatio(resource: CivilizationWorldResource): number {
  if (!Number.isFinite(resource.quantity) || !Number.isFinite(resource.maxQuantity)
    || resource.maxQuantity <= 0) return 0;
  return Math.max(0, Math.min(1, resource.quantity / resource.maxQuantity));
}
function relevant(material: string, kind: DressingKind): boolean {
  // Both herbaceous layers clear in harvested food/fiber patches.
  return kinds[material] === kind || ((kind === 'grass' || kind === 'shrubs')
    && (material === 'Fiber' || material === 'PlantFood'));
}

/** Immutable, disposable presentation projection. Depleted nodes still own their
 * area; no history, regrowth clock, or invented resource state is retained. */
export class NaturalResourceProjection {
  readonly signature: string;
  private readonly visible: Set<string>;
  private readonly patches = new Map<string, CivilizationWorldResource[]>();
  private readonly access = new Map<string, { x: number; z: number }[]>();

  constructor(civilization: CivilizationWorldPayload, terrain: TerrainWindow) {
    const visible = new Set(terrain.chunks.map(c => `${c.x}:${c.y}`));
    this.visible = visible;
    const resources = civilization.available === true
      ? (civilization.resources ?? []).filter(r => kinds[r.material]
        && Number.isFinite(r.gridX) && Number.isFinite(r.gridY)
        && visible.has(`${Math.floor(r.gridX / span)}:${Math.floor(r.gridY / span)}`))
        .slice().sort((a, b) => a.id.localeCompare(b.id)) : [];
    // Include zero and all authority inputs. Seed/origin/terrain are separately
    // part of terrainDressingSignature; same stock refresh does no mesh upload.
    this.signature = JSON.stringify(resources.map(r => [r.id, r.gridX, r.gridY,
      r.hasAccessGrid, r.accessGridX, r.accessGridY, r.material, r.quantity,
      r.maxQuantity, r.renewable, r.regenerationPerDay]));
    for (const r of resources) {
      const cx = Math.floor(r.gridX / span), cy = Math.floor(r.gridY / span);
      // Index neighboring chunks too: baseline masking crosses chunk seams.
      for (let y = cy - 1; y <= cy + 1; y++) for (let x = cx - 1; x <= cx + 1; x++) {
        const key = `${x}:${y}`;
        const list = this.patches.get(key) ?? [];
        list.push(r); this.patches.set(key, list);
      }
      const gx = r.hasAccessGrid === true && Number.isFinite(r.accessGridX) ? Number(r.accessGridX) : r.gridX;
      const gy = r.hasAccessGrid === true && Number.isFinite(r.accessGridY) ? Number(r.accessGridY) : r.gridY;
      const ax = gx / span * size, az = gy / span * size;
      const acx = Math.floor(gx / span), acy = Math.floor(gy / span);
      for (let y = acy - 1; y <= acy + 1; y++) for (let x = acx - 1; x <= acx + 1; x++) {
        const key = `${x}:${y}`, list = this.access.get(key) ?? [];
        list.push({ x: ax, z: az }); this.access.set(key, list);
      }
    }
  }

  candidates(kind: DressingKind, chunkX: number, chunkY: number, seed: string,
    baseline: DressingCandidate[], budget: number): DressingCandidate[] {
    const nearby = this.patches.get(`${chunkX}:${chunkY}`) ?? [];
    const owners = nearby.filter(r => kinds[r.material] === kind
      && Math.floor(r.gridX / span) === chunkX && Math.floor(r.gridY / span) === chunkY);
    const radius = profile.patchRadiusWorldUnits;
    const absolute = (p: DressingCandidate) => ({ x: (chunkX + 0.5) * size + p.x, z: (chunkY + 0.5) * size + p.z });
    const center = (r: CivilizationWorldResource) => ({ x: r.gridX / span * size, z: r.gridY / span * size });
    const safeAccess = (p: DressingCandidate) => {
      const a = absolute(p);
      if (!this.visible.has(`${Math.floor(a.x / size)}:${Math.floor(a.z / size)}`)) return false;
      const clearance = kind === 'trees' ? profile.treeAccessClearanceWorldUnits : profile.groundAccessClearanceWorldUnits;
      return !(this.access.get(`${chunkX}:${chunkY}`) ?? []).some(t => Math.hypot(a.x - t.x, a.z - t.z) < clearance);
    };
    const output: DressingCandidate[] = [];
    // Reserve full-stock slots even when depleted. Harvest must never create
    // extra baseline instances or shift the remaining patch positions.
    const reserved = Math.min(budget, owners.length * profile.slotsPerNode);
    owners.forEach((r, ownerIndex) => {
      // Share scarce mobile slots across nodes before allocating a second or
      // third instance. A Stone patch must not consume all ore/clay slots.
      const slots = Math.floor(reserved / owners.length)
        + (ownerIndex < reserved % owners.length ? 1 : 0);
      const ratio = resourceQuantityRatio(r), c = center(r);
      for (let i = 0; i < slots; i++) {
        const progress = Math.max(0, Math.min(1, ratio * slots - i));
        if (progress <= 0) continue;
        const token = `${seed}:${r.id}:${r.material}:${i}`;
        const angle = unit(token) * Math.PI * 2;
        const distance = radius * (profile.innerRadiusRatio + unit(`${token}:radius`) * profile.radiusSpreadRatio);
        const p = { x: c.x - (chunkX + 0.5) * size + Math.cos(angle) * distance,
          z: c.z - (chunkY + 0.5) * size + Math.sin(angle) * distance,
          scale: profile.youngScale + (1 - profile.youngScale) * progress, material: r.material };
        // Nearest same-material node owns overlapping patches, including zero
        // stock. Other real materials may coexist; they are not duplicate props.
        const a = absolute(p);
        let owner = r, nearest = Math.hypot(a.x - c.x, a.z - c.z);
        for (const other of nearby) {
          if (other.material !== r.material) continue;
          const otherCenter = center(other);
          const distance = Math.hypot(a.x - otherCenter.x, a.z - otherCenter.z);
          if (distance < nearest || (distance === nearest && other.id.localeCompare(owner.id) < 0)) {
            owner = other; nearest = distance;
          }
        }
        if (owner === r && safeAccess(p)) output.push(p);
      }
    });
    let baselineSlots = budget - reserved;
    for (const p of baseline) {
      if (baselineSlots <= 0) break;
      const a = absolute(p);
      if (nearby.some(r => relevant(r.material, kind)
        && Math.hypot(a.x - center(r).x, a.z - center(r).z) <= radius) || !safeAccess(p)) continue;
      output.push(p); baselineSlots--;
    }
    return output;
  }
}

/** Reuses the indexed projection on the ordinary 500 ms observer path. */
export class NaturalResourceProjectionCache {
  private resources: CivilizationWorldResource[] | undefined;
  private available: boolean | undefined;
  private terrainKey = '';
  private value: NaturalResourceProjection | null = null;
  get(civilization: CivilizationWorldPayload, terrain: TerrainWindow): NaturalResourceProjection {
    const key = JSON.stringify([terrain.worldSeed, terrain.centerChunkX, terrain.centerChunkY,
      terrain.chunks.map(c => [c.x, c.y])]);
    if (this.value && this.resources === civilization.resources
      && this.available === civilization.available && key === this.terrainKey) return this.value;
    this.resources = civilization.resources; this.available = civilization.available; this.terrainKey = key;
    this.value = new NaturalResourceProjection(civilization, terrain);
    return this.value;
  }
}
