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

static_assert(static_cast<int32>(ELLDetailTab::Count) == ALLObserverHUD::DetailTabCount, "DetailTabCount must match ELLDetailTab::Count");

namespace
{
    // Layout constants, in unscaled pixels.
    constexpr float Margin           = 16.0f;
    constexpr float PadX             = 12.0f;
    constexpr float PadY             = 8.0f;
    constexpr float LineGap          = 6.0f;
    constexpr float SectionGap       = 12.0f;

    constexpr float PanelMaxWidth    = 360.0f;
    constexpr float PanelMinWidth    = 200.0f;
    constexpr float PanelWidthRatio  = 0.42f;

    constexpr float DetailMaxWidth   = 520.0f;
    constexpr float DetailMinWidth   = 280.0f;
    constexpr float DetailWidthRatio = 0.60f;
    constexpr float TabPadX          = 10.0f;
    constexpr float TabPadY          = 6.0f;
    constexpr float TabUnderline     = 2.0f;

    constexpr float OverviewAlpha    = 0.22f;
    constexpr float InspectorAlpha   = 0.45f;
    constexpr float DetailAlpha      = 0.60f;

    const FLinearColor TextPrimary   (1.00f, 1.00f, 1.00f, 1.0f);
    const FLinearColor TextSecondary (0.88f, 0.90f, 0.94f, 1.0f);
    const FLinearColor TextMuted     (0.80f, 0.84f, 0.90f, 0.55f);
    const FLinearColor TextHint      (0.80f, 0.84f, 0.90f, 0.35f);
    const FLinearColor TextAction    (0.75f, 0.90f, 1.00f, 1.0f);
    const FLinearColor TextSection   (0.70f, 0.78f, 0.90f, 0.85f);
    const FLinearColor TabActiveLine (0.75f, 0.90f, 1.00f, 0.9f);

    UFont* HUDFont()
    {
        return GEngine ? GEngine->GetSmallFont() : nullptr;
    }

