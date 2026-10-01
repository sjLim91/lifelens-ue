const missingTranslations = new Set<string>();

function translated(
  domain: string,
  value: string | undefined,
  labels: Record<string, string>,
  fallback: string,
): string {
  const key = value?.trim();
  if (!key) return fallback;
  if (/[가-힣]/.test(key)) return key;
  const label = labels[key];
  if (label) return label;

  const token = `${domain}:${key}`;
  if (!missingTranslations.has(token)) {
    missingTranslations.add(token);
    console.error(`[LifeLens 한글 UI] 번역 등록 누락: ${token}`);
  }
  return fallback;
}

export const KOREAN_SEX_LABELS: Record<string, string> = {
  Male: '남성',
  Female: '여성',
};

export const KOREAN_LIFE_STAGE_LABELS: Record<string, string> = {
  Baby: '영아',
  Toddler: '유아',
  Child: '아동',
  Teen: '청소년',
  YoungAdult: '청년',
  Adult: '성인',
  MiddleAge: '중년',
  Elderly: '노년',
};

export const KOREAN_ACTIVITY_KIND_LABELS: Record<string, string> = {
  Idle: '대기',
  Physical: '생활 행동',
  Social: '사회 행동',
  Civilization: '문명 활동',
  Parenting: '돌봄',
  KnowledgeTeaching: '지식 전수',
};

export const KOREAN_PRESENTATION_KIND_LABELS: Record<string, string> = {
  None: '행동 없음',
  Physical: '생활 행동',
  Social: '사회 행동',
  Civilization: '문명 활동',
  Parenting: '돌봄',
  KnowledgeTeaching: '지식 전수',
};

export const KOREAN_PRESENTATION_PHASE_LABELS: Record<string, string> = {
  Idle: '대기',
  Moving: '이동 중',
  Interacting: '행동 중',
};

export const KOREAN_GOAL_LABELS: Record<string, string> = {
  Idle: '대기',
  Eat: '식사',
  Drink: '물 마시기',
  Sleep: '수면',
  UseToilet: '용변',
  Wash: '씻기',
};

export const KOREAN_SOCIAL_INTENT_LABELS: Record<string, string> = {
  None: '사회 행동 없음',
  Approach: '다가가기',
  Avoid: '피하기',
  Repair: '관계 회복',
  Comfort: '위로하기',
};

export const KOREAN_CIVILIZATION_INTENT_LABELS: Record<string, string> = {
  None: '작업 없음',
  Gather: '채집',
  Store: '저장',
  Retrieve: '꺼내기',
  Explore: '자원 탐색',
  Experiment: '실험',
  Craft: '제작',
};

export const KOREAN_PARENTING_ACTION_LABELS: Record<string, string> = {
  Feed: '먹이기',
  PutToSleep: '재우기',
  Bathe: '씻기기',
  ToiletAssist: '용변 돕기',
  Hold: '안아주기',
  Play: '함께 놀기',
  Educate: '가르치기',
  Discipline: '훈육',
  Comfort: '달래기',
  HealthCare: '돌보기',
};

export const KOREAN_FACILITY_ACTION_LABELS: Record<string, string> = {
  None: '시설 작업 없음',
  Plan: '계획하기',
  DeliverMaterial: '자재 운반',
  Work: '건설 작업',
  Repair: '수리',
  Fuel: '연료 넣기',
  Ignite: '불 붙이기',
  CollectCharcoal: '숯 회수',
  LoadSmeltCharge: '제련 재료 투입',
  CollectMetal: '금속 회수',
  Plant: '씨앗 심기',
  Water: '물주기',
  Tend: '밭 돌보기',
  Harvest: '수확',
  AlloyBronze: '청동 합금 작업',
};

export const KOREAN_MATERIAL_LABELS: Record<string, string> = {
  Unknown: '미확인 자원',
  Stone: '돌',
  Flint: '부싯돌',
  Wood: '나무',
  Fiber: '섬유',
  Clay: '점토',
  Water: '물',
  PlantFood: '식물성 먹거리',
  Bone: '뼈',
  Hide: '가죽',
  CopperOre: '구리 광석',
  TinOre: '주석 광석',
  IronOre: '철 광석',
  Charcoal: '숯',
  CopperMetal: '구리',
  BronzeMetal: '청동',
};

