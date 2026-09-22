import type { TerrainChunk } from '../runtime/core-types';

export function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function mixHex(base: string, target: string, amount: number): string {
  const t = clamp01(amount);
  const a = Number.parseInt(base.slice(1), 16);
  const b = Number.parseInt(target.slice(1), 16);
  const channel = (shift: number): number => Math.round(
    (((a >> shift) & 255) * (1 - t)) + (((b >> shift) & 255) * t),
  );

  return `#${channel(16).toString(16).padStart(2, '0')}${channel(8).toString(16).padStart(2, '0')}${channel(0).toString(16).padStart(2, '0')}`;
}

export function terrainColor(chunk: TerrainChunk): string {
  switch (chunk.waterKind) {
    case 'Ocean': return '#1b4b63';
    case 'Coast': return '#276878';
    case 'Wetland': return '#496e58';
    default: break;
  }

  const elevation = clamp01(chunk.elevation01);
  let base = elevation < 0.34 ? '#294b31'
    : elevation < 0.48 ? '#3b6439'
      : elevation < 0.62 ? '#64794a'
        : elevation < 0.76 ? '#807d5c'
          : '#aaa78f';

  base = mixHex(base, '#1e5a2b', clamp01(chunk.forestCoverage01) * 0.5);
  base = mixHex(base, '#6f8f38', clamp01(chunk.grassCoverage01) * 0.24);
  base = mixHex(base, '#77746d', clamp01(chunk.rockCoverage01) * 0.3);
  base = mixHex(base, '#386b57', clamp01(chunk.wetlandCoverage01) * 0.34);
  return base;
}

export function presentationHash01(
  seed: string,
  x: number,
  y: number,
  index: number,
  salt: string,
): number {
  const input = `${seed}:${x}:${y}:${index}:${salt}`;
  let hash = 2166136261 >>> 0;

  for (let i = 0; i < input.length; i += 1) {
    hash ^= input.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }

  hash ^= hash >>> 16;
  hash = Math.imul(hash, 2246822519) >>> 0;
  hash ^= hash >>> 13;
  return (hash >>> 0) / 4294967295;
}

export function shade(hex: string, factor: number): string {
  const value = Number.parseInt(hex.slice(1), 16);
  const red = Math.max(
    0,
    Math.min(255, Math.round(((value >> 16) & 255) * factor)),
  );
  const green = Math.max(
    0,
    Math.min(255, Math.round(((value >> 8) & 255) * factor)),
  );
  const blue = Math.max(
    0,
    Math.min(255, Math.round((value & 255) * factor)),
  );

  return `rgb(${red} ${green} ${blue})`;
}
