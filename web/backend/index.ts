import { router, json, error } from '@appdeploy/sdk';

const RUNTIME_REVISION = 'f29add91394ec1bfd4a7aa44cfc3a6b0232067c1';
const CORE_JS_URL = `https://github.com/sjLim91/lifelens-ue/releases/download/web-runtime-latest/lifelens_core.js?rev=${RUNTIME_REVISION}`;
const CORE_WASM_URL = `https://github.com/sjLim91/lifelens-ue/releases/download/web-runtime-latest/lifelens_core.wasm?rev=${RUNTIME_REVISION}`;

interface RuntimePayload {
  js: string;
  wasmBase64: string;
}

let runtimeCache: RuntimePayload | null = null;

async function loadRuntime(): Promise<RuntimePayload> {
  if (runtimeCache) return runtimeCache;

  const [jsResponse, wasmResponse] = await Promise.all([
    fetch(CORE_JS_URL),
    fetch(CORE_WASM_URL),
  ]);

  if (!jsResponse.ok) throw new Error(`Core JS fetch failed: ${jsResponse.status}`);
  if (!wasmResponse.ok) throw new Error(`Core WASM fetch failed: ${wasmResponse.status}`);

  const [js, wasmBuffer] = await Promise.all([
    jsResponse.text(),
    wasmResponse.arrayBuffer(),
  ]);

  runtimeCache = {
    js,
    wasmBase64: Buffer.from(wasmBuffer).toString('base64'),
  };
  return runtimeCache;
}

export const handler = router({
  'GET /api/_healthcheck': [async () => json({ message: 'Success' })],
  'GET /api/runtime/core': [async () => {
    try {
      return json(await loadRuntime());
    } catch (cause) {
      console.error('LifeLensCore proxy failed', cause);
      return error('LifeLensCore runtime fetch failed', 502);
    }
  }],
});
