#pragma once

#include "CoreMinimal.h"
#include "Simulation/LLCoreReadTypes.h"
#include "Simulation/LLCivilizationReadTypes.h"

// Observer-only Korean display text. Core ids and enum values stay language neutral.
namespace LLObserverKorean
{
    inline const TCHAR* const Day = TEXT("일차");
    inline const TCHAR* const CoreUnavailable = TEXT("Core 데이터를 사용할 수 없습니다");
    inline const TCHAR* const CoreTraitUnavailable = TEXT("Core 특성·선호 데이터를 사용할 수 없습니다");
    inline const TCHAR* const CoreSkillUnavailable = TEXT("Core 기술 데이터를 사용할 수 없습니다");
    inline const TCHAR* const None = TEXT("없음");
    inline const TCHAR* const Resident = TEXT("주민");

    inline const TCHAR* const Households = TEXT("가구 수");
    inline const TCHAR* const ActiveCouples = TEXT("현재 커플");
    inline const TCHAR* const Pregnancies = TEXT("임신 중");
    inline const TCHAR* const KnownTechniques = TEXT("알고 있는 기술");
    inline const TCHAR* const StoredUnits = TEXT("저장 자원");
    inline const TCHAR* const Facilities = TEXT("시설 수");
    inline const TCHAR* const FacilitiesUnderConstruction = TEXT("건설 중");
    inline const TCHAR* const FacilitiesOperational = TEXT("가동 중");
    inline const TCHAR* const RecentDiscovery = TEXT("최근 발견");

    inline const TCHAR* const Hunger = TEXT("배고픔");
    inline const TCHAR* const Thirst = TEXT("목마름");
    inline const TCHAR* const Energy = TEXT("에너지");
    inline const TCHAR* const Hygiene = TEXT("위생");
    inline const TCHAR* const Bladder = TEXT("배뇨");

    inline const TCHAR* const Introversion = TEXT("내향성");
    inline const TCHAR* const Conscientiousness = TEXT("성실성");
    inline const TCHAR* const Openness = TEXT("개방성");
    inline const TCHAR* const Agreeableness = TEXT("친화성");
    inline const TCHAR* const EmotionalStability = TEXT("정서 안정성");
    inline const TCHAR* const Empathy = TEXT("공감성");
    inline const TCHAR* const Impulsiveness = TEXT("충동성");
    inline const TCHAR* const RiskTolerance = TEXT("위험 감수성");
    inline const TCHAR* const Ambition = TEXT("야망");
    inline const TCHAR* const Patience = TEXT("인내심");
    inline const TCHAR* const Sociability = TEXT("사교성");
    inline const TCHAR* const Curiosity = TEXT("호기심");
    inline const TCHAR* const Orderliness = TEXT("질서 선호");
    inline const TCHAR* const Adaptability = TEXT("적응력");

    inline const TCHAR* const Resilience = TEXT("회복탄력성");
    inline const TCHAR* const Creativity = TEXT("창의성");
    inline const TCHAR* const Discipline = TEXT("자기통제");
    inline const TCHAR* const Compassion = TEXT("연민");
    inline const TCHAR* const Boldness = TEXT("대담성");
    inline const TCHAR* const Perseverance = TEXT("끈기");
    inline const TCHAR* const Resourcefulness = TEXT("임기응변");

    inline const TCHAR* const Preferences = TEXT("선호");
    inline const TCHAR* const Socializing = TEXT("사교");
    inline const TCHAR* const Solitude = TEXT("혼자 있기");
    inline const TCHAR* const Exploration = TEXT("탐험");
    inline const TCHAR* const Crafting = TEXT("제작");
    inline const TCHAR* const Gathering = TEXT("채집");
    inline const TCHAR* const Comfort = TEXT("안락함");
    inline const TCHAR* const Novelty = TEXT("새로움");
    inline const TCHAR* const Order = TEXT("질서");
    inline const TCHAR* const Learning = TEXT("학습");

