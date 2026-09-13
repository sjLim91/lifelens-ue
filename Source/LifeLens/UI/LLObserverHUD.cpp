#include "UI/LLObserverHUD.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverLabels.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Characters/LLResidentCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

namespace
{
    // Layout constants, in unscaled pixels.
    constexpr float Margin           = 16.0f;
    constexpr float PadX             = 12.0f;
    constexpr float PadY             = 8.0f;
    constexpr float LineGap          = 6.0f;
    constexpr float PanelMaxWidth    = 360.0f;
    constexpr float PanelMinWidth    = 200.0f;
    constexpr float PanelWidthRatio  = 0.42f;

    constexpr float OverviewAlpha    = 0.22f;
    constexpr float InspectorAlpha   = 0.45f;

    const FLinearColor TextPrimary   (1.00f, 1.00f, 1.00f, 1.0f);
    const FLinearColor TextSecondary (0.88f, 0.90f, 0.94f, 1.0f);
    const FLinearColor TextMuted     (0.80f, 0.84f, 0.90f, 0.55f);
    const FLinearColor TextHint      (0.80f, 0.84f, 0.90f, 0.35f);
    const FLinearColor TextAction    (0.75f, 0.90f, 1.00f, 1.0f);

    UFont* HUDFont()
    {
        return GEngine ? GEngine->GetSmallFont() : nullptr;
    }

    ALLResidentCharacter* FindResidentActor(UWorld* World, FGuid ResidentId)
    {
        if (!World)
        {
            return nullptr;
        }

        for (TActorIterator<ALLResidentCharacter> It(World); It; ++It)
        {
            if (It->GetResidentId() == ResidentId)
            {
                return *It;
            }
        }
        return nullptr;
    }
}

float ALLObserverHUD::ComputeUIScale() const
{
    if (!Canvas)
    {
        return 1.0f;
    }
    const float ShortSide = FMath::Min(Canvas->ClipX, Canvas->ClipY);
    return FMath::Clamp(ShortSide / 540.0f, 1.0f, 2.5f);
}

FString ALLObserverHUD::CurrentActionFor(const FLLResidentData& Resident) const
{
    const ALLResidentCharacter* Actor = FindResidentActor(GetWorld(), Resident.ResidentId);
    return Actor ? LLObserverLabels::IntentToString(Actor->GetCurrentIntent()) : FString();
}

void ALLObserverHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !GetWorld())
    {
        return;
    }

    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    ULLSimulationSubsystem* Simulation = GameInstance ? GameInstance->GetSubsystem<ULLSimulationSubsystem>() : nullptr;
    ULLObservationSubsystem* Observation = GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (!Simulation)
    {
        return;
    }

    const float UIScale = ComputeUIScale();
    const TArray<FLLResidentData> Residents = Simulation->GetResidents();

    FLLResidentData Selected;
    const bool bHasSelection = Observation && Observation->HasObservedResident()
        && Simulation->FindResidentById(Observation->GetObservedResidentId(), Selected);

    // LEVEL 0 is always drawn; it is deliberately thin.
    const float OverviewBottom = DrawOverview(*Simulation, Residents, UIScale, !bHasSelection);

    // LEVEL 1 only when a resident is selected.
    if (bHasSelection)
    {
        DrawQuickInspector(Selected, UIScale, OverviewBottom);
    }
}

TArray<FString> ALLObserverHUD::WrapText(const FString& Text, float MaxWidth, float Scale) const
{
    TArray<FString> Lines;
    UFont* Font = HUDFont();

    TArray<FString> Words;
    Text.ParseIntoArray(Words, TEXT(" "), true);
    if (Words.Num() == 0)
    {
        Lines.Add(Text);
        return Lines;
    }

    FString Current;
    for (const FString& Word : Words)
    {
        const FString Candidate = Current.IsEmpty() ? Word : Current + TEXT(" ") + Word;
        float W = 0.0f, H = 0.0f;
        GetTextSize(Candidate, W, H, Font, Scale);
        if (W <= MaxWidth || Current.IsEmpty())
        {
            Current = Candidate;
        }
        else
        {
            Lines.Add(Current);
            Current = Word;
        }
    }
    if (!Current.IsEmpty())
    {
        Lines.Add(Current);
    }
    return Lines;
}