    bool RectContains(const FBox2D& Rect, const FVector2D& Point)
    {
        return Rect.bIsValid && Rect.IsInside(Point);
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

    const TCHAR* TabTitle(ELLDetailTab Tab)
    {
        switch (Tab)
        {
            case ELLDetailTab::Needs:        return LLObserverText::TabNeeds;
            case ELLDetailTab::Personality:  return LLObserverText::TabPersonality;
            case ELLDetailTab::TraitsSkills: return LLObserverText::TabTraitsSkills;
            case ELLDetailTab::Overview:
            default:                         return LLObserverText::TabOverview;
        }
    }

    // One row of panel content. Right is optional (two-column rows); rows with
    // no Right text are word-wrapped to the panel width.
    struct FRow
    {
        FString Left;
        FLinearColor LeftColor = TextSecondary;
        FString Right;
        FLinearColor RightColor = TextSecondary;
        float Scale = 1.0f;
        float GapBefore = 0.0f; // extra spacing above, unscaled
    };
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float ALLObserverHUD::ComputeUIScale() const
{
    if (!Canvas)
    {
        return 1.0f;
    }
    const float ShortSide = FMath::Min(Canvas->ClipX, Canvas->ClipY);
    return FMath::Clamp(ShortSide / 540.0f, 1.0f, 2.5f);
}

ULLObservationSubsystem* ALLObserverHUD::GetObservation() const
{
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
}

FString ALLObserverHUD::CurrentActionFor(const FLLResidentData& Resident) const
{
    const ALLResidentCharacter* Actor = FindResidentActor(GetWorld(), Resident.ResidentId);
    return Actor ? LLObserverLabels::IntentToString(Actor->GetCurrentIntent()) : FString();
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

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

bool ALLObserverHUD::HandleTap(const FVector2D& ScreenPosition)
{
    ULLObservationSubsystem* Observation = GetObservation();
    if (!Observation)
    {
        return false;
    }

    switch (Observation->GetObservationLevel())
    {
        case ELLObservationLevel::Detail:
            for (int32 Index = 0; Index < DetailTabCount; ++Index)
            {
                if (RectContains(DetailTabRects[Index], ScreenPosition))
                {
                    ActiveTab = static_cast<ELLDetailTab>(Index);
                    return true;
                }
            }
            // Taps inside the panel body are swallowed so the world underneath
            // is not selected through the panel.
            return RectContains(DetailPanelRect, ScreenPosition);

        case ELLObservationLevel::Quick:
            if (RectContains(QuickInspectorRect, ScreenPosition))
            {
                ActiveTab = ELLDetailTab::Overview;
                Observation->OpenDetail();
                return true;
            }
            return false;

        case ELLObservationLevel::World:
        default:
            if (RectContains(OverviewBandRect, ScreenPosition))
            {
                bWorldOverviewOpen = !bWorldOverviewOpen;
                return true;
            }
            if (bWorldOverviewOpen)
            {
                // Inside the panel: swallowed. Outside: close it.
                bWorldOverviewOpen = false;
                return true;
            }
            return false;
    }
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void ALLObserverHUD::DrawHUD()
{
    Super::DrawHUD();

    OverviewBandRect = FBox2D(ForceInit);
    WorldOverviewRect = FBox2D(ForceInit);
    QuickInspectorRect = FBox2D(ForceInit);
    DetailPanelRect = FBox2D(ForceInit);
    for (FBox2D& Rect : DetailTabRects)
    {
        Rect = FBox2D(ForceInit);
    }

    if (!Canvas || !GetWorld())
    {
        return;
    }

    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    ULLSimulationSubsystem* Simulation = GameInstance ? GameInstance->GetSubsystem<ULLSimulationSubsystem>() : nullptr;
    ULLObservationSubsystem* Observation = GetObservation();
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

    if (!bHasSelection)
    {
        if (bWorldOverviewOpen)
        {
            DrawWorldOverview(*Simulation, Residents, UIScale, OverviewBottom);
        }
        return;
    }

    bWorldOverviewOpen = false;

    if (Observation->IsDetailOpen())
    {
        DrawDetailPanel(Selected, UIScale, OverviewBottom);
    }
    else
    {
        DrawQuickInspector(Selected, UIScale, OverviewBottom);
    }
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
    OverviewBandRect = FBox2D(FVector2D(0.0f, 0.0f), FVector2D(Canvas->ClipX, BandHeight));

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

void ALLObserverHUD::DrawWorldOverview(const ULLSimulationSubsystem& Simulation, const TArray<FLLResidentData>& Residents, float UIScale, float TopY)
{
    UFont* Font = HUDFont();

    const float TitleScale = 1.05f * UIScale;
    const float RowScale   = 0.90f * UIScale;
    const float Gap = LineGap * UIScale;
    const float Pad = PadY * UIScale;
    const float InnerPadX = PadX * UIScale;

    const int64 TotalMinutes = Simulation.GetSimulationMinute();
    const int64 Day = TotalMinutes / 1440 + 1;
    const int32 MinuteOfDay = static_cast<int32>(TotalMinutes % 1440);
    const int32 Hour = MinuteOfDay / 60;
    const int32 Minute = MinuteOfDay % 60;

    int32 StageCounts[5] = { 0, 0, 0, 0, 0 };
    for (const FLLResidentData& Resident : Residents)
    {
        const int32 Index = static_cast<int32>(Resident.LifeStage);
        if (Index >= 0 && Index < 5)
        {
            ++StageCounts[Index];
        }
    }

    // Rows: title, day/time, population, one row per life stage.
    TArray<FRow> Rows;
    {
        FRow Title;
        Title.Left = LLObserverText::WorldOverviewTitle;
        Title.LeftColor = TextPrimary;
        Title.Scale = TitleScale;
        Rows.Add(Title);

        FRow Clock;
        Clock.Left = FString::Printf(TEXT("Day %lld  %02d:%02d"), static_cast<long long>(Day), Hour, Minute);
        Clock.LeftColor = TextSecondary;
        Clock.Scale = RowScale;
        Rows.Add(Clock);

        FRow Population;
        Population.Left = LLObserverText::PopulationLabel;
        Population.Right = FString::FromInt(Residents.Num());
        Population.RightColor = TextPrimary;
        Population.Scale = RowScale;
        Population.GapBefore = SectionGap;
        Rows.Add(Population);

        const ELLLifeStage Stages[5] = { ELLLifeStage::Infant, ELLLifeStage::Child, ELLLifeStage::Teen, ELLLifeStage::Adult, ELLLifeStage::Elder };
        for (int32 Index = 0; Index < 5; ++Index)
        {
            FRow Row;
            Row.Left = LLObserverLabels::LifeStageToString(Stages[Index]);
            Row.Right = FString::FromInt(StageCounts[static_cast<int32>(Stages[Index])]);
            Row.RightColor = TextPrimary;
            Row.Scale = RowScale;
            Rows.Add(Row);
        }
    }

    float WidestLeft = 0.0f;
    float WidestRight = 0.0f;
    float ContentHeight = 0.0f;
    TArray<float> Heights;
    for (const FRow& Row : Rows)
    {
        float W = 0.0f, H = 0.0f;
        GetTextSize(Row.Left, W, H, Font, Row.Scale);
        Heights.Add(H);
        ContentHeight += H + Row.GapBefore * UIScale;
        if (!Row.Right.IsEmpty())
        {
            WidestLeft = FMath::Max(WidestLeft, W);
            float RW = 0.0f, RH = 0.0f;
            GetTextSize(Row.Right, RW, RH, Font, Row.Scale);
            WidestRight = FMath::Max(WidestRight, RW);
        }
        else
        {
            WidestLeft = FMath::Max(WidestLeft, W);
        }
    }
    ContentHeight += Gap * FMath::Max(0, Rows.Num() - 1);

    const float RightColumnX = WidestLeft + 2.0f * InnerPadX;
    const float MaxPanelWidth = FMath::Min(PanelMaxWidth * UIScale, Canvas->ClipX * PanelWidthRatio);
    const float PanelWidth = FMath::Clamp(RightColumnX + WidestRight + 2.0f * InnerPadX, FMath::Min(PanelMinWidth * UIScale, MaxPanelWidth), MaxPanelWidth);
    const float PanelHeight = ContentHeight + 2.0f * Pad;
    const float PanelX = Margin * UIScale;
    const float PanelY = TopY + Margin * UIScale;

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, InspectorAlpha), PanelX, PanelY, PanelWidth, PanelHeight);
    WorldOverviewRect = FBox2D(FVector2D(PanelX, PanelY), FVector2D(PanelX + PanelWidth, PanelY + PanelHeight));

    float CursorY = PanelY + Pad;
    const float TextX = PanelX + InnerPadX;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const FRow& Row = Rows[Index];
        CursorY += Row.GapBefore * UIScale;
        DrawText(Row.Left, Row.LeftColor, TextX, CursorY, Font, Row.Scale, false);
        if (!Row.Right.IsEmpty())
        {
            DrawText(Row.Right, Row.RightColor, TextX + RightColumnX, CursorY, Font, Row.Scale, false);
        }
        CursorY += Heights[Index] + Gap;
    }
}

void ALLObserverHUD::DrawQuickInspector(const FLLResidentData& Resident, float UIScale, float TopY)
{
    UFont* Font = HUDFont();

    const float NameScale     = 1.15f * UIScale;
    const float BodyScale     = 0.95f * UIScale;
    const float SummaryScale  = 0.90f * UIScale;
    const float WordsScale    = 0.85f * UIScale;
    const float HintScale     = 0.80f * UIScale;

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
    Entries.Add({ FString(LLObserverText::DetailsHint), TextHint, HintScale });

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
    QuickInspectorRect = FBox2D(FVector2D(PanelX, PanelY), FVector2D(PanelX + PanelWidth, PanelY + PanelHeight));

    float CursorY = PanelY + Pad;
    const float TextX = PanelX + InnerPadX;
    for (const FLine& Line : Lines)
    {
        DrawText(Line.Text, Line.Color, TextX, CursorY, Font, Line.Scale, false);
        CursorY += Line.Height + Gap;
    }
}

void ALLObserverHUD::DrawDetailPanel(const FLLResidentData& Resident, float UIScale, float TopY)
{
    UFont* Font = HUDFont();

    // A different resident always opens on Overview.
    if (LastDetailResidentId != Resident.ResidentId)
    {
        LastDetailResidentId = Resident.ResidentId;
        ActiveTab = ELLDetailTab::Overview;
    }

    const float NameScale    = 1.15f * UIScale;
    const float BodyScale    = 0.95f * UIScale;
    const float RowScale     = 0.90f * UIScale;
    const float SectionScale = 0.85f * UIScale;
    const float TabScale     = 0.90f * UIScale;

    const float Gap = LineGap * UIScale;
    const float Pad = PadY * UIScale;
    const float InnerPadX = PadX * UIScale;

    // ---- Panel geometry ----------------------------------------------------
    // Prefer DetailWidthRatio of the screen (capped), never narrower than the
    // minimum, never wider than the screen minus margins.
    const float AvailableWidth = FMath::Max(0.0f, Canvas->ClipX - 2.0f * Margin * UIScale);
    const float PreferredWidth = FMath::Min(DetailMaxWidth * UIScale, Canvas->ClipX * DetailWidthRatio);
    const float PanelWidth = FMath::Min(AvailableWidth, FMath::Max(PreferredWidth, DetailMinWidth * UIScale));
    const float PanelX = FMath::Max(Margin * UIScale, Canvas->ClipX - PanelWidth - Margin * UIScale);
    const float PanelY = TopY + Margin * UIScale;
    const float InnerWidth = PanelWidth - 2.0f * InnerPadX;
    const float MaxPanelBottom = Canvas->ClipY - Margin * UIScale;

    // ---- Tab bar (measure first; rows are laid out left to right, wrapping) --
    struct FTabBox
    {
        ELLDetailTab Tab;
        FString Title;
        float W = 0.0f;
        float H = 0.0f;
        float X = 0.0f;
        float Y = 0.0f;
    };
    TArray<FTabBox> Tabs;
    for (int32 Index = 0; Index < DetailTabCount; ++Index)
    {
        FTabBox Box;
        Box.Tab = static_cast<ELLDetailTab>(Index);
        Box.Title = TabTitle(Box.Tab);
        GetTextSize(Box.Title, Box.W, Box.H, Font, TabScale);
        Tabs.Add(Box);
    }

    const float TabPadXs = TabPadX * UIScale;
    const float TabPadYs = TabPadY * UIScale;
    float TabRowY = 0.0f;      // relative to the tab bar top
    float TabCursorX = 0.0f;   // relative to the inner left
    float TabRowH = 0.0f;
    for (FTabBox& Box : Tabs)
    {
        const float BoxW = Box.W + 2.0f * TabPadXs;
        const float BoxH = Box.H + 2.0f * TabPadYs;
        if (TabCursorX > 0.0f && TabCursorX + BoxW > InnerWidth)
        {
            TabRowY += TabRowH;
            TabCursorX = 0.0f;
            TabRowH = 0.0f;
        }
        Box.X = TabCursorX;
        Box.Y = TabRowY;
        TabCursorX += BoxW;
        TabRowH = FMath::Max(TabRowH, BoxH);
    }
    const float TabBarHeight = TabRowY + TabRowH;

    // ---- Content rows for the active tab -------------------------------------
    TArray<FRow> Rows;

    const FString Action = CurrentActionFor(Resident);
    LLObserverLabels::ENeedLevel WorstLevel = LLObserverLabels::ENeedLevel::Good;
    const FString SummaryLine = LLObserverLabels::StatusSummary(Resident.Needs, WorstLevel);

    switch (ActiveTab)
    {
        case ELLDetailTab::Needs:
        {
            for (const LLObserverLabels::FNeedRow& Need : LLObserverLabels::NeedRows(Resident.Needs))
            {
                FRow Row;
                Row.Left = Need.Name;
                Row.Right = LLObserverLabels::NeedLabel(Need.Value);
                Row.RightColor = LLObserverLabels::NeedColor(Need.Value);
                Row.Scale = RowScale;
                Rows.Add(Row);
            }
            break;
        }

        case ELLDetailTab::Personality:
        {
            for (const LLObserverLabels::FPersonalityAxisRow& Axis : LLObserverLabels::PersonalityAxisRows(Resident.Personality))
            {
                FRow Row;
                Row.Left = Axis.Name;
                Row.Right = LLObserverLabels::PersonalityAxisLabel(Axis.Value, Axis.Words);
                Row.RightColor = TextPrimary;
                Row.Scale = RowScale;
                Rows.Add(Row);
            }
            break;
        }

        case ELLDetailTab::TraitsSkills:
        {
            FRow TraitsHeader;
            TraitsHeader.Left = LLObserverText::SectionTraits;
            TraitsHeader.LeftColor = TextSection;
            TraitsHeader.Scale = SectionScale;
            Rows.Add(TraitsHeader);

            FRow TraitsRow;
            TraitsRow.Left = LLObserverLabels::JoinNames(Resident.Traits);
            TraitsRow.LeftColor = TextPrimary;
            TraitsRow.Scale = RowScale;
            Rows.Add(TraitsRow);

            FRow SkillsHeader;
            SkillsHeader.Left = LLObserverText::SectionSkills;
            SkillsHeader.LeftColor = TextSection;
            SkillsHeader.Scale = SectionScale;
            SkillsHeader.GapBefore = SectionGap;
            Rows.Add(SkillsHeader);

            if (Resident.Skills.Num() == 0)
            {
                FRow None;
                None.Left = LLObserverText::NoneListed;
                None.Scale = RowScale;
                Rows.Add(None);
            }
            else
            {
                TArray<FName> SkillNames;
                Resident.Skills.GetKeys(SkillNames);
                SkillNames.Sort(FNameLexicalLess());
                for (const FName& SkillName : SkillNames)
                {
                    FRow Row;
                    Row.Left = SkillName.ToString();
                    Row.Right = LLObserverLabels::SkillLabel(Resident.Skills[SkillName]);
                    Row.RightColor = TextPrimary;
                    Row.Scale = RowScale;
                    Rows.Add(Row);
                }
            }

            FRow LikesHeader;
            LikesHeader.Left = LLObserverText::SectionLikes;
            LikesHeader.LeftColor = TextSection;
            LikesHeader.Scale = SectionScale;
            LikesHeader.GapBefore = SectionGap;
            Rows.Add(LikesHeader);

            FRow LikesRow;
            LikesRow.Left = LLObserverLabels::JoinNames(Resident.Preferences);
            LikesRow.LeftColor = TextPrimary;
            LikesRow.Scale = RowScale;
            Rows.Add(LikesRow);
            break;
        }

        case ELLDetailTab::Overview:
        default:
        {
            FRow Name;
            Name.Left = FString::Printf(TEXT("%s   %d"), *Resident.DisplayName, Resident.AgeYears);
            Name.LeftColor = TextPrimary;
            Name.Scale = NameScale;
            Rows.Add(Name);

            FRow Identity;
            Identity.Left = LLObserverLabels::LifeStageToString(Resident.LifeStage)
                + LLObserverText::SummaryJoin + LLObserverLabels::SexToString(Resident.Sex);
            Identity.LeftColor = TextMuted;
            Identity.Scale = SectionScale;
            Rows.Add(Identity);

            if (!Action.IsEmpty())
            {
                FRow Now;
                Now.Left = FString(LLObserverText::NowPrefix) + Action;
                Now.LeftColor = TextAction;
                Now.Scale = BodyScale;
                Now.GapBefore = SectionGap;
                Rows.Add(Now);
            }

            FRow Summary;
            Summary.Left = SummaryLine;
            Summary.LeftColor = LLObserverLabels::NeedColorForLevel(WorstLevel);
            Summary.Scale = RowScale;
            Summary.GapBefore = Action.IsEmpty() ? SectionGap : 0.0f;
            Rows.Add(Summary);

            FRow Words;
            Words.Left = LLObserverLabels::PersonalityWords(Resident);
            Words.LeftColor = TextSecondary;
            Words.Scale = RowScale;
            Rows.Add(Words);

            if (!Resident.BackgroundTag.IsEmpty())
            {
                FRow Background;
                Background.Left = FString(LLObserverText::BackgroundPrefix) + Resident.BackgroundTag;
                Background.LeftColor = TextMuted;
                Background.Scale = SectionScale;
                Background.GapBefore = SectionGap;
                Rows.Add(Background);
            }
            break;
        }
    }

    // ---- Measure rows --------------------------------------------------------
    // Two-column rows share one right-hand column, placed after the widest left.
    float WidestLeft = 0.0f;
    for (const FRow& Row : Rows)
    {
        if (!Row.Right.IsEmpty())
        {
            float W = 0.0f, H = 0.0f;
            GetTextSize(Row.Left, W, H, Font, Row.Scale);
            WidestLeft = FMath::Max(WidestLeft, W);
        }
    }
    const float RightColumnX = FMath::Min(WidestLeft + 2.0f * InnerPadX, InnerWidth * 0.6f);

    struct FLine
    {
        FString Left;
        FLinearColor LeftColor;
        FString Right;
        FLinearColor RightColor;
        float Scale;
        float Height;
        float GapBefore;
    };
    TArray<FLine> Lines;
    float ContentHeight = 0.0f;
    for (const FRow& Row : Rows)
    {
        const float GapBefore = Row.GapBefore * UIScale;
        if (Row.Right.IsEmpty())
        {
            bool bFirst = true;
            for (const FString& Wrapped : WrapText(Row.Left, InnerWidth, Row.Scale))
            {
                float W = 0.0f, H = 0.0f;
                GetTextSize(Wrapped, W, H, Font, Row.Scale);
                Lines.Add({ Wrapped, Row.LeftColor, FString(), Row.RightColor, Row.Scale, H, bFirst ? GapBefore : 0.0f });
                ContentHeight += H + (bFirst ? GapBefore : 0.0f);
                bFirst = false;
            }
        }
        else
        {
            float W = 0.0f, H = 0.0f;
            GetTextSize(Row.Left, W, H, Font, Row.Scale);
            Lines.Add({ Row.Left, Row.LeftColor, Row.Right, Row.RightColor, Row.Scale, H, GapBefore });
            ContentHeight += H + GapBefore;
        }
    }
    ContentHeight += Gap * FMath::Max(0, Lines.Num() - 1);

    // ---- Draw ------------------------------------------------------------------
    const float DesiredHeight = Pad + TabBarHeight + Gap + ContentHeight + Pad;
    const float PanelHeight = FMath::Min(DesiredHeight, FMath::Max(0.0f, MaxPanelBottom - PanelY));

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, DetailAlpha), PanelX, PanelY, PanelWidth, PanelHeight);
    DetailPanelRect = FBox2D(FVector2D(PanelX, PanelY), FVector2D(PanelX + PanelWidth, PanelY + PanelHeight));

