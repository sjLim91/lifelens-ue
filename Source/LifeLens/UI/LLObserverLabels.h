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

    // ---- LEVEL 1 quick inspector -------------------------------------------
    inline const TCHAR* const NowPrefix          = TEXT("Now: ");
    inline const TCHAR* const SummaryJoin        = TEXT(" · ");
    inline const TCHAR* const SummaryAllGood     = TEXT("Doing well");
    inline const TCHAR* const PersonalityFallback = TEXT("Even-tempered");

    // ---- Action intent -----------------------------------------------------
    inline const TCHAR* const ActionEat          = TEXT("Eating");
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

    constexpr int32 MaxSummaryPhrases   = 3;
    constexpr int32 MaxPersonalityWords = 3;

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

    inline FString IntentToString(ELLActionIntent Intent)
    {
        switch (Intent)
        {
            case ELLActionIntent::Eat:       return LLObserverText::ActionEat;
            case ELLActionIntent::Sleep:     return LLObserverText::ActionSleep;
            case ELLActionIntent::Socialize: return LLObserverText::ActionSocialize;
            case ELLActionIntent::Hygiene:   return LLObserverText::ActionHygiene;
            case ELLActionIntent::Toilet:    return LLObserverText::ActionToilet;
            case ELLActionIntent::HaveFun:   return LLObserverText::ActionHaveFun;
            case ELLActionIntent::Idle:
            default:                         return LLObserverText::ActionIdle;
        }
    }

    // One human-readable line: the lowest needs first, at most MaxSummaryPhrases.
    // Also reports the worst level so the caller can colour the line.
    inline FString StatusSummary(const FLLNeedState& Needs, ENeedLevel& OutWorstLevel)
    {
        struct FEntry
        {
            float Value;
            const TCHAR* const* Phrases;
        };
        const FEntry Entries[] = {
            { Needs.Hunger,  LLObserverText::HungerPhrases },
            { Needs.Energy,  LLObserverText::EnergyPhrases },
            { Needs.Bladder, LLObserverText::BladderPhrases },
            { Needs.Hygiene, LLObserverText::HygienePhrases },
            { Needs.Social,  LLObserverText::SocialPhrases },
            { Needs.Fun,     LLObserverText::FunPhrases },
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