float ALLObserverHUD::DrawOverview(const ULLSimulationSubsystem& Simulation, const TArray<FLLResidentData>& Residents, float UIScale, bool bShowHint)
{
    UFont* Font = HUDFont();

    const int64 TotalMinutes = Simulation.GetSimulationMinute();
    const int64 Day = TotalMinutes / 1440 + 1;
    const int32 MinuteOfDay = static_cast<int32>(TotalMinutes % 1440);
    const int32 Hour = MinuteOfDay / 60;
    const int32 Minute = MinuteOfDay % 60;

    const float TitleScale = 1.0f * UIScale;
    const float StripScale = 0.85f * UIScale;

    const FString StatusLine = FString::Printf(TEXT("%s   Day %lld  %02d:%02d   %d %s"),
        LLObserverText::OverviewTitle, static_cast<long long>(Day), Hour, Minute, Residents.Num(), LLObserverText::ResidentsSuffix);

    // Build the resident strip: "name · action" items, one line.
    TArray<FString> StripItems;
    StripItems.Reserve(Residents.Num());
    for (const FLLResidentData& Resident : Residents)
    {
        const FString Action = CurrentActionFor(Resident);
        StripItems.Add(Action.IsEmpty()
            ? Resident.DisplayName
            : Resident.DisplayName + LLObserverText::StripNameActionJoin + Action);
    }
    const FString StripLine = FString::Join(StripItems, LLObserverText::StripSeparator);
    // The hint text lives in LLObserverLabels.h as LLObserverText::TapHint.
    // Tools/validate_bootstrap.py still greps this file for the literal
    // "Tap/click a resident for details"; keep the two in sync until that
    // check is updated to look at the labels header.
    const FString HintLine(LLObserverText::TapHint);

    float StatusW = 0.0f, StatusH = 0.0f;
    float StripW = 0.0f, StripH = 0.0f;
    float HintW = 0.0f, HintH = 0.0f;
    GetTextSize(StatusLine, StatusW, StatusH, Font, TitleScale);
    GetTextSize(StripLine, StripW, StripH, Font, StripScale);
    GetTextSize(HintLine, HintW, HintH, Font, StripScale);

    const float X = Margin * UIScale;
    const float Pad = PadY * UIScale;
    const float Gap = LineGap * UIScale;
    const float BandHeight = Pad + StatusH + Gap + StripH + Pad;

    // Full-width translucent band from the top edge; the world stays visible underneath.
    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, OverviewAlpha), 0.0f, 0.0f, Canvas->ClipX, BandHeight);

    float CursorY = Pad;
    DrawText(StatusLine, TextPrimary, X, CursorY, Font, TitleScale, false);
    CursorY += StatusH + Gap;

    DrawText(StripLine, TextMuted, X, CursorY, Font, StripScale, false);

    // Hint after the strip: LEVEL 0 only, and only if it fits on the same line.
    const float HintX = X + StripW + 3.0f * PadX * UIScale;
    if (bShowHint && HintX + HintW <= Canvas->ClipX - X)
    {
        DrawText(HintLine, TextHint, HintX, CursorY, Font, StripScale, false);
    }

    return BandHeight;
}

void ALLObserverHUD::DrawQuickInspector(const FLLResidentData& Resident, float UIScale, float TopY)
{
    UFont* Font = HUDFont();

    const float NameScale     = 1.15f * UIScale;
    const float BodyScale     = 0.95f * UIScale;
    const float SummaryScale  = 0.90f * UIScale;
    const float WordsScale    = 0.85f * UIScale;

    // Lines, top to bottom.
    const FString NameLine = FString::Printf(TEXT("%s   %d"), *Resident.DisplayName, Resident.AgeYears);

    const FString Action = CurrentActionFor(Resident);
    const FString NowLine = Action.IsEmpty() ? FString() : FString(LLObserverText::NowPrefix) + Action;

    LLObserverLabels::ENeedLevel WorstLevel = LLObserverLabels::ENeedLevel::Good;
    const FString SummaryLine = LLObserverLabels::StatusSummary(Resident.Needs, WorstLevel);
    const FLinearColor SummaryColor = LLObserverLabels::NeedColorForLevel(WorstLevel);

    const FString WordsLine = LLObserverLabels::PersonalityWords(Resident);

    struct FEntry
    {
        FString Text;
        FLinearColor Color;
        float Scale;
    };
    TArray<FEntry> Entries;
    Entries.Add({ NameLine, TextPrimary, NameScale });
    if (!NowLine.IsEmpty())
    {
        Entries.Add({ NowLine, TextAction, BodyScale });
    }
    Entries.Add({ SummaryLine, SummaryColor, SummaryScale });
    Entries.Add({ WordsLine, TextSecondary, WordsScale });

    // Panel width follows the measured content, bounded by the viewport so the
    // card never covers more than PanelWidthRatio of the screen. Anything wider
    // than that is word-wrapped below.
    const float Gap = LineGap * UIScale;
    const float Pad = PadY * UIScale;
    const float InnerPadX = PadX * UIScale;
    const float MaxPanelWidth = FMath::Min(PanelMaxWidth * UIScale, Canvas->ClipX * PanelWidthRatio);
    const float MinPanelWidth = FMath::Min(PanelMinWidth * UIScale, MaxPanelWidth);

    float WidestLine = 0.0f;
    for (const FEntry& Entry : Entries)
    {
        float W = 0.0f, H = 0.0f;
        GetTextSize(Entry.Text, W, H, Font, Entry.Scale);
        WidestLine = FMath::Max(WidestLine, W);
    }
    const float PanelWidth = FMath::Clamp(WidestLine + 2.0f * InnerPadX, MinPanelWidth, MaxPanelWidth);
    const float TextMaxWidth = PanelWidth - 2.0f * InnerPadX;

    struct FLine
    {
        FString Text;
        FLinearColor Color;
        float Scale;
        float Height;
    };
    TArray<FLine> Lines;
    float ContentHeight = 0.0f;
    for (const FEntry& Entry : Entries)
    {
        for (const FString& Wrapped : WrapText(Entry.Text, TextMaxWidth, Entry.Scale))
        {
            float W = 0.0f, H = 0.0f;
            GetTextSize(Wrapped, W, H, Font, Entry.Scale);
            Lines.Add({ Wrapped, Entry.Color, Entry.Scale, H });
            ContentHeight += H;
        }
    }
    ContentHeight += Gap * FMath::Max(0, Lines.Num() - 1);

    const float PanelHeight = ContentHeight + 2.0f * Pad;
    const float PanelX = FMath::Max(Margin * UIScale, Canvas->ClipX - PanelWidth - Margin * UIScale);
    const float PanelY = TopY + Margin * UIScale;

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, InspectorAlpha), PanelX, PanelY, PanelWidth, PanelHeight);

    float CursorY = PanelY + Pad;
    const float TextX = PanelX + InnerPadX;
    for (const FLine& Line : Lines)
    {
        DrawText(Line.Text, Line.Color, TextX, CursorY, Font, Line.Scale, false);
        CursorY += Line.Height + Gap;
    }
}
