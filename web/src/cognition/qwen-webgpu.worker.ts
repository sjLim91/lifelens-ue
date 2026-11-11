// The one resident Qwen engine runs off the Three.js/React UI thread.
// No second agent, external inference endpoint, or browser-side Core authority.
import { WebWorkerMLCEngineHandler } from '@mlc-ai/web-llm';

const handler = new WebWorkerMLCEngineHandler();
self.onmessage = (event: MessageEvent) => handler.onmessage(event);