    inline const TCHAR* const Joy = TEXT("기쁨");
    inline const TCHAR* const Sadness = TEXT("슬픔");
    inline const TCHAR* const Anger = TEXT("분노");
    inline const TCHAR* const Fear = TEXT("두려움");
    inline const TCHAR* const Affection = TEXT("애정");
    inline const TCHAR* const Anxiety = TEXT("불안");
    inline const TCHAR* const Grief = TEXT("애도");
    inline const TCHAR* const Pride = TEXT("자부심");
    inline const TCHAR* const Jealousy = TEXT("질투");
    inline const TCHAR* const Relief = TEXT("안도");
    inline const TCHAR* const Embarrassment = TEXT("당혹감");

    inline const TCHAR* const Bond = TEXT("유대");
    inline const TCHAR* const Romance = TEXT("연애 가능성");
    inline const TCHAR* const Trust = TEXT("신뢰");
    inline const TCHAR* const Respect = TEXT("존중");
    inline const TCHAR* const Familiarity = TEXT("친숙도");
    inline const TCHAR* const Attraction = TEXT("끌림");
    inline const TCHAR* const RomanticInterest = TEXT("연애 관심");
    inline const TCHAR* const SexualAttraction = TEXT("성적 끌림");
    inline const TCHAR* const Commitment = TEXT("헌신");
    inline const TCHAR* const Conflict = TEXT("갈등");
    inline const TCHAR* const Grudge = TEXT("원한");

    inline const TCHAR* const Household = TEXT("가구");
    inline const TCHAR* const Partner = TEXT("파트너");
    inline const TCHAR* const ExpectingChild = TEXT("아이를 기다리는 중");
    inline const TCHAR* const Parents = TEXT("부모");
    inline const TCHAR* const Children = TEXT("자녀");
    inline const TCHAR* const Siblings = TEXT("형제자매");

    inline const TCHAR* const CarriedUnits = TEXT("소지 자원");
    inline const TCHAR* const Reproducible = TEXT("재현 가능한 기술");
    inline const TCHAR* const Inventory = TEXT("소지품");
    inline const TCHAR* const Techniques = TEXT("기술");

    inline FString RomanceStage(ELLCoreRomanceStage Stage)
    {
        switch (Stage)
        {
            case ELLCoreRomanceStage::Dating:         return TEXT("연애 중");
            case ELLCoreRomanceStage::Engaged:        return TEXT("약혼");
            case ELLCoreRomanceStage::Married:        return TEXT("결혼");
            case ELLCoreRomanceStage::Separated:      return TEXT("별거");
            case ELLCoreRomanceStage::Divorced:       return TEXT("이혼");
            case ELLCoreRomanceStage::Widowed:        return TEXT("사별");
            case ELLCoreRomanceStage::FormerPartners: return TEXT("과거 연인");
            case ELLCoreRomanceStage::None:
            default:                                   return TEXT("없음");
        }
    }

    inline FString Material(ELLCoreMaterialKind Value)
    {
        switch (Value)
        {
            case ELLCoreMaterialKind::Stone:       return TEXT("돌");
            case ELLCoreMaterialKind::Flint:       return TEXT("부싯돌");
            case ELLCoreMaterialKind::Wood:        return TEXT("목재");
            case ELLCoreMaterialKind::Fiber:       return TEXT("섬유");
            case ELLCoreMaterialKind::Clay:        return TEXT("점토");
            case ELLCoreMaterialKind::Water:       return TEXT("물");
            case ELLCoreMaterialKind::PlantFood:   return TEXT("식물성 식량");
            case ELLCoreMaterialKind::Bone:        return TEXT("뼈");
            case ELLCoreMaterialKind::Hide:        return TEXT("가죽");
            case ELLCoreMaterialKind::CopperOre:   return TEXT("구리 광석");
            case ELLCoreMaterialKind::TinOre:      return TEXT("주석 광석");
            case ELLCoreMaterialKind::IronOre:     return TEXT("철 광석");
            case ELLCoreMaterialKind::Charcoal:    return TEXT("목탄");
            case ELLCoreMaterialKind::CopperMetal: return TEXT("구리");
            case ELLCoreMaterialKind::Unknown:
            default:                               return TEXT("미상");
        }
    }