export const KOREAN_ITEM_LABELS: Record<string, string> = {
  RawMaterial: '원재료',
  SharpFlake: '날카로운 석편',
  StoneCuttingTool: '석제 절단 도구',
  Cordage: '끈',
  SimpleContainer: '단순 용기',
  FuelBundle: '연료 묶음',
  DiggingStick: '굴착 막대',
  StoneHammer: '돌망치',
  BronzeEdgeTool: '청동 날도구',
};

export const KOREAN_TECHNIQUE_LABELS: Record<string, string> = {
  None: '기술 없음',
  SharpFlake: '날카로운 석편 제작',
  ChippedStoneTool: '뗀석기 제작',
  FireMaking: '불 피우기',
  FiberCordage: '섬유 끈 만들기',
  SimpleContainer: '단순 용기 만들기',
  DesignatedSanitationArea: '위생 구역 지정',
  DugSanitationPit: '위생 구덩이 만들기',
  PrimitiveStorage: '원시 저장',
  DiggingStick: '굴착 막대 제작',
  StoneHammer: '돌망치 제작',
  CopperSmelting: '구리 제련',
  Cultivation: '재배',
  BronzeAlloying: '청동 합금',
  BronzeEdgeToolmaking: '청동 날도구 제작',
};

export const KOREAN_FACILITY_KIND_LABELS: Record<string, string> = {
  PrimitiveStorage: '원시 저장소',
  FirePit: '화덕',
  WorkSurface: '작업대',
  SleepingPlace: '잠자리',
  Shelter: '쉼터',
  Furnace: '제련로',
  CultivatedPlot: '재배지',
};

export const KOREAN_FACILITY_STATE_LABELS: Record<string, string> = {
  Planned: '계획',
  UnderConstruction: '건설 중',
  Operational: '가동 중',
  Ruined: '파손',
};

export const KOREAN_OBJECT_KIND_LABELS: Record<string, string> = {
  Bed: '잠자리',
  Toilet: '화장실',
  Sink: '씻는 곳',
  Fridge: '식량 보관소',
  Chair: '의자',
  Table: '작업대',
  Sofa: '휴식 장소',
};

export const KOREAN_ROMANCE_STAGE_LABELS: Record<string, string> = {
  Dating: '연애 중',
  Engaged: '약혼',
  Married: '결혼',
  Separated: '별거',
  Divorced: '이혼',
  Widowed: '사별',
  FormerPartners: '이전 연인',
};

export const KOREAN_KINSHIP_LABELS: Record<string, string> = {
  Unrelated: '친족 아님',
  Self: '본인',
  Parent: '부모',
  Child: '자녀',
  Sibling: '형제자매',
  HalfSibling: '이복·반형제',
  Spouse: '배우자',
  Grandparent: '조부모',
  Grandchild: '손자녀',
  InLaw: '인척',
};

export const KOREAN_PREGNANCY_STAGE_LABELS: Record<string, string> = {
  FirstTrimester: '임신 1기',
  SecondTrimester: '임신 2기',
  ThirdTrimester: '임신 3기',
  Due: '출산 예정',
  Completed: '임신 종료',
};

export const KOREAN_WEATHER_LABELS: Record<string, string> = {
  Clear: '맑음',
  Cloudy: '흐림',
  Rain: '비',
  Snow: '눈',
  Fog: '안개',
  Storm: '폭풍',
  Heat: '고온',
  Cold: '한랭',
};

export const KOREAN_BIOME_LABELS: Record<string, string> = {
  TemperateForest: '온대림',
  Meadow: '초원',
  Plains: '평야',
  Hills: '구릉',
  Wetland: '습지',
  DryScrub: '건조 관목지',
  ColdSteppe: '한랭 스텝',
  RockyHighland: '바위 고지대',
  Coast: '해안',
  Ocean: '대양',
};

export const KOREAN_TRAIT_LABELS: Record<string, string> = {
  resilience: '회복력',
  creativity: '창의성',
  discipline: '규율',
  compassion: '공감·연민',
  adaptability: '적응력',
  boldness: '대담성',
  perseverance: '끈기',
  resourcefulness: '생활력',
};

