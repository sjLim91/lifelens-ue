// Pointer capability also covers phones in landscape and large tablets.
// A narrow desktop window alone is not evidence of a mobile GPU.
export function useMobilePerformanceProfile(): boolean {
  return typeof window !== 'undefined'
    && (window.matchMedia?.('(pointer: coarse)').matches ?? false);
}
