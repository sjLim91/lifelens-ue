import type { Resident } from '../runtime/core-types';
import './resident-needs.css';

const NEED_LABELS = [
  ['hunger', '배고픔'],
  ['thirst', '갈증'],
  ['sleep', '수면 필요'],
  ['bladder', '용변 필요'],
  ['hygiene', '위생 필요'],
] as const;

/** Read-only presentation: no urgency bands or simulation decisions here. */
export function ResidentNeeds({ needs }: { needs: Resident['needs'] }) {
  return (
    <div className="resident-needs">
      <div className="resident-needs-caption">
        <strong>생활 욕구</strong>
        <span>높을수록 필요가 큼</span>
      </div>
      <div className="resident-needs-rows">
        {NEED_LABELS.map(([key, label]) => {
          const value = needs?.[key];
          const known = typeof value === 'number'
            && Number.isFinite(value) && value >= 0 && value <= 1;
          const percent = known ? Math.round(value * 100) : null;
          return (
            <div className="resident-need-row" key={key}>
              <span className="resident-need-label">{label}</span>
              <span
                className={`resident-need-track${known ? '' : ' is-unknown'}`}
                role={known ? 'meter' : undefined}
                aria-label={known ? label : undefined}
                aria-valuemin={known ? 0 : undefined}
                aria-valuemax={known ? 100 : undefined}
                aria-valuenow={percent ?? undefined}
                aria-valuetext={known ? `${percent}% · 높을수록 필요가 큼` : undefined}
                aria-hidden={known ? undefined : true}
              >
                {known ? (
                  <span className="resident-need-fill" style={{ width: `${value * 100}%` }} />
                ) : null}
              </span>
              <b className="resident-need-value">{percent === null ? '확인 중' : `${percent}%`}</b>
            </div>
          );
        })}
      </div>
    </div>
  );
}
