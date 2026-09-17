#pragma once

// Observer-facing text and colour helpers. User-visible wording stays in this
// presentation layer; Core ids remain language neutral.

#include "CoreMinimal.h"
#include "Core/LLTypes.h"

namespace LLObserverText
{
    inline const TCHAR* const OverviewTitle       = TEXT("LifeLens");
    inline const TCHAR* const ResidentsSuffix     = TEXT("명");
    inline const TCHAR* const StripSeparator      = TEXT("    ");
    inline const TCHAR* const StripNameActionJoin = TEXT(" · ");
    inline const TCHAR* const TapHint             = TEXT("주민을 탭/클릭하면 자세히 볼 수 있습니다");

    inline const TCHAR* const WorldOverviewTitle  = TEXT("세계");
    inline const TCHAR* const PopulationLabel     = TEXT("인구");

    inline const TCHAR* const NowPrefix           = TEXT("현재: ");
    inline const TCHAR* const SummaryJoin         = TEXT(" · ");
    inline const TCHAR* const SummaryAllGood      = TEXT("상태 양호");
    inline const TCHAR* const PersonalityFallback = TEXT("차분함");
    inline const TCHAR* const DetailsHint         = TEXT("상세 보기 ›");

    inline const TCHAR* const TabOverview         = TEXT("개요");
    inline const TCHAR* const TabNeeds            = TEXT("욕구");
    inline const TCHAR* const TabPersonality      = TEXT("성격");
    inline const TCHAR* const TabTraitsSkills     = TEXT("특성·기술");
    inline const TCHAR* const TabEmotion          = TEXT("감정");
    inline const TCHAR* const TabRelationships    = TEXT("관계");
    inline const TCHAR* const TabFamily           = TEXT("가족");
    inline const TCHAR* const TabCivilization     = TEXT("지식·장비");

    inline const TCHAR* const SectionTraits       = TEXT("특성");
    inline const TCHAR* const SectionSkills       = TEXT("기술");
    inline const TCHAR* const SectionLikes        = TEXT("선호");
    inline const TCHAR* const BackgroundPrefix    = TEXT("배경: ");
    inline const TCHAR* const NoneListed          = TEXT("없음");

    inline const TCHAR* const NeedNameHunger      = TEXT("배고픔");
    inline const TCHAR* const NeedNameThirst      = TEXT("목마름");
    inline const TCHAR* const NeedNameEnergy      = TEXT("에너지");
    inline const TCHAR* const NeedNameSocial      = TEXT("사회성");
    inline const TCHAR* const NeedNameHygiene     = TEXT("위생");
    inline const TCHAR* const NeedNameBladder     = TEXT("배뇨");
    inline const TCHAR* const NeedNameFun         = TEXT("즐거움");

    inline const TCHAR* const AxisExtraversion       = TEXT("외향성");
    inline const TCHAR* const AxisAgreeableness      = TEXT("친화성");
    inline const TCHAR* const AxisConscientiousness  = TEXT("성실성");
    inline const TCHAR* const AxisOpenness           = TEXT("개방성");
    inline const TCHAR* const AxisStability          = TEXT("정서 안정성");
    inline const TCHAR* const AxisBalanced           = TEXT("균형형");

    inline const TCHAR* const SexMale             = TEXT("남성");
    inline const TCHAR* const SexFemale           = TEXT("여성");
    inline const TCHAR* const StageInfant         = TEXT("영아");
    inline const TCHAR* const StageChild          = TEXT("아동");
    inline const TCHAR* const StageTeen           = TEXT("청소년");
    inline const TCHAR* const StageAdult          = TEXT("성인");
    inline const TCHAR* const StageElder          = TEXT("노년");

    inline const TCHAR* const SkillExpert         = TEXT("숙련");
    inline const TCHAR* const SkillCapable        = TEXT("능숙");
    inline const TCHAR* const SkillNovice         = TEXT("초보");
    inline const TCHAR* const SkillBeginner       = TEXT("입문");

