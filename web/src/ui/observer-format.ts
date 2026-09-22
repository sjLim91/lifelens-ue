import type { Resident } from '../runtime/core-types';

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


export function formatSocialEventType(value: string | undefined): string {
  const labels: Record<string, string> = {
    PositiveInteraction: '대화를 나눔',
    Help: '도움을 줌',
    Comfort: '위로함',
    Conflict: '갈등',
    Betrayal: '배신',
    Rejection: '거절',
    Apology: '사과',
    Intimacy: '친밀한 교류',
    Commitment: '관계 약속',
  };
  return value ? (labels[value] ?? '사회적 상호작용') : '사회적 상호작용';
}

export function formatFacilityKind(value: string | undefined): string {
  const labels: Record<string, string> = {
    PrimitiveStorage: '원시 저장소',
    FirePit: '화덕',
    WorkSurface: '작업대',
    SleepingPlace: '잠자리',
    Shelter: '쉼터',
    Furnace: '용광로',
  };
  return value ? (labels[value] ?? '시설') : '시설';
}

export function formatFacilityState(value: string | undefined): string {
  const labels: Record<string, string> = {
    Planned: '계획됨',
    UnderConstruction: '건설 중',
    Operational: '사용 가능',
    Ruined: '폐허',
  };
  return value ? (labels[value] ?? '상태 확인 중') : '상태 확인 중';
}

function actionTargetName(
  resident: Resident,
  residents: Resident[],
): string {
  const targetId = resident.contextAction?.targetResidentId
    ?? resident.presentation?.targetResidentId
    ?? resident.activityTargetId
    ?? '';
  if (!targetId) return resident.activityTargetName ?? '상대';
  return residents.find((candidate) => candidate.id === targetId)?.name
    ?? resident.activityTargetName
    ?? '상대';
}

export function formatMaterialName(value: string | undefined): string {
  const labels: Record<string, string> = {
    Stone: '돌',
    Flint: '부싯돌',
    Wood: '나무',
    Fiber: '섬유',
    Clay: '점토',
    Water: '물',
    PlantFood: '먹을거리',
    Bone: '뼈',
    Hide: '가죽',
    CopperOre: '구리 광석',
    TinOre: '주석 광석',
    IronOre: '철 광석',
    Charcoal: '숯',
    CopperMetal: '구리',
  };
  return value ? (labels[value] ?? '재료') : '재료';
}

export function formatTechniqueName(value: string | undefined): string {
  const labels: Record<string, string> = {
    SharpFlake: '날카로운 박편',
    ChippedStoneTool: '뗀석기',
    FireMaking: '불 피우기',
    FiberCordage: '섬유 끈',
    SimpleContainer: '간이 용기',
    DesignatedSanitationArea: '지정 위생구역',
    DugSanitationPit: '구덩이식 위생시설',
    PrimitiveStorage: '원시 저장소',
    DiggingStick: '굴착봉',
    StoneHammer: '돌망치',
    CopperSmelting: '구리 제련',
  };
  return value ? (labels[value] ?? '기술') : '기술';
}

