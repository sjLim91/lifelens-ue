#pragma once

// Observer-facing text and colour helpers.
//
// Every user-visible string the observer HUD draws lives in this header so the
// wording (and, later, the language) can be changed in one place. The helpers
// are pure functions over LifeLens read-only data types; they never touch the
// simulation.

#include "CoreMinimal.h"
#include "Core/LLTypes.h"

namespace LLObserverText
{
    // ---- LEVEL 0 overview -------------------------------------------------
    inline const TCHAR* const OverviewTitle      = TEXT("LifeLens");
    inline const TCHAR* const ResidentsSuffix    = TEXT("residents");
    inline const TCHAR* const StripSeparator     = TEXT("    ");
    inline const TCHAR* const StripNameActionJoin = TEXT(" · "); // "name · action"
    // Tools/validate_bootstrap.py (owned by the simulation side) asserts this
    // exact sentence is present in LLObserverHUD.cpp. Keep the wording in sync
    // with the note there until the check is moved to this header.
    inline const TCHAR* const TapHint            = TEXT("Tap/click a resident for details");

    // ---- World overview (SPEC 61) -----------------------------------------
    inline const TCHAR* const WorldOverviewTitle = TEXT("World");
    inline const TCHAR* const PopulationLabel    = TEXT("Population");

    // ---- LEVEL 1 quick inspector -------------------------------------------
    inline const TCHAR* const NowPrefix          = TEXT("Now: ");
    inline const TCHAR* const SummaryJoin        = TEXT(" · ");
    inline const TCHAR* const SummaryAllGood     = TEXT("Doing well");
    inline const TCHAR* const PersonalityFallback = TEXT("Even-tempered");
    inline const TCHAR* const DetailsHint        = TEXT("Details ›"); // "Details ›"

    // ---- LEVEL 2 detail panel ------------------------------------------------
    inline const TCHAR* const TabOverview        = TEXT("Overview");
    inline const TCHAR* const TabNeeds           = TEXT("Needs");
    inline const TCHAR* const TabPersonality     = TEXT("Personality");
    inline const TCHAR* const TabTraitsSkills    = TEXT("Traits & Skills");
    inline const TCHAR* const TabEmotion         = TEXT("Emotion");
    inline const TCHAR* const TabRelationships   = TEXT("Relationships");
    inline const TCHAR* const TabFamily          = TEXT("Family");
    inline const TCHAR* const TabCivilization    = TEXT("Knowledge & Gear");

    inline const TCHAR* const SectionTraits      = TEXT("Traits");
    inline const TCHAR* const SectionSkills      = TEXT("Skills");
    inline const TCHAR* const SectionLikes       = TEXT("Likes");
    inline const TCHAR* const BackgroundPrefix   = TEXT("Background: ");
    inline const TCHAR* const NoneListed         = TEXT("None listed");

    inline const TCHAR* const NeedNameHunger     = TEXT("Hunger");
    inline const TCHAR* const NeedNameThirst     = TEXT("Thirst");
    inline const TCHAR* const NeedNameEnergy     = TEXT("Energy");
    inline const TCHAR* const NeedNameSocial     = TEXT("Social");
    inline const TCHAR* const NeedNameHygiene    = TEXT("Hygiene");
    inline const TCHAR* const NeedNameBladder    = TEXT("Bladder");
    inline const TCHAR* const NeedNameFun        = TEXT("Fun");

    inline const TCHAR* const AxisExtraversion      = TEXT("Extraversion");
    inline const TCHAR* const AxisAgreeableness     = TEXT("Agreeableness");
    inline const TCHAR* const AxisConscientiousness = TEXT("Conscientiousness");
    inline const TCHAR* const AxisOpenness          = TEXT("Openness");
    inline const TCHAR* const AxisStability         = TEXT("Emotional stability");
    inline const TCHAR* const AxisBalanced          = TEXT("Balanced");

    inline const TCHAR* const SexMale            = TEXT("Male");
    inline const TCHAR* const SexFemale          = TEXT("Female");
    inline const TCHAR* const StageInfant        = TEXT("Infant");
    inline const TCHAR* const StageChild         = TEXT("Child");
    inline const TCHAR* const StageTeen          = TEXT("Teen");
    inline const TCHAR* const StageAdult         = TEXT("Adult");
    inline const TCHAR* const StageElder         = TEXT("Elder");

    // Skill levels (value is 0..100).
    inline const TCHAR* const SkillExpert        = TEXT("Skilled");
    inline const TCHAR* const SkillCapable       = TEXT("Capable");
    inline const TCHAR* const SkillNovice        = TEXT("Novice");
    inline const TCHAR* const SkillBeginner      = TEXT("Beginner");

