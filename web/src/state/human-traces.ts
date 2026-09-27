import type { HumanTrace, Resident, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { formatFacilityKind, formatMaterial } from '../localization/korean';

export const MAX_VISIBLE_HUMAN_TRACES = 64;

export function visibleHumanTraces(terrain: TerrainWindow | null): HumanTrace[] {
  if (!terrain?.available || !Array.isArray(terrain.humanTraces?.entries)) return [];
  const chunks = new Set(terrain.chunks.map(chunk => `${chunk.x}:${chunk.y}`));
  const seen = new Set<string>();
  const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  return terrain.humanTraces.entries.filter(trace => {
    if (!trace || typeof trace.id !== 'string' || !trace.id || seen.has(trace.id)
      || !Number.isFinite(trace.gridX) || !Number.isFinite(trace.gridY)
      || !chunks.has(`${Math.floor(trace.gridX / span)}:${Math.floor(trace.gridY / span)}`)) return false;
    const valid = trace.kind === 'ResourceUse'
      ? Number.isFinite(trace.quantity) && Number.isFinite(trace.baselineQuantity)
        && trace.quantity >= 0 && trace.baselineQuantity > trace.quantity
        && trace.material !== 'Water'
      : trace.kind === 'Residue'
        ? Number.isFinite(trace.amount) && trace.amount > 0.01
          && Number.isFinite(trace.intensity) && trace.intensity > 0.01 && trace.intensity <= 1
          && Number.isFinite(trace.radiusTiles) && trace.radiusTiles >= 1 && trace.radiusTiles <= 16
        : trace.kind === 'Facility'
          && Number.isFinite(trace.progress01) && trace.progress01 >= 0 && trace.progress01 <= 1
          && Number.isFinite(trace.deliveredMaterialUnits) && Number.isFinite(trace.requiredMaterialUnits)
          && trace.deliveredMaterialUnits >= 0 && trace.requiredMaterialUnits > 0
          && (trace.deliveredMaterialUnits > 0 || trace.progress01 > 0);
    if (valid) seen.add(trace.id);
    return valid;
  }).slice(0, MAX_VISIBLE_HUMAN_TRACES);
}


export function describeHumanTrace(trace: HumanTrace, residents: Resident[] = []) {
  if (trace.kind === 'ResourceUse') {
    return {
      title: `${formatMaterial(trace.material)} 채집 지점`,
      status: trace.quantity === 0 ? '소진' : '자원 감소',
      detail: `남은 수량 ${trace.quantity} / 처음 ${trace.baselineQuantity}`,
      note: trace.renewable ? '시간이 지나면 자원이 다시 자랍니다.' : '이 자원은 자연적으로 회복되지 않습니다.',
    };
  }
  const source = residents.find(resident => resident.id === trace.sourceResidentId)?.name;
  if (trace.kind === 'Residue') {
    return {
      title: '야외 용변 흔적',
      status: trace.intensity >= 0.4 ? '뚜렷한 흔적' : '옅어지는 흔적',
      detail: source ? `최근 이곳에 흔적을 남긴 주민 · ${source}` : '주민이 남긴 환경 흔적',
      note: '시간에 따라 옅어지며 주변 위생에 영향을 줍니다.',
    };
  }
  const status = trace.state === 'Ruined' ? '무너짐'
    : trace.state === 'Operational' ? (trace.active ? (trace.lit ? '불이 켜짐' : '사용 가능') : '사용 중지')
      : trace.progress01 > 0 ? `건설 ${Math.round(trace.progress01 * 100)}%` : '자재 모으는 중';
  return {
    title: formatFacilityKind(trace.facilityKind), status,
    detail: `모인 자재 ${trace.deliveredMaterialUnits} / 필요 ${trace.requiredMaterialUnits}`,
    note: source ? `최근 작업한 주민 · ${source}` : '주민이 실제로 자재와 작업을 투입한 장소입니다.',
  };
}

export function humanTraceFocus(trace: Pick<HumanTrace, 'gridX' | 'gridY'>) {
  const span = WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const size = WORLD_GRID_CONTRACT.worldUnitsPerChunk;
  const centerChunkX = Math.floor(trace.gridX / span);
  const centerChunkY = Math.floor(trace.gridY / span);
  return {
    centerChunkX, centerChunkY,
    panX: (trace.gridX / span - centerChunkX - 0.5) * size,
    panZ: (trace.gridY / span - centerChunkY - 0.5) * size,
  };
}