    inline const TCHAR* const ActionEat           = TEXT("식사 중");
    inline const TCHAR* const ActionDrink         = TEXT("물 마시는 중");
    inline const TCHAR* const ActionSleep         = TEXT("수면 중");
    inline const TCHAR* const ActionSocialize     = TEXT("대화 중");
    inline const TCHAR* const ActionHygiene       = TEXT("씻는 중");
    inline const TCHAR* const ActionToilet        = TEXT("용변 보는 중");
    inline const TCHAR* const ActionHaveFun       = TEXT("여가 중");
    inline const TCHAR* const ActionIdle          = TEXT("대기 중");

    inline const TCHAR* const NeedGood            = TEXT("좋음");
    inline const TCHAR* const NeedFine            = TEXT("보통");
    inline const TCHAR* const NeedLow             = TEXT("낮음");
    inline const TCHAR* const NeedVeryLow         = TEXT("매우 낮음");

    inline const TCHAR* const HungerPhrases[2]    = { TEXT("배고픔"),       TEXT("매우 배고픔") };
    inline const TCHAR* const ThirstPhrases[2]    = { TEXT("목마름"),       TEXT("매우 목마름") };
    inline const TCHAR* const EnergyPhrases[2]    = { TEXT("조금 피곤함"),   TEXT("매우 지침") };
    inline const TCHAR* const SocialPhrases[2]    = { TEXT("외로움"),       TEXT("매우 외로움") };
    inline const TCHAR* const HygienePhrases[2]   = { TEXT("씻을 필요 있음"), TEXT("위생 상태가 나쁨") };
    inline const TCHAR* const BladderPhrases[2]   = { TEXT("화장실 필요"),   TEXT("화장실이 매우 급함") };
    inline const TCHAR* const FunPhrases[2]       = { TEXT("지루함"),       TEXT("매우 지루함") };

    inline const TCHAR* const ExtraversionWords[2]      = { TEXT("사교적"), TEXT("내향적") };
    inline const TCHAR* const AgreeablenessWords[2]     = { TEXT("다정함"), TEXT("직설적") };
    inline const TCHAR* const ConscientiousnessWords[2] = { TEXT("꼼꼼함"), TEXT("느긋함") };
    inline const TCHAR* const OpennessWords[2]          = { TEXT("호기심 많음"), TEXT("전통적") };
    inline const TCHAR* const StabilityWords[2]         = { TEXT("안정적"), TEXT("예민함") };
}

namespace LLObserverLabels
{
    constexpr float NeedGoodMin = 70.0f;
    constexpr float NeedFineMin = 40.0f;
    constexpr float NeedLowMin  = 20.0f;

    constexpr float PersonalityHighMin = 65.0f;
    constexpr float PersonalityLowMax  = 35.0f;

    constexpr float SkillExpertMin  = 70.0f;
    constexpr float SkillCapableMin = 40.0f;
    constexpr float SkillNoviceMin  = 20.0f;

    constexpr int32 MaxSummaryPhrases   = 3;
    constexpr int32 MaxPersonalityWords = 3;

    struct FNeedRow
    {
        const TCHAR* Name;
        float Value;
    };

    inline TArray<FNeedRow> NeedRows(const FLLNeedState& Needs)
    {
        return {
            { LLObserverText::NeedNameHunger,  Needs.Hunger },
            { LLObserverText::NeedNameThirst,  Needs.Thirst },
            { LLObserverText::NeedNameEnergy,  Needs.Energy },
            { LLObserverText::NeedNameHygiene, Needs.Hygiene },
            { LLObserverText::NeedNameBladder, Needs.Bladder },
        };
    }

    struct FPersonalityAxisRow
    {
        const TCHAR* Name;
        float Value;
        const TCHAR* const* Words;
    };