    // ---- Action intent -----------------------------------------------------
    inline const TCHAR* const ActionEat          = TEXT("Eating");
    inline const TCHAR* const ActionDrink        = TEXT("Drinking");
    inline const TCHAR* const ActionSleep        = TEXT("Sleeping");
    inline const TCHAR* const ActionSocialize    = TEXT("Socializing");
    inline const TCHAR* const ActionHygiene      = TEXT("Washing");
    inline const TCHAR* const ActionToilet       = TEXT("Toilet");
    inline const TCHAR* const ActionHaveFun      = TEXT("Leisure");
    inline const TCHAR* const ActionIdle         = TEXT("Idle");

    // ---- Need levels (value is 0..100, higher = more satisfied) -----------
    inline const TCHAR* const NeedGood           = TEXT("Good");
    inline const TCHAR* const NeedFine           = TEXT("Fine");
    inline const TCHAR* const NeedLow            = TEXT("Low");
    inline const TCHAR* const NeedVeryLow        = TEXT("Very low");

    // Status summary phrases, [0] = Low, [1] = Very low.
    inline const TCHAR* const HungerPhrases[2]   = { TEXT("Hungry"),           TEXT("Starving") };
    inline const TCHAR* const ThirstPhrases[2]   = { TEXT("Thirsty"),          TEXT("Very thirsty") };
    inline const TCHAR* const EnergyPhrases[2]   = { TEXT("A bit tired"),      TEXT("Exhausted") };
    inline const TCHAR* const SocialPhrases[2]   = { TEXT("Lonely"),           TEXT("Very lonely") };
    inline const TCHAR* const HygienePhrases[2]  = { TEXT("Needs a wash"),     TEXT("Really needs a wash") };
    inline const TCHAR* const BladderPhrases[2]  = { TEXT("Needs the toilet"), TEXT("Desperate for the toilet") };
    inline const TCHAR* const FunPhrases[2]      = { TEXT("Bored"),            TEXT("Very bored") };

    // Personality words, [0] = high end of the axis, [1] = low end.
    inline const TCHAR* const ExtraversionWords[2]      = { TEXT("Outgoing"),  TEXT("Reserved") };
    inline const TCHAR* const AgreeablenessWords[2]     = { TEXT("Warm"),      TEXT("Blunt") };
    inline const TCHAR* const ConscientiousnessWords[2] = { TEXT("Careful"),   TEXT("Easygoing") };
    inline const TCHAR* const OpennessWords[2]          = { TEXT("Curious"),   TEXT("Traditional") };
    inline const TCHAR* const StabilityWords[2]         = { TEXT("Steady"),    TEXT("Sensitive") };
}

namespace LLObserverLabels
{
    // Need thresholds (inclusive lower bounds). 0..100, higher = satisfied.
    constexpr float NeedGoodMin = 70.0f;
    constexpr float NeedFineMin = 40.0f;
    constexpr float NeedLowMin  = 20.0f;

    // Personality axis thresholds. 0..100, 50 = neutral.
    constexpr float PersonalityHighMin = 65.0f;
    constexpr float PersonalityLowMax  = 35.0f;

    // Skill thresholds (inclusive lower bounds). 0..100.
    constexpr float SkillExpertMin  = 70.0f;
    constexpr float SkillCapableMin = 40.0f;
    constexpr float SkillNoviceMin  = 20.0f;

    constexpr int32 MaxSummaryPhrases   = 3;
    constexpr int32 MaxPersonalityWords = 3;

    // A named need with its value, for tabular display. Only authoritative Core
    // physical needs belong here. Social state is relationship/emotion driven
    // and Fun is not a Core need, so compatibility placeholders are excluded.
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

    // A named personality axis with its value and high/low word pair.
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

    // "Good" / "Fine" / "Low" / "Very low"
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
            case ENeedLevel::Low:     return FLinearColor(1.0f, 0.72f, 0.35f, 1.0f); // orange
            case ENeedLevel::VeryLow: return FLinearColor(1.0f, 0.42f, 0.38f, 1.0f); // red
            case ENeedLevel::Good:
            case ENeedLevel::Fine:
            default:                  return FLinearColor(0.90f, 0.92f, 0.95f, 1.0f); // neutral
        }
    }

    inline FLinearColor NeedColor(float Value)
    {
        return NeedColorForLevel(NeedLevel(Value));
    }

    // High word / low word / "Balanced" for one personality axis.
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

    // "Alpha · Beta · Gamma" from a name list, or NoneListed.
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

    // One human-readable line: the lowest authoritative physical needs first,
    // at most MaxSummaryPhrases. Compatibility-only Social/Fun placeholders
    // must never generate observer claims such as "Lonely" or "Bored".
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
                break; // sorted ascending, nothing lower remains
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

    // Up to MaxPersonalityWords words: the strongest personality axes first,
    // then generated Traits to fill, then a neutral fallback.
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