export const KOREAN_LIFE_CONDITION_LABELS: Record<string, string> = {
  physicalHealth: '신체 건강',
  energyCapacity: '에너지',
  movementCapacity: '이동 능력',
  reproductivePotential: '생식 잠재',
  workCapacity: '작업 능력',
  appearanceAgeFactor: '외형 노화',
  lifeGoalFamilyFocus: '가족 지향',
  familyRoleSalience: '가족 역할',
};

export const KOREAN_GENETICS_LABELS: Record<string, string> = {
  faceShape: '얼굴형',
  eyePigment: '눈 색소',
  hairPigment: '머리 색소',
  skinTone: '피부 톤',
  heightPotential: '키 잠재',
  buildPotential: '체격 잠재',
  healthPotential: '건강 잠재',
  learningPotential: '학습 잠재',
  temperamentSensitivity: '기질 민감도',
};

export const KOREAN_DEVELOPMENT_LABELS: Record<string, string> = {
  attachment: '애착',
  confidence: '자신감',
  stress: '스트레스',
  socialSkill: '사회성',
  emotionalSecurity: '정서 안정',
  disciplineInternalization: '규율 내면화',
  learningSupport: '학습 지원',
  health: '발달 건강',
};

export const KOREAN_KNOWLEDGE_SOURCE_LABELS: Record<string, string> = {
  Unknown: '출처 미확인',
  SelfDiscovery: '직접 발견',
  DirectWitness: '직접 목격',
  Teaching: '전수받음',
};

export const KOREAN_MEMORY_SOURCE_LABELS: Record<string, string> = {
  DirectWitness: '직접 목격',
  ToldByOther: '전해 들음',
  Inferred: '추론',
};

export const KOREAN_SOCIAL_EVENT_LABELS: Record<string, string> = {
  PositiveInteraction: '좋은 상호작용',
  Help: '도움',
  Comfort: '위로',
  Conflict: '갈등',
  Betrayal: '배신',
  Rejection: '거절',
  Apology: '사과',
  Intimacy: '친밀한 교류',
  Commitment: '관계 약속',
};

export const KOREAN_SOCIAL_PRESENTATION_LEVEL_LABELS: Record<string, string> = {
  Everyday: '일상',
  Meaningful: '의미 있음',
  Important: '중요',
};

export const KOREAN_SANITATION_SITE_KIND_LABELS: Record<string, string> = {
  DesignatedArea: '지정 위생 구역',
  DugPit: '위생 구덩이',
};

export const KOREAN_SEASON_LABELS: Record<string, string> = {
  Spring: '봄',
  Summer: '여름',
  Autumn: '가을',
  Winter: '겨울',
};

export const KOREAN_PRECIPITATION_LABELS: Record<string, string> = {
  None: '강수 없음',
  Rain: '비',
  Snow: '눈',
};

export const KOREAN_SURFACE_WATER_KIND_LABELS: Record<string, string> = {
  None: '지표수 없음',
  Spring: '샘',
  Stream: '개울',
  River: '강',
  Lake: '호수',
  Wetland: '습지',
  Coast: '해안',
  Ocean: '바다',
};

export const KOREAN_WATER_SALINITY_LABELS: Record<string, string> = {
  Fresh: '민물',
  Brackish: '기수',
  Salt: '바닷물',
};

export const KOREAN_LIFE_EVENT_LABELS: Record<string, string> = {
  Birth: '출생',
  LifeStageChanged: '생애 단계 변화',
  DatingStarted: '연애 시작',
  Engaged: '약혼',
  Married: '결혼',
  CohabitationStarted: '동거 시작',
  PregnancyStarted: '임신 시작',
  ChildBorn: '자녀 출생',
  ParentingMilestone: '육아 변화',
  Separated: '별거',
  Divorced: '이혼',
  PartnerWidowed: '배우자 사별',
  HouseholdChanged: '가구 변화',
  Death: '사망',
  Bereavement: '상실',
};

