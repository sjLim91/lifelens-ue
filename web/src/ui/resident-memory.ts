import type { ResidentMemory } from '../runtime/core-types';
import {
  formatLocationText,
  formatMemoryTag,
  formatMemoryText,
} from '../localization/korean';

export interface VisibleResidentMemory {
  memory: ResidentMemory;
  tags: string[];
  repeatCount: number;
}

function confidence(memory: ResidentMemory): number {
  const value = memory.effectiveConfidence ?? memory.confidence ?? 0;
  return Number.isFinite(Number(value)) ? Number(value) : 0;
}

function recall(memory: ResidentMemory): number {
  const value = memory.recallScore ?? 0;
  return Number.isFinite(Number(value)) ? Number(value) : 0;
}

function visibleTags(memory: ResidentMemory): string[] {
  return Array.from(new Set(
    (memory.tags ?? [])
      .map(formatMemoryTag)
      .filter((tag): tag is string => Boolean(tag)),
  )).sort((a, b) => a.localeCompare(b, 'ko'));
}

function semanticKey(memory: ResidentMemory, tags: string[]): string {
  return [
    memory.who ?? '',
    formatMemoryText(memory.what),
    formatLocationText(memory.where),
    tags.join('|'),
  ].join('\u0001');
}

/**
 * Presentation-only grouping for semantically identical memories.
 * Core memories are never deleted or rewritten; repeated observations remain
 * visible through repeatCount while avoiding three nearly identical rows.
 */
export function summarizeResidentMemories(
  memories: ResidentMemory[] | undefined,
  limit = 3,
): VisibleResidentMemory[] {
  if (!memories?.length || limit <= 0) return [];

  const groups = new Map<string, VisibleResidentMemory>();
  for (const memory of memories) {
    const tags = visibleTags(memory);
    const key = semanticKey(memory, tags);
    const existing = groups.get(key);
    if (!existing) {
      groups.set(key, { memory, tags, repeatCount: 1 });
      continue;
    }

    existing.repeatCount += 1;
    const better =
      confidence(memory) > confidence(existing.memory)
      || (
        confidence(memory) === confidence(existing.memory)
        && recall(memory) > recall(existing.memory)
      );
    if (better) {
      existing.memory = memory;
      existing.tags = tags;
    }
  }

  return Array.from(groups.values()).slice(0, limit);
}
