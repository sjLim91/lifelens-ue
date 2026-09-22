import { useSyncExternalStore } from 'react';
import { observerStore, type ObserverSnapshot } from './observer-store';

export function useObserverSnapshot(): ObserverSnapshot {
  return useSyncExternalStore(
    observerStore.subscribe,
    observerStore.getSnapshot,
    observerStore.getSnapshot,
  );
}