    inline FString Item(ELLCoreItemKind Value)
    {
        switch (Value)
        {
            case ELLCoreItemKind::RawMaterial:       return TEXT("원재료");
            case ELLCoreItemKind::SharpFlake:        return TEXT("날카로운 박편");
            case ELLCoreItemKind::StoneCuttingTool:  return TEXT("석재 절삭도구");
            case ELLCoreItemKind::Cordage:           return TEXT("끈");
            case ELLCoreItemKind::SimpleContainer:   return TEXT("간이 용기");
            case ELLCoreItemKind::FuelBundle:        return TEXT("연료 묶음");
            case ELLCoreItemKind::DiggingStick:      return TEXT("굴착봉");
            case ELLCoreItemKind::StoneHammer:       return TEXT("돌망치");
            default:                                 return TEXT("물품");
        }
    }

    inline FString Technique(ELLCoreTechniqueId Value)
    {
        switch (Value)
        {
            case ELLCoreTechniqueId::SharpFlake:                return TEXT("날카로운 박편 제작");
            case ELLCoreTechniqueId::ChippedStoneTool:          return TEXT("뗀석기 제작");
            case ELLCoreTechniqueId::FireMaking:                return TEXT("불 피우기");
            case ELLCoreTechniqueId::FiberCordage:              return TEXT("섬유 끈 제작");
            case ELLCoreTechniqueId::SimpleContainer:           return TEXT("간이 용기 제작");
            case ELLCoreTechniqueId::DesignatedSanitationArea:  return TEXT("지정 위생 구역");
            case ELLCoreTechniqueId::DugSanitationPit:          return TEXT("위생 구덩이");
            case ELLCoreTechniqueId::PrimitiveStorage:          return TEXT("원시 저장소");
            case ELLCoreTechniqueId::DiggingStick:              return TEXT("굴착봉 제작");
            case ELLCoreTechniqueId::StoneHammer:               return TEXT("돌망치 제작");
            case ELLCoreTechniqueId::CopperSmelting:            return TEXT("구리 제련");
            case ELLCoreTechniqueId::None:
            default:                                             return TEXT("없음");
        }
    }

    inline FString KnowledgeLevel(ELLCoreKnowledgeLevel Value)
    {
        switch (Value)
        {
            case ELLCoreKnowledgeLevel::Observed:      return TEXT("관찰함");
            case ELLCoreKnowledgeLevel::Hypothesized:  return TEXT("가설 단계");
            case ELLCoreKnowledgeLevel::Understood:    return TEXT("이해함");
            case ELLCoreKnowledgeLevel::Reproducible:  return TEXT("재현 가능");
            case ELLCoreKnowledgeLevel::Practiced:     return TEXT("숙련 중");
            case ELLCoreKnowledgeLevel::Mastered:      return TEXT("숙달");
            case ELLCoreKnowledgeLevel::Unknown:
            default:                                    return TEXT("미상");
        }
    }

    inline FString KnowledgeSource(ELLCoreKnowledgeSource Value)
    {
        switch (Value)
        {
            case ELLCoreKnowledgeSource::SelfDiscovery: return TEXT("자체 발견");
            case ELLCoreKnowledgeSource::DirectWitness: return TEXT("직접 목격");
            case ELLCoreKnowledgeSource::Teaching:      return TEXT("전수받음");
            case ELLCoreKnowledgeSource::Unknown:
            default:                                     return TEXT("출처 미상");
        }
    }
}
