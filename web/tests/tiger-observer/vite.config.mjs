import {mergeConfig} from 'vite';
import base from '../../vite.config.ts';

// Production React and the unchanged application entry. Only QA camera/measurement
// instrumentation is injected before the real app starts; Core remains its WASM.
export default mergeConfig(base,{
  plugins:[{name:'lifelens-tiger-observer-instrumentation',
    transformIndexHtml:{order:'pre',handler:html=>html.replace('</head>',
      '<script type="module" src="/tests/tiger-observer/bridge.mjs"></script></head>')}}],
  build:{outDir:'tests/tiger-observer/preview',emptyOutDir:true},
});
