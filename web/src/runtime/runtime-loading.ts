/** Browser transport budgets only; these never control simulation time or rules. */
export interface RuntimeLoadPolicy {
  requestTimeoutMs: number;
  initializationTimeoutMs: number;
}

export const DEFAULT_RUNTIME_LOAD_POLICY: Readonly<RuntimeLoadPolicy> = {
  requestTimeoutMs: 6000,
  initializationTimeoutMs: 12000,
};

export interface RuntimeBytes {
  js: string;
  wasmBuffer: ArrayBuffer;
}

/** The deadline includes reading the response body, not only receiving headers. */
export async function readRuntimeResponse<T>(
  url: string,
  read: (response: Response) => Promise<T>,
  timeoutMs: number,
  fetcher: typeof fetch = fetch,
): Promise<T> {
  const controller = new AbortController();
  let timer: ReturnType<typeof setTimeout> | undefined;
  try {
    return await Promise.race([
      (async () => {
        const response = await fetcher(url, { cache: 'no-store', signal: controller.signal });
        if (!response.ok) {
          // Stop an error response body rather than retaining an unused stream.
          await response.body?.cancel();
          throw new Error(`Core runtime HTTP ${response.status}: ${url}`);
        }
        return read(response);
      })(),
      new Promise<never>((_, reject) => {
        timer = setTimeout(() => {
          reject(new Error(`Core runtime request timed out after ${timeoutMs}ms: ${url}`));
          controller.abort();
        }, timeoutMs);
      }),
    ]);
  } finally {
    clearTimeout(timer);
  }
}

export async function withRuntimeDeadline<T>(
  promise: Promise<T>, timeoutMs: number, label: string,
): Promise<T> {
  let timer: ReturnType<typeof setTimeout> | undefined;
  try {
    return await Promise.race([
      promise,
      new Promise<never>((_, reject) => {
        timer = setTimeout(() => reject(new Error(`${label} timed out after ${timeoutMs}ms`)), timeoutMs);
      }),
    ]);
  } finally {
    clearTimeout(timer);
  }
}

/** Detect HTML fallback pages / truncated artifacts before importing executable JS. */
export function validateRuntimeBytes(payload: RuntimeBytes): RuntimeBytes {
  if (!payload.js.trim() || /^\s*</.test(payload.js)) {
    throw new Error('Core JavaScript response is empty or HTML');
  }
  const bytes = new Uint8Array(payload.wasmBuffer);
  // WebAssembly binary magic and version 1 are a file-format contract, not game rules.
  const header = [0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00];
  if (bytes.length < header.length || header.some((value, index) => bytes[index] !== value)) {
    throw new Error('Core WASM response has an invalid binary header');
  }
  return payload;
}

export interface RuntimeSource {
  name: string;
  load: () => Promise<RuntimeBytes>;
}

/** A source succeeds only after its own JS/WASM pair has initialized successfully. */
export async function connectRuntimeSources<T>(
  sources: readonly RuntimeSource[],
  initialize: (payload: RuntimeBytes) => Promise<T>,
): Promise<T> {
  const failures: string[] = [];
  for (const source of sources) {
    try {
      return await initialize(validateRuntimeBytes(await source.load()));
    } catch (error) {
      failures.push(`${source.name}: ${error instanceof Error ? error.message : String(error)}`);
    }
  }
  throw new Error(`LifeLensCore runtime loading failed. ${failures.join(' | ')}`);
}
