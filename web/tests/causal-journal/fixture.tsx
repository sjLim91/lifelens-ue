import React from 'react';
import { createRoot } from 'react-dom/client';
import { ResidentCausalTimeline } from '../../src/ui/resident-causal-timeline';
import { residentAI } from '../../src/cognition/live-resident-ai';
import { parseCognitiveCausalJournal } from '../../src/cognition/cognitive-causal-journal';
import { observerStore } from '../../src/state/observer-store';
import '../../src/styles.css';
import '../../src/ui/observer-shell.css';
const path = new URLSearchParams(location.search).get('case') === 'failure' ? 'failure.json' : 'completed.json';
const raw = await fetch('./review/' + path).then(r => r.json());
const journal = parseCognitiveCausalJournal(raw, raw.actor);
if (!journal) throw Error('Actual native Core fixture failed DTO validation');
const resident = { id: journal.actor, name: `주민 ${journal.actor}`, alive: true };
const world = { worldSeed: journal.worldSeed, populationSeed: journal.populationSeed, generationVersion: journal.generationVersion, minute: journal.minute };
observerStore.update({ world, residents: [resident] });
residentAI.attach({ worldOverview: () => world, cognitiveRequest: () => null, submitLocalCognitiveProposal: () => false, cognitiveCausalJournal: () => journal });
createRoot(document.getElementById('root')!).render(<div style={{ maxWidth: 500, margin: '0 auto', padding: 16 }}>
  <p className="hint">실제 Core 생성 기록 · 관찰 UI 검증</p>
  <ResidentCausalTimeline resident={resident} residents={[resident]} />
</div>);
