import {mergeConfig} from 'vite';
import base from '../../vite.config.ts';
export default mergeConfig(base,{
 plugins:[{name:'lifelens-native-wildlife-observer-review',transformIndexHtml:{order:'pre',handler:html=>html.replace('</head>','<script type="module" src="/tests/native-wildlife-observer/bridge.mjs"></script></head>')}}],
 build:{outDir:'tests/native-wildlife-observer/preview',emptyOutDir:true},
});
