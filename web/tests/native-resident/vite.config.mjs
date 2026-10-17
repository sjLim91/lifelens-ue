import {mergeConfig} from 'vite';
import base from '../../vite.config.ts';
export default mergeConfig(base,{
 plugins:[{name:'native-resident-observer-review',transformIndexHtml:{order:'pre',handler:html=>html.replace('</head>','<script type="module" src="/tests/native-resident/observer-bridge.mjs"></script></head>')}}],
 build:{outDir:'tests/native-resident/preview',emptyOutDir:true},
});
