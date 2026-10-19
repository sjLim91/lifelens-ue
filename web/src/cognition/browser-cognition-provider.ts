import {
  parseCognitiveProposal,
  type CognitiveProposalDto,
  type CognitiveRequestDto,
} from './cognitive-contract';
import type { CognitiveProvider } from './cognitive-provider';
import { CognitiveProviderError } from './cognitive-provider';
import { cognitiveSystemPrompt } from './local-cognition-provider';

// Open-source quantized model. One shared browser engine reasons about each
// resident's SEPARATE authoritative Core identity and memory context.
// The first download can be large; WebGPU availability does not guarantee
// enough VRAM. This is not a paid API, cloud language service or second NPC.
export const BUILTIN_RESIDENT_MODEL =
  'Qwen2.5-0.5B-Instruct-q4f16_1-MLC';

interface BrowserChatRequest {
  messages: Array<{ role: 'system' | 'user'; content: string }>;
  temperature: number;
  max_tokens: number;
  response_format: { type: 'json_object' };
}

export interface BrowserChatEngine {
  // Native WebLLM MLCEngine.unload frees the model's GPU pipelines/buffers.
  // Optional for small deterministic fake engines used by the tests.
  unload?(): Promise<void>;
  chat: {
    completions: {
      create(request: BrowserChatRequest): Promise<{
        choices?: Array<{ message?: { content?: unknown } }>;
      }>;
    };
  };
}

export class BrowserCognitionProvider implements CognitiveProvider {
  readonly name = 'resident-browser-webgpu';
  private disposed = false;
  constructor(private readonly engine: BrowserChatEngine) {}

  async dispose(): Promise<void> {
    if (this.disposed) return;
    this.disposed = true;
    await this.engine.unload?.();
  }

  async reason(
    request: CognitiveRequestDto,
    signal: AbortSignal,
  ): Promise<CognitiveProposalDto | null> {
    if (this.disposed || signal.aborted || request.allowedIntents.length === 0) return null;
    let reply: { choices?: Array<{ message?: { content?: unknown } }> };
    try {
      reply = await this.engine.chat.completions.create({
        messages: [
          { role: 'system', content: cognitiveSystemPrompt() },
          { role: 'user', content: JSON.stringify(request) },
        ],
        temperature: 0,
        max_tokens: 180,
        response_format: { type: 'json_object' },
      });
    } catch {
      throw new CognitiveProviderError(
        'http_error', 'on-device WebGPU inference failed');
    }
    if (signal.aborted) return null;
    const raw = reply.choices?.[0]?.message?.content;
    if (typeof raw !== 'string' || raw.length === 0 || raw.length > 8192) {
      throw new CognitiveProviderError(
        'malformed_response', 'on-device model returned no JSON string');
    }
    let json: unknown;
    try { json = JSON.parse(raw); }
    catch {
      throw new CognitiveProviderError(
        'invalid_json', 'on-device model response was not JSON');
    }
    try { return parseCognitiveProposal(json, request); }
    catch {
      // Even local neural inference is never allowed to invent Core truth.
      throw new CognitiveProviderError(
        'invalid_schema', 'on-device proposal violated the Core contract');
    }
  }
}

export function browserWebGPUSupported(): boolean {
  return typeof navigator !== 'undefined'
    && typeof (navigator as Navigator & { gpu?: unknown }).gpu !== 'undefined';
}

async function loadBuiltinWebGPUEngine(modelId: string): Promise<BrowserChatEngine> {
  // Keep heavyweight WebLLM out of initial app JS. The first load downloads
  // model weights and caches them in the browser; no native server is needed.
  const webllm = await import('@mlc-ai/web-llm');
  const engine = await webllm.CreateMLCEngine(modelId);
  return engine as unknown as BrowserChatEngine;
}

export async function createBrowserResidentCognition(
  available: () => boolean = browserWebGPUSupported,
  loadEngine: (modelId: string) => Promise<BrowserChatEngine> =
    loadBuiltinWebGPUEngine,
): Promise<BrowserCognitionProvider | null> {
  if (!available()) return null;
  try {
    const engine = await loadEngine(BUILTIN_RESIDENT_MODEL);
    return new BrowserCognitionProvider(engine);
  } catch {
    // Unsupported WebGPU, insufficient mobile VRAM, model download failure,
    // or asset availability must NOT fabricate AI inference or stop Core.
    return null;
  }
}