export const KOREAN_KNOWLEDGE_LEVEL_LABELS: Record<string, string> = {
  Unknown: '알려지지 않음',
  Observed: '관찰됨',
  Hypothesized: '가설 단계',
  Understood: '이해함',
  Reproducible: '재현 가능',
  Practiced: '숙련 중',
  Mastered: '숙달',
};

export const KOREAN_MEMORY_TOKEN_LABELS: Record<string, string> = {
  positive_interaction: '좋은 상호작용',
  help: '도움',
  comfort: '위로',
  conflict: '갈등',
  betrayal: '배신',
  rejection: '거절',
  apology: '사과',
  intimacy: '친밀한 교류',
  commitment: '관계 약속',
  social_event: '사회적 사건',
};

export const KOREAN_MEMORY_TAG_LABELS: Record<string, string> = {
  social: '사회',
  positive: '긍정',
  negative: '부정',
  positive_interaction: '좋은상호작용',
  help: '도움',
  comfort: '위로',
  conflict: '갈등',
  betrayal: '배신',
  rejection: '거절',
  apology: '사과',
  intimacy: '친밀',
  commitment: '약속',
};

export const KOREAN_BELIEF_LABELS: Record<string, string> = {
  is_friendly: '친근한 사람이다',
  is_reliable: '믿고 맡길 수 있다',
  is_caring: '나를 돌봐준다',
  is_safe: '안전한 사람이다',
  is_trustworthy: '신뢰할 수 있다',
  romantically_interested: '연애 감정이 있다',
  wants_repair: '관계를 회복하고 싶어 한다',
  is_committed: '관계를 지킬 의지가 있다',
};

export const KOREAN_PERSON_NAME_LABELS: Record<string, string> = {
  Minjun: '민준',
  Doyun: '도윤',
  Seojun: '서준',
  Jiho: '지호',
  Hyunwoo: '현우',
  Taeyun: '태윤',
  Junho: '준호',
  Siwoo: '시우',
  Gunwoo: '건우',
  Joon: '준',
  Minseok: '민석',
  Jaeho: '재호',
  Seoyun: '서윤',
  Hayun: '하윤',
  Jia: '지아',
  Sua: '수아',
  Minseo: '민서',
  Yerin: '예린',
  Chaewon: '채원',
  Naeun: '나은',
  Jiwon: '지원',
  Yuna: '유나',
  Soyeon: '소연',
  Eunji: '은지',
  Yejun: '예준',
  Eunwoo: '은우',
  Juwon: '주원',
  Hajun: '하준',
  Sunwoo: '선우',
  Yunho: '윤호',
  Jinwoo: '진우',
  Minho: '민호',
  Woojin: '우진',
  Seungmin: '승민',
  Jisung: '지성',
  Jaeyun: '재윤',
  Seoa: '서아',
  Arin: '아린',
  Dayeon: '다연',
  Jiyu: '지유',
  Eunseo: '은서',
  Sena: '세나',
  Yeji: '예지',
  Nari: '나리',
  Haeun: '하은',
  Bomin: '보민',
  Somin: '소민',
  Chaeyeon: '채연',
  Harin: '하린',
  Minjae: '민재',
  Taehun: '태훈',
};

function normalizeObserverToken(value: string | undefined): string {
  return (value ?? '')
    .trim()
    .replace(/([a-z0-9])([A-Z])/g, '$1_$2')
    .replace(/[\s-]+/g, '_')
    .toLowerCase();
}

export function formatResidentName(value: string | undefined): string {
  const text = value?.trim();
  if (!text) return '이름 미확인';
  if (/[가-힣]/.test(text)) return text;

  const exact = KOREAN_PERSON_NAME_LABELS[text];
  if (exact) return exact;

  const suffixed = text.match(/^([A-Za-z]+)(\d+)$/);
  if (suffixed) {
    const base = KOREAN_PERSON_NAME_LABELS[suffixed[1]];
    if (base) return `${base}${suffixed[2]}`;
  }

  const token = `주민이름:${text}`;
  if (!missingTranslations.has(token)) {
    missingTranslations.add(token);
    console.error(`[LifeLens 한글 UI] 번역 등록 누락: ${token}`);
  }
  return `미등록 이름(${text})`;
}

