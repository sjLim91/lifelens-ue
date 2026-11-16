// Pointer capability also covers phones in landscape and large tablets.
// A narrow desktop window alone is not evidence of a mobile GPU.
export function useMobilePerformanceProfile(): boolean {
  return typeof window !== 'undefined'
    && (window.matchMedia?.('(pointer: coarse)').matches ?? false);
}

export function canvasCompatibilityRequired(userAgent: string, search: string): boolean {
  // Custom tabs can hide their embedding app in the UA. A shareable recovery
  // URL also works there without changing the saved world or render preference.
  if (new URLSearchParams(search).get('safe') === '1') return true;
  return /\bAndroid\b/i.test(userAgent)
    && (/\bwv\b/i.test(userAgent) || /\bChatGPT\b/i.test(userAgent));
}

export function useCanvasCompatibilityProfile(): boolean {
  return typeof window !== 'undefined'
    && canvasCompatibilityRequired(
      window.navigator?.userAgent ?? '', window.location?.search ?? '',
    );
}
