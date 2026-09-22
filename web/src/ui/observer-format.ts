export function formatDay(minuteValue: unknown): string {
  const minute = Math.max(0, Number(minuteValue) || 0);
  const day = Math.floor(minute / 1440) + 1;
  const clock = minute % 1440;
  const hour = String(Math.floor(clock / 60)).padStart(2, '0');
  const minuteText = String(clock % 60).padStart(2, '0');
  return `${day}일차 · ${hour}:${minuteText}`;
}

export function formatPercent(value: unknown): string {
  const normalized = Math.max(0, Math.min(1, Number(value) || 0));
  return `${Math.round(normalized * 100)}%`;
}


export function formatSex(value: string | undefined): string {
  if (value === 'Male') return '남성';
  if (value === 'Female') return '여성';
  return value || '—';
}

export function formatLifeStage(value: string | undefined): string {
  const labels: Record<string, string> = {
    Baby: '영아',
    Toddler: '유아',
    Child: '아동',
    Teen: '청소년',
    YoungAdult: '청년',
    Adult: '성인',
    MiddleAge: '중년',
    Elder: '노년',
    Unknown: '알 수 없음',
  };
  return value ? (labels[value] ?? value) : '—';
}

export function formatActivity(value: string | undefined): string {
  const labels: Record<string, string> = {
    Idle: '대기',
    Walk: '이동',
    Approach: '접근',
    Eat: '식사',
    Drink: '음수',
    Sleep: '수면',
    UseToilet: '화장실',
    Wash: '씻기',
    Repair: '수리',
    Comfort: '위로',
    Talk: '대화',
    Gather: '채집',
    Build: '건설',
  };
  return value ? (labels[value] ?? value) : '대기';
}

export function formatWeather(value: string | undefined): string {
  const labels: Record<string, string> = {
    Clear: '맑음',
    Cloudy: '흐림',
    Rain: '비',
    Snow: '눈',
    Fog: '안개',
    Storm: '폭풍',
    Heat: '고온',
    Cold: '한랭',
  };
  return value ? (labels[value] ?? value) : '맑음';
}

export function formatBiome(value: string | undefined): string {
  const labels: Record<string, string> = {
    TemperateForest: '온대림',
    Meadow: '초원',
    Plains: '평야',
    Hills: '구릉',
    Wetland: '습지',
    DryScrub: '건조 관목지',
    ColdSteppe: '한랭 스텝',
    Ocean: '대양',
    Coast: '해안',
  };
  return value ? (labels[value] ?? value) : '알 수 없음';
}

export function formatPartnerStage(value: string | undefined): string {
  const labels: Record<string, string> = {
    Stranger: '낯선 사이',
    Dating: '연애 중',
    Partner: '파트너',
    Cohabiting: '동거',
    Married: '결혼',
    Separated: '별거',
    Widowed: '사별',
  };
  return value ? (labels[value] ?? value) : '';
}


const TECHNIQUE_LABELS: Record<number, string> = {
  1: '날카로운 박편 제작',
  2: '뗀석기 제작',
  3: '불 피우기',
  4: '섬유 끈 제작',
  5: '간이 용기 제작',
  6: '지정 위생 구역',
  7: '위생 구덩이',
  8: '원시 저장소',
  9: '굴착봉 제작',
  10: '돌망치 제작',
  11: '구리 제련',
};

function formatTechniqueProposition(value: string): string | null {
  const match = /^demonstrated civilization technique:(\d+)$/.exec(value);
  if (!match) return null;
  return TECHNIQUE_LABELS[Number(match[1])] ?? '문명 기술';
}

export function formatMemoryText(value: string | undefined): string {
  if (!value) return '기록된 경험';

  const technique = formatTechniqueProposition(value);
  if (technique) return `${technique} 기술 시연을 목격함`;

  const labels: Record<string, string> = {
    'experienced unsanitary surroundings': '비위생적인 주변 환경을 경험함',
    positive_interaction: '긍정적인 상호작용을 경험함',
    help: '도움을 주고받은 일을 기억함',
    comfort: '위로를 주고받은 일을 기억함',
    conflict: '갈등이 있었던 일을 기억함',
    betrayal: '배신당하거나 배신한 일을 기억함',
    rejection: '거절을 경험한 일을 기억함',
    apology: '사과가 오간 일을 기억함',
    intimacy: '친밀한 상호작용을 기억함',
    commitment: '관계에 대한 약속을 기억함',
  };
  return labels[value] ?? '기록된 경험';
}

export function formatBeliefText(
  value: string | undefined,
  stanceValue: unknown,
): string {
  if (!value) return '형성된 믿음';

  const stance = Number(stanceValue) || 0;
  const supports = stance >= 0;

  const technique = formatTechniqueProposition(value);
  if (technique) {
    return supports
      ? `${technique} 기술을 시연했다고 믿음`
      : `${technique} 기술을 시연했다는 주장에 동의하지 않음`;
  }

  if (value === 'human-waste contamination is a recurring sanitation problem') {
    return supports
      ? '인분 오염이 반복되는 위생 문제라고 판단함'
      : '인분 오염이 반복적인 위생 문제라는 판단에 동의하지 않음';
  }

  const normalized = value.trim().toLowerCase().replace(/\s+/g, '_');
  const labels: Record<string, [string, string]> = {
    is_friendly: ['대상이 친근한 사람이라고 생각함', '대상이 친근하지 않다고 생각함'],
    is_reliable: ['대상이 믿고 의지할 만하다고 생각함', '대상이 믿고 의지하기 어렵다고 생각함'],
    is_caring: ['대상이 배려하는 사람이라고 생각함', '대상이 배려하지 않는다고 생각함'],
    is_safe: ['대상이 안전한 사람이라고 생각함', '대상이 안전하지 않다고 생각함'],
    is_trustworthy: ['대상을 신뢰할 수 있다고 생각함', '대상을 신뢰하기 어렵다고 생각함'],
    romantically_interested: ['대상이 연애 감정을 갖고 있다고 생각함', '대상이 연애 감정을 갖고 있지 않다고 생각함'],
    wants_repair: ['대상이 관계 회복을 원한다고 생각함', '대상이 관계 회복을 원하지 않는다고 생각함'],
    is_committed: ['대상이 관계에 헌신하고 있다고 생각함', '대상이 관계에 헌신하지 않는다고 생각함'],
  };
  const pair = labels[normalized];
  return pair ? pair[supports ? 0 : 1] : '형성된 믿음';
}

export function formatMemoryLocation(value: string | undefined): string {
  if (!value) return '';

  const grid = /^grid:(-?\d+),(-?\d+)$/.exec(value);
  if (grid) return `좌표 ${grid[1]}, ${grid[2]}`;

  const labels: Record<string, string> = {
    'civilization-worksite': '문명 작업 장소',
  };
  return labels[value] ?? '기록된 장소';
}

export function formatLifeEventType(value: string | undefined): string {
  const labels: Record<string, string> = {
    Birth: '탄생',
    LifeStageChanged: '성장 단계 변화',
    DatingStarted: '연애 시작',
    Engaged: '약혼',
    Married: '결혼',
    CohabitationStarted: '동거 시작',
    PregnancyStarted: '임신 시작',
    ChildBorn: '자녀 출생',
    ParentingMilestone: '육아 이정표',
    Separated: '별거',
    Divorced: '이혼',
    PartnerWidowed: '배우자 사별',
    HouseholdChanged: '가구 변화',
    Death: '사망',
    Bereavement: '상실과 애도',
  };
  return value ? (labels[value] ?? '주요 사건') : '주요 사건';
}