export function formatSex(value: string | undefined): string {
  return translated('성별', value, KOREAN_SEX_LABELS, '성별 미확인');
}

export function formatLifeStage(value: string | undefined): string {
  return translated('생애단계', value, KOREAN_LIFE_STAGE_LABELS, '생애단계 미확인');
}

export function formatActivity(value: string | undefined): string {
  const labels = {
    ...KOREAN_GOAL_LABELS,
    ...KOREAN_SOCIAL_INTENT_LABELS,
    ...KOREAN_CIVILIZATION_INTENT_LABELS,
    ...KOREAN_PARENTING_ACTION_LABELS,
    ...KOREAN_PRESENTATION_KIND_LABELS,
  };
  return translated('행동', value, labels, '행동 확인 중');
}

export function formatActivityKind(value: string | undefined): string {
  return translated('행동분류', value, KOREAN_ACTIVITY_KIND_LABELS, '행동 분류 확인 중');
}

export function formatPresentationKind(value: string | undefined): string {
  return translated('표현행동종류', value, KOREAN_PRESENTATION_KIND_LABELS, '행동 종류 확인 중');
}

export function formatPresentationPhase(value: string | undefined): string {
  return translated('표현행동단계', value, KOREAN_PRESENTATION_PHASE_LABELS, '행동 단계 확인 중');
}

export function formatSocialIntent(value: string | undefined): string {
  return translated('사회행동', value, KOREAN_SOCIAL_INTENT_LABELS, '사회 행동 확인 중');
}

export function formatCivilizationIntent(value: string | undefined): string {
  return translated('문명행동', value, KOREAN_CIVILIZATION_INTENT_LABELS, '작업 확인 중');
}

export function formatParentingAction(value: string | undefined): string {
  return translated('돌봄행동', value, KOREAN_PARENTING_ACTION_LABELS, '돌봄 행동 확인 중');
}

export function formatFacilityAction(value: string | undefined): string {
  return translated('시설행동', value, KOREAN_FACILITY_ACTION_LABELS, '시설 작업 확인 중');
}

export function formatMaterial(value: string | undefined): string {
  return translated('자원', value, KOREAN_MATERIAL_LABELS, '미확인 자원');
}

export function formatItem(value: string | undefined): string {
  return translated('물품', value, KOREAN_ITEM_LABELS, '미확인 물품');
}

export function formatTechnique(value: string | undefined): string {
  return translated('기술', value, KOREAN_TECHNIQUE_LABELS, '미확인 기술');
}

export function formatFacilityKind(value: string | undefined): string {
  return translated('시설종류', value, KOREAN_FACILITY_KIND_LABELS, '미확인 시설');
}

export function formatFacilityState(value: string | undefined): string {
  return translated('시설상태', value, KOREAN_FACILITY_STATE_LABELS, '시설 상태 확인 중');
}

export function formatObjectKind(value: string | undefined): string {
  return translated('생활오브젝트', value, KOREAN_OBJECT_KIND_LABELS, '생활 장소');
}

export function formatWeather(value: string | undefined): string {
  return translated('날씨', value, KOREAN_WEATHER_LABELS, '날씨 확인 중');
}

export function formatBiome(value: string | undefined): string {
  return translated('생물군계', value, KOREAN_BIOME_LABELS, '환경 확인 중');
}

export function formatPartnerStage(value: string | undefined): string {
  return translated('연애단계', value, KOREAN_ROMANCE_STAGE_LABELS, '관계 단계 확인 중');
}

export function formatKinship(value: string | undefined): string {
  return translated('친족관계', value, KOREAN_KINSHIP_LABELS, '가족 관계');
}

export function formatPregnancyStage(value: string | undefined): string {
  return translated('임신단계', value, KOREAN_PREGNANCY_STAGE_LABELS, '임신 단계 확인 중');
}

export function formatTrait(value: string | undefined): string {
  return translated('성향', value, KOREAN_TRAIT_LABELS, '성향 항목 확인 중');
}

export function formatLifeCondition(value: string | undefined): string {
  return translated('신체상태', value, KOREAN_LIFE_CONDITION_LABELS, '신체 상태 확인 중');
}