export function formatResidentCurrentAction(
  resident: Resident,
  residents: Resident[] = [],
): string {
  const presentation = resident.presentation;
  const action = resident.contextAction;
  const moving = presentation?.phase === 'Moving';
  const target = actionTargetName(resident, residents);

  if (action?.active) {
    if (action.kind === 'Social') {
      switch (action.socialIntent) {
        case 'Approach': return moving ? `${target}에게 다가가는 중` : `${target}와 대화 중`;
        case 'Repair': return moving ? `${target}에게 사과하러 가는 중` : `${target}에게 사과 중`;
        case 'Comfort': return moving ? `${target}을 위로하러 가는 중` : `${target}을 위로 중`;
        case 'Avoid': return `${target}을 피하는 중`;
        default: return `${target}와 상호작용 중`;
      }
    }

    if (action.kind === 'KnowledgeTeaching') {
      return moving
        ? `${target}에게 기술을 가르치러 가는 중`
        : `${target}에게 ${formatTechniqueName(action.technique)} 기술을 가르치는 중`;
    }

    if (action.kind === 'Parenting') {
      const labels: Record<string, string> = {
        Feed: '먹이는 중',
        PutToSleep: '재우는 중',
        Bathe: '씻기는 중',
        ToiletAssist: '화장실을 도와주는 중',
        Hold: '안아주는 중',
        Play: '놀아주는 중',
        Educate: '가르치는 중',
        Discipline: '훈육하는 중',
        Comfort: '달래주는 중',
        HealthCare: '돌보는 중',
      };
      return moving
        ? `${target}을 돌보러 가는 중`
        : `${target}을 ${labels[action.parentingAction ?? ''] ?? '돌보는 중'}`;
    }

    if (action.kind === 'Civilization') {
      const facility = formatFacilityKind(action.facilityKind);
      const build: Record<string, string> = {
        Plan: `${facility} 자리 선정 중`,
        DeliverMaterial: `${facility} 재료 운반 중`,
        Work: `${facility} 건설 중`,
        Repair: `${facility} 수리 중`,
        Fuel: `${facility}에 연료 공급 중`,
        Ignite: `${facility}에 불 붙이는 중`,
        CollectCharcoal: '숯을 거두는 중',
        LoadSmeltCharge: '용광로에 광석과 숯을 넣는 중',
        CollectMetal: '제련한 구리를 거두는 중',
      };
      if (action.facilityAction && action.facilityAction !== 'None') {
        return build[action.facilityAction] ?? `${facility} 작업 중`;
      }
      switch (action.civilizationIntent) {
        case 'Gather': return `${formatMaterialName(action.material)} 채집 중`;
        case 'Store': return `${formatMaterialName(action.material)} 저장 중`;
        case 'Retrieve': return `${formatMaterialName(action.material)} 꺼내는 중`;
        case 'Experiment': return `${formatTechniqueName(action.technique)} 실험 중`;
        case 'Craft': return `${formatTechniqueName(action.technique)} 제작 중`;
        default: return '생활 기반 작업 중';
      }
    }
  }

  if (presentation?.active && presentation.kind === 'Physical') {
    switch (presentation.physicalGoal) {
      case 'Eat': return moving ? '먹을거리로 이동 중' : '식사 중';
      case 'Drink': return moving ? '물로 이동 중' : '물을 마시는 중';
      case 'Sleep': return moving ? '잠자리로 이동 중' : '수면 중';
      case 'Wash': return moving ? '씻을 곳으로 이동 중' : '씻는 중';
      case 'UseToilet':
        if (presentation.designatedSanitationSite) {
          return moving ? '위생시설로 이동 중' : '위생시설 이용 중';
        }
        return moving ? '야외 배변 장소로 이동 중' : '야외에서 용변 보는 중';
      default:
        break;
    }
  }

  return formatActivity(resident.activityLabel);
}


export function formatItemKind(value: string | undefined): string {
  const labels: Record<string, string> = {
    RawMaterial: '원재료',
    SharpFlake: '날카로운 박편',
    StoneCuttingTool: '뗀석기 도구',
    Cordage: '섬유 끈',
    SimpleContainer: '간이 용기',
    FuelBundle: '연료 묶음',
    DiggingStick: '굴착봉',
    StoneHammer: '돌망치',
  };
  return value ? (labels[value] ?? '물품') : '물품';
}

export function formatKnowledgeLevel(value: string | undefined): string {
  const labels: Record<string, string> = {
    Unknown: '모름',
    Observed: '목격',
    Hypothesized: '가설',
    Understood: '이해',
    Reproducible: '재현 가능',
    Practiced: '숙련',
    Mastered: '통달',
  };
  return value ? (labels[value] ?? '알고 있음') : '알고 있음';
}

export function formatKnowledgeSource(value: string | undefined): string {
  const labels: Record<string, string> = {
    Unknown: '출처 불명',
    SelfDiscovery: '직접 발견',
    DirectWitness: '직접 목격',
    Teaching: '다른 주민에게 배움',
  };
  return value ? (labels[value] ?? '출처 불명') : '출처 불명';
}

export function formatMemorySource(value: string | undefined): string {
  const labels: Record<string, string> = {
    DirectWitness: '직접 경험/목격',
    ToldByOther: '다른 사람에게 전해 들음',
    Inferred: '추론해서 형성',
  };
  return value ? (labels[value] ?? '출처 불명') : '출처 불명';
}