    inline TArray<FPersonalityAxisRow> PersonalityAxisRows(const FLLPersonality& P)
    {
        return {
            { LLObserverText::AxisExtraversion,      P.Extraversion,       LLObserverText::ExtraversionWords },
            { LLObserverText::AxisAgreeableness,     P.Agreeableness,      LLObserverText::AgreeablenessWords },
            { LLObserverText::AxisConscientiousness, P.Conscientiousness,  LLObserverText::ConscientiousnessWords },
            { LLObserverText::AxisOpenness,          P.Openness,           LLObserverText::OpennessWords },
            { LLObserverText::AxisStability,         P.EmotionalStability, LLObserverText::StabilityWords },
        };
    }

    enum class ENeedLevel : uint8
    {
        Good,
        Fine,
        Low,
        VeryLow
    };

    inline ENeedLevel NeedLevel(float Value)
    {
        if (Value >= NeedGoodMin) return ENeedLevel::Good;
        if (Value >= NeedFineMin) return ENeedLevel::Fine;
        if (Value >= NeedLowMin)  return ENeedLevel::Low;
        return ENeedLevel::VeryLow;
    }

    inline FString NeedLabel(float Value)
    {
        switch (NeedLevel(Value))
        {
            case ENeedLevel::Good:    return LLObserverText::NeedGood;
            case ENeedLevel::Fine:    return LLObserverText::NeedFine;
            case ENeedLevel::Low:     return LLObserverText::NeedLow;
            case ENeedLevel::VeryLow:
            default:                  return LLObserverText::NeedVeryLow;
        }
    }

    inline FLinearColor NeedColorForLevel(ENeedLevel Level)
    {
        switch (Level)
        {
            case ENeedLevel::Low:     return FLinearColor(1.0f, 0.72f, 0.35f, 1.0f);
            case ENeedLevel::VeryLow: return FLinearColor(1.0f, 0.42f, 0.38f, 1.0f);
            case ENeedLevel::Good:
            case ENeedLevel::Fine:
            default:                  return FLinearColor(0.90f, 0.92f, 0.95f, 1.0f);
        }
    }

    inline FLinearColor NeedColor(float Value)
    {
        return NeedColorForLevel(NeedLevel(Value));
    }

    inline FString PersonalityAxisLabel(float Value, const TCHAR* const* Words)
    {
        if (Value >= PersonalityHighMin) return Words[0];
        if (Value <= PersonalityLowMax)  return Words[1];
        return LLObserverText::AxisBalanced;
    }

    inline FString SkillLabel(float Value)
    {
        if (Value >= SkillExpertMin)  return LLObserverText::SkillExpert;
        if (Value >= SkillCapableMin) return LLObserverText::SkillCapable;
        if (Value >= SkillNoviceMin)  return LLObserverText::SkillNovice;
        return LLObserverText::SkillBeginner;
    }

    inline FString SexToString(ELLSex Sex)
    {
        return Sex == ELLSex::Female ? LLObserverText::SexFemale : LLObserverText::SexMale;
    }

    inline FString LifeStageToString(ELLLifeStage Stage)
    {
        switch (Stage)
        {
            case ELLLifeStage::Infant: return LLObserverText::StageInfant;
            case ELLLifeStage::Child:  return LLObserverText::StageChild;
            case ELLLifeStage::Teen:   return LLObserverText::StageTeen;
            case ELLLifeStage::Elder:  return LLObserverText::StageElder;
            case ELLLifeStage::Adult:
            default:                   return LLObserverText::StageAdult;
        }
    }

    inline FString JoinNames(const TArray<FName>& Names)
    {
        TArray<FString> Parts;
        Parts.Reserve(Names.Num());
        for (const FName& Name : Names)
        {
            if (!Name.IsNone())
            {
                Parts.Add(Name.ToString());
            }
        }
        return Parts.Num() > 0 ? FString::Join(Parts, LLObserverText::SummaryJoin) : FString(LLObserverText::NoneListed);
    }

    inline FString IntentToString(ELLActionIntent Intent)
    {
        switch (Intent)
        {
            case ELLActionIntent::Eat:       return LLObserverText::ActionEat;
            case ELLActionIntent::Drink:     return LLObserverText::ActionDrink;
            case ELLActionIntent::Sleep:     return LLObserverText::ActionSleep;
            case ELLActionIntent::Socialize: return LLObserverText::ActionSocialize;
            case ELLActionIntent::Hygiene:   return LLObserverText::ActionHygiene;
            case ELLActionIntent::Toilet:    return LLObserverText::ActionToilet;
            case ELLActionIntent::HaveFun:   return LLObserverText::ActionHaveFun;
            case ELLActionIntent::Idle:
            default:                         return LLObserverText::ActionIdle;
        }
    }