    const float TextX = PanelX + InnerPadX;
    const float TabBarTop = PanelY + Pad;
    for (const FTabBox& Box : Tabs)
    {
        const float BoxX = TextX + Box.X;
        const float BoxY = TabBarTop + Box.Y;
        const float BoxW = Box.W + 2.0f * TabPadXs;
        const float BoxH = Box.H + 2.0f * TabPadYs;
        const bool bActive = Box.Tab == ActiveTab;

        DrawText(Box.Title, bActive ? TextPrimary : TextMuted, BoxX + TabPadXs, BoxY + TabPadYs, Font, TabScale, false);
        if (bActive)
        {
            DrawRect(TabActiveLine, BoxX + TabPadXs, BoxY + BoxH - TabUnderline * UIScale, Box.W, TabUnderline * UIScale);
        }
        DetailTabRects[static_cast<int32>(Box.Tab)] = FBox2D(FVector2D(BoxX, BoxY), FVector2D(BoxX + BoxW, BoxY + BoxH));
    }

    float CursorY = TabBarTop + TabBarHeight + Gap;
    const float PanelBottom = PanelY + PanelHeight - Pad;
    for (const FLine& Line : Lines)
    {
        CursorY += Line.GapBefore;
        if (CursorY + Line.Height > PanelBottom)
        {
            break; // content that does not fit is simply not drawn
        }
        DrawText(Line.Left, Line.LeftColor, TextX, CursorY, Font, Line.Scale, false);
        if (!Line.Right.IsEmpty())
        {
            DrawText(Line.Right, Line.RightColor, TextX + RightColumnX, CursorY, Font, Line.Scale, false);
        }
        CursorY += Line.Height + Gap;
    }
}
