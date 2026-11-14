import type { ArtifactPart, Resident } from '../runtime/core-types';
import { formatDay, formatPercent } from './observer-format';

const SHAPES = ['막대', '날', '뾰족끝', '용기', '가열판', '끈'];
const JOINTS = ['일체형', '묶음', '끼움'];
const MATERIALS: Record<string, string> = {
  Stone: '돌', Flint: '부싯돌', Wood: '나무', Fiber: '섬유', Clay: '점토',
  Bone: '뼈', Hide: '가죽', CopperOre: '구리 광석', TinOre: '주석 광석',
  IronOre: '철 광석', Charcoal: '숯', CopperMetal: '구리', TinMetal: '주석', Bronze: '청동',
};
const EVENTS = ['실험 실패·재료 소모', '제작 성공', '실제 사용', '파손', '재료를 써서 보수',
  '제작법 학습', '도입 거부', '제작법 기록', '잃었던 설계 재발견'];
const SOURCES = ['직접 실험', '현장 시범', '물리 기록'];

export function artifactStructure(parts: ArtifactPart[]): string {
  return parts.map(p => `${MATERIALS[p.material] ?? '미확인 재료'} ${SHAPES[p.shape] ?? '미확인 형상'} ${p.size}단위 (${JOINTS[p.joint] ?? '미확인 결합'})`).join(' + ');
}

export function ResidentInventionPanel({ resident }: { resident: Resident }) {
  const invention = resident.civilization?.invention;
  return <details className="resident-mind-panel">
    <summary>발명과 제작 지식 <span>{invention?.artifacts.length ?? 0}개 물건</span></summary>
    {!invention?.techniques.length && <p className="hint">아직 직접 시험하거나 배운 설계가 없습니다.</p>}
    {invention?.artifacts.map(a => <section key={a.id} aria-label="실제 발명품">
      <h3>발명품 #{a.id}</h3>
      <p>{artifactStructure(a.parts)}</p>
      <p className="hint">내구 {formatPercent(a.durability)} · 실제 사용 {a.uses}회 · 보수 {a.repairs}회</p>
      <p className="hint">베기 {formatPercent(a.cutting)} · 파기 {formatPercent(a.digging)} · 운반 {a.carrying.toFixed(2)} · 가열 {formatPercent(a.heating)}</p>
    </section>)}
    {invention?.techniques.map(t => <section key={t.designId} aria-label="개인 제작 지식">
      <h3>설계 #{t.designId}</h3>
      <p>{artifactStructure(t.parts)}</p>
      <p className="hint">{SOURCES[t.source] ?? '미확인 경로'} · {t.reproducible ? '재현 가능' : '가설·학습 중'} · 실험 {t.trials}회 / 성공 {t.successes}회 · 직접 사용 {t.uses}회 · 시범·기록 학습 {t.demonstrations}회</p>
    </section>)}
    {(invention?.recordCount ?? 0) > 0 && <p>보유 제작 기록 {invention?.recordCount}개</p>}
    {invention && invention.events.length > 0 && <ol aria-label="발명 인과 기록">
      {invention.events.map((e, i) => <li key={`${e.minute}:${e.kind}:${e.artifactId}:${i}`}>
        {formatDay(e.minute)} · {EVENTS[e.kind] ?? '미확인 사건'}
        {e.rawUnits > 0 && ` · 원료 ${e.rawUnits}단위 소모`}
        {e.work > 0 && ` · 작업량 ${e.work.toFixed(1)}`}
      </li>)}
    </ol>}
  </details>;
}