export function formatGenetics(value: string | undefined): string {
  return translated('유전항목', value, KOREAN_GENETICS_LABELS, '유전 항목 확인 중');
}

export function formatDevelopment(value: string | undefined): string {
  return translated('발달항목', value, KOREAN_DEVELOPMENT_LABELS, '발달 항목 확인 중');
}

export function formatKnowledgeSource(value: string | undefined): string {
  return translated('지식출처', value, KOREAN_KNOWLEDGE_SOURCE_LABELS, '지식 출처 확인 중');
}

export function formatMemorySource(value: string | undefined): string {
  return translated('기억출처', value, KOREAN_MEMORY_SOURCE_LABELS, '기억 출처 확인 중');
}

export function formatSocialEvent(value: string | undefined): string {
  return translated('사회사건', value, KOREAN_SOCIAL_EVENT_LABELS, '사회 사건 확인 중');
}

export function formatSocialPresentationLevel(value: string | undefined): string {
  return translated('사회사건등급', value, KOREAN_SOCIAL_PRESENTATION_LEVEL_LABELS, '사건 중요도 확인 중');
}

export function formatSanitationSiteKind(value: string | undefined): string {
  return translated('위생시설종류', value, KOREAN_SANITATION_SITE_KIND_LABELS, '위생 장소 확인 중');
}

export function formatSeason(value: string | undefined): string {
  return translated('계절', value, KOREAN_SEASON_LABELS, '계절 확인 중');
}

export function formatPrecipitation(value: string | undefined): string {
  return translated('강수', value, KOREAN_PRECIPITATION_LABELS, '강수 상태 확인 중');
}

export function formatSurfaceWaterKind(value: string | undefined): string {
  return translated('지표수종류', value, KOREAN_SURFACE_WATER_KIND_LABELS, '물 종류 확인 중');
}

export function formatWaterSalinity(value: string | undefined): string {
  return translated('염도', value, KOREAN_WATER_SALINITY_LABELS, '염도 확인 중');
}

export function formatLifeEvent(value: string | undefined): string {
  return translated('생애사건', value, KOREAN_LIFE_EVENT_LABELS, '생애 사건 확인 중');
}

export function formatKnowledgeLevel(value: string | undefined): string {
  return translated('지식수준', value, KOREAN_KNOWLEDGE_LEVEL_LABELS, '지식 수준 확인 중');
}

export function formatLocationText(value: string | undefined): string {
  const text = value?.trim();
  if (!text) return '';
  if (/[가-힣]/.test(text)) return text;
  const grid = text.match(/^grid\s*[:(]?\s*(-?\d+)\s*[,/]\s*(-?\d+)\s*\)?$/i);
  if (grid) return `좌표 ${grid[1]}, ${grid[2]}`;
  const known: Record<string, string> = {
    nearby: '주변',
    home: '거주지',
    shelter: '쉼터',
    outdoors: '야외',
    world: '월드',
  };
  return translated('장소', text, known, '장소 정보 확인 중');
}

export function formatMemoryText(value: string | undefined): string {
  const text = value?.trim();
  if (!text) return '기억 내용 미확인';
  if (/[가-힣]/.test(text)) return text;
  const normalized = normalizeObserverToken(text);
  return translated(
    '기억내용',
    normalized,
    KOREAN_MEMORY_TOKEN_LABELS,
    `미등록 기억(${text})`,
  );
}

export function formatMemoryTag(value: string | undefined): string {
  const text = value?.trim();
  if (!text) return '미등록 태그';
  if (/[가-힣]/.test(text)) return text;
  const normalized = normalizeObserverToken(text);
  return translated(
    '기억태그',
    normalized,
    KOREAN_MEMORY_TAG_LABELS,
    `미등록 태그(${text})`,
  );
}

export function formatBelief(value: string | undefined): string {
  const text = value?.trim();
  if (!text) return '믿음 내용 미확인';
  if (/[가-힣]/.test(text)) return text;
  const normalized = normalizeObserverToken(text);
  return translated(
    '믿음',
    normalized,
    KOREAN_BELIEF_LABELS,
    `미등록 믿음(${text})`,
  );
}
