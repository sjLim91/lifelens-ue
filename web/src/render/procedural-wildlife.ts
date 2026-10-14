import * as THREE from 'three';
import type { WildlifeSpecies } from '../runtime/core-types';
import { createQualityWildlifeSurface, type WildlifeLod } from './wildlife-quality-surface';

// Q1-B/C: unified, self-authored animal meshes replace all sphere/tube
// assemblies. Generation is deterministic and memoized per species, sex and
// LOD, so a mobile observer never recomputes 128 copies of the same animal.
const baseCache=new Map<string, THREE.BufferGeometry>();
export function createWildlifeGeometry(
  species: WildlifeSpecies, sex: 'Male' | 'Female' = 'Female',
  quality: WildlifeLod = 'near',
): THREE.BufferGeometry {
  const key=`${species}/${sex}/${quality}`;
  let geometry=baseCache.get(key);
  if(!geometry){
    geometry=createQualityWildlifeSurface(species,sex,quality);
    baseCache.set(key,geometry);
  }
  // The caller owns the clone; cached base assets are immutable.
  return geometry.clone();
}
