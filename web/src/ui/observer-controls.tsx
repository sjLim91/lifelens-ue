import { SIMULATION_SPEED_MODES } from '../runtime/lifelens-contract';
import { observerActions } from '../state/observer-actions';
import { useObserverSnapshot } from '../state/use-observer-snapshot';

export const MOBILE_OBSERVER_QUERY = '(max-width: 800px), (max-width: 1024px) and (pointer: coarse)';
export type ObserverModalView = 'menu' | 'residents' | 'resident' | 'seed' | 'events' | 'traces' | 'activity' | 'world' | null;
type IconName = 'observe' | 'people' | 'journal' | 'menu' | 'locate' | 'pause' | 'leaf' | 'trace' | 'activity' | 'settings';
const ICON_PATHS: Record<IconName, string> = {
  observe: 'M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7S2 12 2 12Z M15 12a3 3 0 1 1-6 0 3 3 0 0 1 6 0Z',
  people: 'M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2 M13 7a4 4 0 1 1-8 0 4 4 0 0 1 8 0Z M20 21v-2a4 4 0 0 0-3-3.87 M16 3.13a4 4 0 0 1 0 7.75',
  journal: 'M4 3h12a2 2 0 0 1 2 2v16H6a2 2 0 0 1-2-2V3Z M4 17h14 M8 7h6 M8 11h4',
  menu: 'M4 6h16 M4 12h16 M4 18h16',
  locate: 'M12 2v3 M12 19v3 M2 12h3 M19 12h3 M19 12a7 7 0 1 1-14 0 7 7 0 0 1 14 0Z M14 12a2 2 0 1 1-4 0 2 2 0 0 1 4 0Z',
  pause: 'M8 5v14 M16 5v14',
  leaf: 'M20 3C8 2 2 8 5 15s15 5 15-12Z M5 21 16 10',
  trace: 'M4 21V10l8-7 8 7v11H4Z M9 21v-8h6v8',
  activity: 'M3 12h4l3-8 4 16 3-8h4',
  settings: 'M4 7h16 M4 17h16 M9 4v6 M15 14v6',
};
export function ObserverIcon({ name }: { name: IconName }) {
  return <svg className="observer-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true"><path d={ICON_PATHS[name]} /></svg>;
}

export function MobileObservationControls({ view, onView }: {
  view: ObserverModalView;
  onView: (view: ObserverModalView) => void;
}) {
  const snapshot = useObserverSnapshot();
  const disabled = snapshot.runtime.status !== 'ready' || snapshot.fastForward.status === 'running';
  const items = [
    { view: null, label: '관찰', icon: 'observe', active: view === null },
    { view: 'residents', label: '주민', icon: 'people', active: view === 'residents' || view === 'resident' },
    { view: 'events', label: '기록', icon: 'journal', active: view === 'events' },
    { view: 'menu', label: '메뉴', icon: 'menu', active: view !== null && !['residents', 'resident', 'events'].includes(view) },
  ] as const;
  return <div className="mobile-observation-controls">
    <div className="world-playback-row">
      <div className="world-playback" role="group" aria-label="시뮬레이션 관찰 속도">
        <span className="playback-label">관찰 속도</span>
        {SIMULATION_SPEED_MODES.map(preset => <button key={preset.speed} type="button"
          className={snapshot.simulationSpeed === preset.speed ? 'active' : ''}
          aria-label={preset.speed === 0 ? '일시정지' : preset.label + ' 관찰 속도'}
          aria-pressed={snapshot.simulationSpeed === preset.speed}
          disabled={disabled} onClick={() => observerActions.setSimulationSpeed(preset.speed)}>
          {preset.speed === 0 ? <ObserverIcon name="pause" /> : preset.label}
        </button>)}
      </div>
      <button className="world-recenter" type="button" aria-label="주민 위치로 돌아가기" title="주민 위치로 돌아가기"
        disabled={disabled} onClick={() => observerActions.recenterObserver()}><ObserverIcon name="locate" /></button>
    </div>
    <nav className="mobile-navigation" aria-label="주요 관찰 메뉴">
      {items.map(item => <button key={item.label} type="button" aria-pressed={item.active}
        className={item.active ? 'active' : ''} onClick={() => onView(item.view)}>
        <ObserverIcon name={item.icon} /><span>{item.label}</span>
      </button>)}
    </nav>
  </div>;
}
