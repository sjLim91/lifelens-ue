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