    inline FString StatusSummary(const FLLNeedState& Needs, ENeedLevel& OutWorstLevel)
    {
        struct FEntry
        {
            float Value;
            const TCHAR* const* Phrases;
        };
        const FEntry Entries[] = {
            { Needs.Hunger,  LLObserverText::HungerPhrases },
            { Needs.Thirst,  LLObserverText::ThirstPhrases },
            { Needs.Energy,  LLObserverText::EnergyPhrases },
            { Needs.Bladder, LLObserverText::BladderPhrases },
            { Needs.Hygiene, LLObserverText::HygienePhrases },
        };

        TArray<FEntry> Sorted(Entries, static_cast<int32>(UE_ARRAY_COUNT(Entries)));
        Sorted.Sort([](const FEntry& A, const FEntry& B) { return A.Value < B.Value; });

        OutWorstLevel = ENeedLevel::Good;
        TArray<FString> Phrases;
        for (const FEntry& Entry : Sorted)
        {
            const ENeedLevel Level = NeedLevel(Entry.Value);
            if (Level == ENeedLevel::Good || Level == ENeedLevel::Fine)
            {
                break;
            }
            if (Phrases.Num() == 0)
            {
                OutWorstLevel = Level;
            }
            Phrases.Add(FString(Entry.Phrases[Level == ENeedLevel::VeryLow ? 1 : 0]));
            if (Phrases.Num() >= MaxSummaryPhrases)
            {
                break;
            }
        }

        if (Phrases.Num() == 0)
        {
            return LLObserverText::SummaryAllGood;
        }
        return FString::Join(Phrases, LLObserverText::SummaryJoin);
    }

    inline FString StatusSummary(const FLLNeedState& Needs)
    {
        ENeedLevel Unused;
        return StatusSummary(Needs, Unused);
    }

    inline FString PersonalityWords(const FLLResidentData& Resident)
    {
        struct FAxis
        {
            float Value;
            const TCHAR* const* Words;
        };
        const FLLPersonality& P = Resident.Personality;
        const FAxis Axes[] = {
            { P.Extraversion,       LLObserverText::ExtraversionWords },
            { P.Agreeableness,      LLObserverText::AgreeablenessWords },
            { P.Conscientiousness,  LLObserverText::ConscientiousnessWords },
            { P.Openness,           LLObserverText::OpennessWords },
            { P.EmotionalStability, LLObserverText::StabilityWords },
        };

        TArray<FAxis> Sorted(Axes, static_cast<int32>(UE_ARRAY_COUNT(Axes)));
        Sorted.Sort([](const FAxis& A, const FAxis& B)
        {
            return FMath::Abs(A.Value - 50.0f) > FMath::Abs(B.Value - 50.0f);
        });

        TArray<FString> Words;
        for (const FAxis& Axis : Sorted)
        {
            if (Words.Num() >= MaxPersonalityWords)
            {
                break;
            }
            if (Axis.Value >= PersonalityHighMin)
            {
                Words.Add(FString(Axis.Words[0]));
            }
            else if (Axis.Value <= PersonalityLowMax)
            {
                Words.Add(FString(Axis.Words[1]));
            }
        }

        for (const FName& Trait : Resident.Traits)
        {
            if (Words.Num() >= MaxPersonalityWords)
            {
                break;
            }
            const FString TraitText = Trait.ToString();
            if (!TraitText.IsEmpty() && !Words.Contains(TraitText))
            {
                Words.Add(TraitText);
            }
        }

        if (Words.Num() == 0)
        {
            return LLObserverText::PersonalityFallback;
        }
        return FString::Join(Words, LLObserverText::SummaryJoin);
    }
}
