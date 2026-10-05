import { useState, useSyncExternalStore } from 'react';
import type { Resident } from '../runtime/core-types';
import { shadowObserver, type ShadowObserver, type ShadowRecord } from '../cognition/shadow-observer';
import {
  KOREAN_SHADOW_LABELS as L, formatCognitiveIntent, formatCognitionState,
  formatCognitiveContext, formatMemoryText, formatBelief, formatResidentName,
} from '../localization/korean';
import { residentStatusText } from './resident-status';

function numericSummary(values: Record<string, number>): string {
  return Object.entries(values).sort((a, b) => Math.abs(b[1]) - Math.abs(a[1])).slice(0, 4)
    .map(([key, value]) => `${formatCognitiveContext(key)} ${value.toFixed(2)}`).join(' · ') || L.none;
}
function residentName(residents: Resident[], id: string): string {
  const resident = residents.find((entry) => entry.id === id);
  return resident ? formatResidentName(resident.name) : L.unavailable;
}
function RecordDetails({ record, residents }: { record: ShadowRecord; residents: Resident[] }) {
  const { context, outcome } = record;
  const proposal = outcome.proposal;
  return <div className="cognitive-shadow-record">
    <dl>
      <dt>{L.resident}</dt><dd>{formatResidentName(record.name)}</dd>
      <dt>{L.time}</dt><dd>{record.minute} {L.minute}</dd>
      <dt>{L.sampledAction}</dt><dd>{residentStatusText({ id: record.actor, name: record.name, ...record.actual }, residents)}</dd>
      <dt>{L.intent}</dt><dd>{proposal ? formatCognitiveIntent(proposal.intent) : L.none}</dd>
      <dt>{L.target}</dt><dd>{proposal?.targetResident ? residentName(residents, proposal.targetResident) : L.none}</dd>
      <dt>{L.rationale}</dt><dd>{proposal && /[가-힣]/u.test(proposal.rationale) ? proposal.rationale : L.rationaleUnavailable}</dd>
      <dt>{L.validation}</dt><dd>{formatCognitionState(outcome.status)}</dd>
      <dt>{L.reason}</dt><dd>{formatCognitionState(outcome.reason)}</dd>
      <dt>{L.provider}</dt><dd>{record.provider}</dd>
    </dl>
    <p className="hint">{L.validationNote}</p>
    <details><summary>{L.context}</summary><dl>
      <dt>{L.personality}</dt><dd>{numericSummary(context.personality)}</dd>
      <dt>{L.emotion}</dt><dd>{numericSummary(context.emotion)}</dd>
      <dt>{L.needs}</dt><dd>{numericSummary(context.needs)}</dd>
      <dt>{L.memory}</dt><dd>{context.memories.slice(0, 3).map((memory) => formatMemoryText(memory.what)).join(' · ') || L.none}</dd>
      <dt>{L.belief}</dt><dd>{context.beliefs.slice(0, 3).map((belief) => formatBelief(belief.proposition)).join(' · ') || L.none}</dd>
      <dt>{L.relationship}</dt><dd>{context.relationships.slice(0, 3).map((relation) =>
        `${residentName(residents, relation.target)} · ${L.trust} ${relation.trust.toFixed(2)} · ${L.affection} ${relation.affection.toFixed(2)} · ${L.conflict} ${relation.conflict.toFixed(2)}`,
      ).join(' / ') || L.none}</dd>
    </dl></details>
  </div>;
}
export function CognitiveShadowPanel({ resident, residents, controller = shadowObserver }: {
  resident: Resident; residents: Resident[]; controller?: ShadowObserver;
}) {
  const state = useSyncExternalStore(controller.subscribe, controller.getSnapshot, controller.getSnapshot);
  const [endpoint, setEndpoint] = useState('http://127.0.0.1:8080');
  const [model, setModel] = useState('');
  const records = state.records.filter((record) => record.actor === resident.id);
  // Keep the most recent validated proposal visible even after a provider failure.
  const latest = records.find((record) => record.outcome.status === 'validated') ?? records[0];
  return <details className="cognitive-shadow-panel">
    <summary>{L.title}</summary>
    <p className="hint">{L.description}</p>
    <form onSubmit={(event) => {
      event.preventDefault();
      if (controller.configure(endpoint, model)) controller.setEnabled(true);
    }}>
      <label>{L.endpoint}<input type="url" value={endpoint} disabled={state.enabled}
        maxLength={300} onChange={(event) => setEndpoint(event.target.value)} /></label>
      <label>{L.model}<input value={model} maxLength={200} disabled={state.enabled}
        onChange={(event) => setModel(event.target.value)} /></label>
      {state.enabled
        ? <button type="button" onClick={() => controller.setEnabled(false)}>{L.disable}</button>
        : <button type="submit">{L.enable}</button>}
    </form>
    <label className="cognitive-shadow-auto"><input type="checkbox" checked={state.automatic}
      onChange={(event) => controller.setAutomatic(event.target.checked)} />{L.automatic}</label>
    <button type="button" disabled={!state.enabled || state.suspended || !!state.pendingActor || resident.alive !== true}
      onClick={() => { void controller.sample(resident.id); }}>{L.sample}</button>
    <p role="status">{formatCognitionState(state.status)}</p>
    <p className="hint">{L.budget}</p>
    <p>{L.currentAction}: <b>{residentStatusText(resident, residents)}</b></p>
    {records[0] && records[0] !== latest && <p role="status">
      {L.reason}: {formatCognitionState(records[0].outcome.reason)}
    </p>}
    {latest ? <RecordDetails record={latest} residents={residents} /> : <p>{L.empty}</p>}
    {records.length > 1 && <details><summary>{L.history}</summary>
      {records.filter((record) => record !== latest).map((record) =>
        <RecordDetails key={record.id} record={record} residents={residents} />)}
    </details>}
  </details>;
}
