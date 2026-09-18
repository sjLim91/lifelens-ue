#include "UI/LLObserverHUD.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverLabels.h"
#include "UI/LLObserverMobilePolishLabels.h"
#include "UI/LLObserverKorean.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Characters/LLResidentCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "SceneView.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/IConsoleManager.h"

static_assert(static_cast<int32>(ELLDetailTab::Count) == ALLObserverHUD::DetailTabCount, "DetailTabCount must match ELLDetailTab::Count");

static TAutoConsoleVariable<int32> CVarLLDebugTapTargets(
    TEXT("ll.DebugTapTargets"),
    0,
    TEXT("1: draw each resident's projected bounds (yellow) and tap rectangle (cyan) on the observer HUD."),
    ECVF_Default);

namespace
{
    constexpr float TouchTargetLogicalPixels = 48.0f;
    constexpr float Margin           = 16.0f;
    constexpr float PadX             = 12.0f;
    constexpr float PadY             = 8.0f;
    constexpr float LineGap          = 6.0f;
    constexpr float SectionGap       = 12.0f;
    constexpr float DetailWheelStep  = 64.0f;

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
            case ELLDetailTab::Needs:         return LLObserverText::TabNeeds;
            case ELLDetailTab::Personality:   return LLObserverText::TabPersonality;
            case ELLDetailTab::TraitsSkills:  return LLObserverText::TabTraitsSkills;
            case ELLDetailTab::Emotion:       return LLObserverText::TabEmotion;
            case ELLDetailTab::Relationships: return LLObserverText::TabRelationships;
            case ELLDetailTab::Family:        return LLObserverText::TabFamily;
            case ELLDetailTab::Civilization:  return LLObserverText::TabCivilization;
            case ELLDetailTab::Overview:
            default:                          return LLObserverText::TabOverview;
        }
    }

    template <typename TEnum>
    FString EnumLabel(TEnum Value)
    {
        const UEnum* Enum = StaticEnum<TEnum>();
        if (!Enum)
        {
            return TEXT("미상");
        }
        FString Name = Enum->GetNameStringByValue(static_cast<int64>(Value));
        Name.ReplaceInline(TEXT("_"), TEXT(" "));
        return Name;
    }

    FString EnumLabel(ELLCoreRomanceStage Value)      { return LLObserverKorean::RomanceStage(Value); }
    FString EnumLabel(ELLCoreMaterialKind Value)      { return LLObserverKorean::Material(Value); }
    FString EnumLabel(ELLCoreItemKind Value)          { return LLObserverKorean::Item(Value); }
    FString EnumLabel(ELLCoreTechniqueId Value)       { return LLObserverKorean::Technique(Value); }
    FString EnumLabel(ELLCoreKnowledgeLevel Value)    { return LLObserverKorean::KnowledgeLevel(Value); }
    FString EnumLabel(ELLCoreKnowledgeSource Value)   { return LLObserverKorean::KnowledgeSource(Value); }

    FString Percent01(float Value)
    {
        return FString::Printf(TEXT("%.0f%%"), FMath::Clamp(Value, 0.0f, 1.0f) * 100.0f);
    }

    FString NeedSatisfactionBar(float CoreDeficit)
    {
        const float Satisfaction = 1.0f - FMath::Clamp(CoreDeficit, 0.0f, 1.0f);
        const int32 Filled = FMath::Clamp(FMath::RoundToInt(Satisfaction * 10.0f), 0, 10);
        FString Bar(TEXT("["));
        for (int32 Index = 0; Index < 10; ++Index)
        {
            Bar += Index < Filled ? TEXT("|") : TEXT(".");
        }
        Bar += FString::Printf(TEXT("] %.0f%%"), Satisfaction * 100.0f);
        return Bar;
    }

    float NeedSatisfaction100(float CoreDeficit)
    {
        return (1.0f - FMath::Clamp(CoreDeficit, 0.0f, 1.0f)) * 100.0f;
    }

    FString Scalar02(float Value)
    {
        return FString::Printf(TEXT("%.2f"), Value);
    }

    FString JoinFamilyNames(const TArray<FLLCoreFamilyMemberSnapshot>& Members)
    {
        TArray<FString> Names;
        Names.Reserve(Members.Num());
        for (const FLLCoreFamilyMemberSnapshot& Member : Members)
        {
            if (!Member.DisplayName.IsEmpty())
            {
                Names.Add(Member.DisplayName);
            }
        }
        return Names.Num() > 0 ? FString::Join(Names, TEXT(" · ")) : FString(LLObserverKorean::None);
    }

    struct FRow
    {
        FString Left;
        FLinearColor LeftColor = TextSecondary;
        FString Right;
        FLinearColor RightColor = TextSecondary;
        float Scale = 1.0f;
        float GapBefore = 0.0f;
    };
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

void ALLObserverHUD::ResetDetailScroll()
{
    DetailScrollOffset = 0.0f;
    DetailScrollMax = 0.0f;
    LastDetailScrollDragY = 0.0f;
    bDetailScrollDragging = false;
    DetailContentRect = FBox2D(ForceInit);
}

ULLObservationSubsystem* ALLObserverHUD::GetObservation() const
{
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
}

ULLCoreBridgeSubsystem* ALLObserverHUD::GetCoreBridge() const
{
    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
}

FString ALLObserverHUD::CurrentActionFor(const FLLResidentData& Resident) const
{
    if (ULLCoreBridgeSubsystem* Bridge = GetCoreBridge())
    {
        FLLCoreResidentObservation CoreResident;
        if (Bridge->GetResidentObservation(Resident.ResidentId, CoreResident) && !CoreResident.ActivityLabel.IsEmpty())
        {
            return CoreResident.ActivityLabel;
        }
    }

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

float ALLObserverHUD::TouchTargetRadiusPixels(const UObject* WorldContext)
{
    const float DPIScale = WorldContext ? FMath::Max(1.0f, UWidgetLayoutLibrary::GetViewportScale(WorldContext)) : 1.0f;
    return TouchTargetLogicalPixels * DPIScale;
}

bool ALLObserverHUD::ProjectResidentTapRect(const APlayerController* PlayerController, const ALLResidentCharacter* Resident,
    float RadiusPixels, FBox2D& OutBoundsRect, FBox2D& OutTapRect)
{
    OutBoundsRect = FBox2D(ForceInit);
    OutTapRect = FBox2D(ForceInit);
    if (!PlayerController || !Resident)
    {
        return false;
    }

    FVector Origin = FVector::ZeroVector;
    FVector Extent = FVector::ZeroVector;
    Resident->GetActorBounds(false, Origin, Extent, false);

    for (int32 Corner = 0; Corner < 8; ++Corner)
    {
        const FVector World(
            Origin.X + ((Corner & 1) ? Extent.X : -Extent.X),
            Origin.Y + ((Corner & 2) ? Extent.Y : -Extent.Y),
            Origin.Z + ((Corner & 4) ? Extent.Z : -Extent.Z));

        FVector2D Screen;
        if (!PlayerController->ProjectWorldLocationToScreen(World, Screen, false))
        {
            return false;
        }
        OutBoundsRect += Screen;
    }

    OutTapRect = OutBoundsRect.ExpandBy(FVector2D(RadiusPixels, RadiusPixels));
    return true;
}

void ALLObserverHUD::DrawDebugTapTargets(const FVector2D& ViewportSize)
{
    const APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController || !GetWorld())
    {
        return;
    }

    const float Radius = TouchTargetRadiusPixels(this);
    const FLinearColor BoundsColor(1.0f, 0.9f, 0.2f, 0.9f);
    const FLinearColor TapColor(0.3f, 0.9f, 1.0f, 0.7f);
    const FLinearColor OriginColor(1.0f, 0.4f, 0.4f, 0.9f);

    auto DrawRectOutline = [this](const FBox2D& Rect, const FLinearColor& Color)
    {
        DrawLine(Rect.Min.X, Rect.Min.Y, Rect.Max.X, Rect.Min.Y, Color, 1.0f);
        DrawLine(Rect.Max.X, Rect.Min.Y, Rect.Max.X, Rect.Max.Y, Color, 1.0f);
        DrawLine(Rect.Max.X, Rect.Max.Y, Rect.Min.X, Rect.Max.Y, Color, 1.0f);
        DrawLine(Rect.Min.X, Rect.Max.Y, Rect.Min.X, Rect.Min.Y, Color, 1.0f);
    };

    for (TActorIterator<ALLResidentCharacter> It(GetWorld()); It; ++It)
    {
        FBox2D Bounds;
        FBox2D Tap;
        if (!ProjectResidentTapRect(PlayerController, *It, Radius, Bounds, Tap))
        {
            continue;
        }

        const FBox2D BoundsCanvas(ViewportToCanvas(Bounds.Min, ViewportSize), ViewportToCanvas(Bounds.Max, ViewportSize));
        const FBox2D TapCanvas(ViewportToCanvas(Tap.Min, ViewportSize), ViewportToCanvas(Tap.Max, ViewportSize));
        DrawRectOutline(TapCanvas, TapColor);
        DrawRectOutline(BoundsCanvas, BoundsColor);

        FVector2D OriginScreen;
        if (PlayerController->ProjectWorldLocationToScreen(It->GetActorLocation(), OriginScreen, false))
        {
            const FVector2D O = ViewportToCanvas(OriginScreen, ViewportSize);
            DrawLine(O.X - 6.0f, O.Y, O.X + 6.0f, O.Y, OriginColor, 1.0f);
            DrawLine(O.X, O.Y - 6.0f, O.X, O.Y + 6.0f, OriginColor, 1.0f);
        }
    }
}

FVector2D ALLObserverHUD::ViewportToCanvas(const FVector2D& ViewportPosition, const FVector2D& ViewportSize) const
{
    if (bLastViewRectKnown)
    {
        return ViewportPosition - LastViewRectMin;
    }

    if (ViewportSize.X > 0.0 && ViewportSize.Y > 0.0 && LastCanvasSize.X > 0.0 && LastCanvasSize.Y > 0.0)
    {
        const FVector2D Offset = (ViewportSize - LastCanvasSize) * 0.5;
        return ViewportPosition - Offset;
    }
    return ViewportPosition;
}

bool ALLObserverHUD::HandleDetailScrollWheel(const FVector2D& InScreenPosition, float WheelDelta, const FVector2D& ViewportSize)
{
    ULLObservationSubsystem* Observation = GetObservation();
    if (!Observation || Observation->GetObservationLevel() != ELLObservationLevel::Detail)
    {
        return false;
    }

    const FVector2D ScreenPosition = ViewportToCanvas(InScreenPosition, ViewportSize);
    if (!RectContains(DetailContentRect, ScreenPosition))
    {
        return false;
    }

    const float Step = DetailWheelStep * ComputeUIScale();
    DetailScrollOffset = FMath::Clamp(DetailScrollOffset - WheelDelta * Step, 0.0f, DetailScrollMax);
    return true;
}

bool ALLObserverHUD::BeginDetailScrollDrag(const FVector2D& InScreenPosition, const FVector2D& ViewportSize)
{
    ULLObservationSubsystem* Observation = GetObservation();
    if (!Observation || Observation->GetObservationLevel() != ELLObservationLevel::Detail)
    {
        return false;
    }

    const FVector2D ScreenPosition = ViewportToCanvas(InScreenPosition, ViewportSize);
    if (!RectContains(DetailContentRect, ScreenPosition))
    {
        return false;
    }

    bDetailScrollDragging = true;
    LastDetailScrollDragY = ScreenPosition.Y;
    return true;
}

bool ALLObserverHUD::UpdateDetailScrollDrag(const FVector2D& InScreenPosition, const FVector2D& ViewportSize)
{
    if (!bDetailScrollDragging)
    {
        return false;
    }

    const FVector2D ScreenPosition = ViewportToCanvas(InScreenPosition, ViewportSize);
    const float DeltaY = LastDetailScrollDragY - ScreenPosition.Y;
    LastDetailScrollDragY = ScreenPosition.Y;
    DetailScrollOffset = FMath::Clamp(DetailScrollOffset + DeltaY, 0.0f, DetailScrollMax);
    return true;
}

void ALLObserverHUD::EndDetailScrollDrag()
{
    bDetailScrollDragging = false;
}

bool ALLObserverHUD::HandleTap(const FVector2D& InScreenPosition, const FVector2D& ViewportSize)
{
    ULLObservationSubsystem* Observation = GetObservation();
    if (!Observation)
    {
        return false;
    }

    const FVector2D ScreenPosition = ViewportToCanvas(InScreenPosition, ViewportSize);

    switch (Observation->GetObservationLevel())
    {
        case ELLObservationLevel::Detail:
            if (RectContains(DetailBackRect, ScreenPosition))
            {
                ResetDetailScroll();
                Observation->CloseDetail();
                return true;
            }
            for (int32 Index = 0; Index < DetailTabCount; ++Index)
            {
                if (RectContains(DetailTabRects[Index], ScreenPosition))
                {
                    const ELLDetailTab NewTab = static_cast<ELLDetailTab>(Index);
                    if (NewTab != ActiveTab)
                    {
                        ActiveTab = NewTab;
                        ResetDetailScroll();
                    }
                    return true;
                }
            }
            return RectContains(DetailPanelRect, ScreenPosition);

        case ELLObservationLevel::Quick:
            if (RectContains(QuickInspectorRect, ScreenPosition))
            {
                ActiveTab = ELLDetailTab::Overview;
                ResetDetailScroll();
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
                bWorldOverviewOpen = false;
                return true;
            }
            return false;
    }
}

void ALLObserverHUD::DrawHUD()
{
    Super::DrawHUD();

    OverviewBandRect = FBox2D(ForceInit);
    WorldOverviewRect = FBox2D(ForceInit);
    QuickInspectorRect = FBox2D(ForceInit);
    DetailPanelRect = FBox2D(ForceInit);
    DetailBackRect = FBox2D(ForceInit);
    DetailContentRect = FBox2D(ForceInit);
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
    LastCanvasSize = FVector2D(Canvas->ClipX, Canvas->ClipY);
    if (Canvas->SceneView)
    {
        LastViewRectMin = FVector2D(Canvas->SceneView->UnscaledViewRect.Min.X, Canvas->SceneView->UnscaledViewRect.Min.Y);
        bLastViewRectKnown = true;
    }
    else
    {
        bLastViewRectKnown = false;
    }

    FLLResidentData Selected;
    const bool bHasSelection = Observation && Observation->HasObservedResident()
        && Simulation->FindResidentById(Observation->GetObservedResidentId(), Selected);

    UpdateFeedbackState(Observation);

    const bool bDetailOpen = bHasSelection && Observation->IsDetailOpen();
    if (!bDetailOpen)
    {
        ResetDetailScroll();
    }
    const float OverviewBottom = DrawOverview(*Simulation, Residents, UIScale, !bHasSelection, bDetailOpen);

    if (CVarLLDebugTapTargets.GetValueOnGameThread() > 0)
    {
        int32 ViewportX = 0;
        int32 ViewportY = 0;
        if (APlayerController* PlayerController = GetOwningPlayerController())
        {
            PlayerController->GetViewportSize(ViewportX, ViewportY);
        }
        DrawDebugTapTargets(FVector2D(ViewportX, ViewportY));
    }

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

    DrawSelectionFeedback(Selected, UIScale);
}

float ALLObserverHUD::DrawOverview(const ULLSimulationSubsystem& Simulation, const TArray<FLLResidentData>& Residents,
    float UIScale, bool bShowHint, bool bDimStrip)
{
    UFont* Font = HUDFont();

    const int64 TotalMinutes = Simulation.GetSimulationMinute();
    const int64 Day = TotalMinutes / 1440 + 1;
    const int32 MinuteOfDay = static_cast<int32>(TotalMinutes % 1440);
    const int32 Hour = MinuteOfDay / 60;
    const int32 Minute = MinuteOfDay % 60;

    const float TitleScale = 1.0f * UIScale;
    const float StripScale = 0.85f * UIScale;

    // Adaptive information density: the observer should see less chrome when
    // the phone viewport is tight or the camera is pulled far away. This is
    // presentation-only and uses rendered resident distance, never Core state.
    const float LogicalWidth = Canvas->ClipX / FMath::Max(0.01f, UIScale);
    float ClosestResidentDistanceUU = TNumericLimits<float>::Max();
    if (const APlayerController* PlayerController = GetOwningPlayerController())
    {
        const APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager;
        if (CameraManager)
        {
            const FVector CameraLocation = CameraManager->GetCameraLocation();
            for (const FLLResidentData& Resident : Residents)
            {
                if (const ALLResidentCharacter* Actor = FindResidentActor(GetWorld(), Resident.ResidentId))
                {
                    ClosestResidentDistanceUU = FMath::Min(
                        ClosestResidentDistanceUU,
                        FVector::Dist(CameraLocation, Actor->GetActorLocation()));
                }
            }
        }
    }

    const bool bCompactDensity = LogicalWidth < 620.0f || ClosestResidentDistanceUU > 7000.0f;
    const bool bReducedDensity = bCompactDensity || LogicalWidth < 900.0f || ClosestResidentDistanceUU > 4000.0f;

    const FString StatusLine = bCompactDensity
        ? FString::Printf(TEXT("%s   %d%s"), LLObserverText::OverviewTitle, Residents.Num(), LLObserverText::ResidentsSuffix)
        : FString::Printf(TEXT("%s   %lld%s  %02d:%02d   %d%s"),
            LLObserverText::OverviewTitle, static_cast<long long>(Day), LLObserverKorean::Day,
            Hour, Minute, Residents.Num(), LLObserverText::ResidentsSuffix);

    TArray<FString> StripItems;
    StripItems.Reserve(Residents.Num());
    for (const FLLResidentData& Resident : Residents)
    {
        if (bReducedDensity)
        {
            StripItems.Add(Resident.DisplayName);
            continue;
        }

        const FString Action = CurrentActionFor(Resident);
        StripItems.Add(Action.IsEmpty()
            ? Resident.DisplayName
            : Resident.DisplayName + LLObserverText::StripNameActionJoin + Action);
    }

    const FSafeInsets Insets = SafeInsets(UIScale);
    const float StripMaxWidth = FMath::Max(0.0f, Canvas->ClipX - Insets.Left - Insets.Right);

    FString StripLine;
    if (StripItems.Num() == 0)
    {
        StripLine = LLObserverMobilePolishText::NoResidents;
    }
    else
    {
        for (int32 Shown = 0; Shown < StripItems.Num(); ++Shown)
        {
            TArray<FString> Candidate(StripItems.GetData(), Shown + 1);
            FString CandidateLine = FString::Join(Candidate, LLObserverText::StripSeparator);
            const int32 Remaining = StripItems.Num() - (Shown + 1);
            if (Remaining > 0)
            {
                CandidateLine += FString(LLObserverText::StripSeparator)
                    + LLObserverMobilePolishText::StripMorePrefix + FString::FromInt(Remaining);
            }

            float W = 0.0f, H = 0.0f;
            GetTextSize(CandidateLine, W, H, Font, StripScale);
            if (W > StripMaxWidth && Shown > 0)
            {
                break;
            }
            StripLine = CandidateLine;
        }
    }

    const FString HintLine(LLObserverText::TapHint);

    float StatusW = 0.0f, StatusH = 0.0f;
    float StripW = 0.0f, StripH = 0.0f;
    float HintW = 0.0f, HintH = 0.0f;
    GetTextSize(StatusLine, StatusW, StatusH, Font, TitleScale);
    GetTextSize(StripLine, StripW, StripH, Font, StripScale);
    GetTextSize(HintLine, HintW, HintH, Font, StripScale);

    const float X = Insets.Left;
    const float Pad = PadY * UIScale;
    const float TopPad = FMath::Max(Pad, Insets.Top);
    const float Gap = LineGap * UIScale;
    const float BandHeight = TopPad + StatusH + Gap + StripH + Pad;

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, OverviewAlpha), 0.0f, 0.0f, Canvas->ClipX, BandHeight);
    OverviewBandRect = FBox2D(FVector2D(0.0f, 0.0f), FVector2D(Canvas->ClipX, BandHeight));

    float CursorY = TopPad;
    DrawText(StatusLine, TextPrimary, X, CursorY, Font, TitleScale, false);
    CursorY += StatusH + Gap;

    FLinearColor StripColor = TextMuted;
    if (bDimStrip)
    {
        StripColor.A *= 0.5f;
    }
    DrawText(StripLine, StripColor, X, CursorY, Font, StripScale, false);

    const float HintX = X + StripW + 3.0f * PadX * UIScale;
    if (bShowHint && HintX + HintW <= Canvas->ClipX - Insets.Right)
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

    TArray<FRow> Rows;
    {
        FRow Title;
        Title.Left = LLObserverText::WorldOverviewTitle;
        Title.LeftColor = TextPrimary;
        Title.Scale = TitleScale;
        Rows.Add(Title);

        FRow Clock;
        Clock.Left = FString::Printf(TEXT("%lld%s  %02d:%02d"), static_cast<long long>(Day), LLObserverKorean::Day, Hour, Minute);
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

        if (ULLCoreBridgeSubsystem* Bridge = GetCoreBridge(); Bridge && Bridge->IsCoreRunning())
        {
            const FLLCoreWorldObservation CoreWorld = Bridge->GetWorldObservation();
            const FLLCoreCivilizationWorldObservation Civilization = Bridge->GetCivilizationWorldObservation(1);

            FRow Households; Households.Left = LLObserverKorean::Households; Households.Right = FString::FromInt(CoreWorld.Households); Households.Scale = RowScale; Households.GapBefore = SectionGap; Rows.Add(Households);
            FRow Couples; Couples.Left = LLObserverKorean::ActiveCouples; Couples.Right = FString::FromInt(CoreWorld.ActiveCouples); Couples.Scale = RowScale; Rows.Add(Couples);
            FRow Pregnancies; Pregnancies.Left = LLObserverKorean::Pregnancies; Pregnancies.Right = FString::FromInt(CoreWorld.ActivePregnancies); Pregnancies.Scale = RowScale; Rows.Add(Pregnancies);
            FRow Techniques; Techniques.Left = LLObserverKorean::KnownTechniques; Techniques.Right = FString::FromInt(Civilization.UniqueKnownTechniqueTypes); Techniques.Scale = RowScale; Techniques.GapBefore = SectionGap; Rows.Add(Techniques);
            FRow Stored; Stored.Left = LLObserverKorean::StoredUnits; Stored.Right = FString::FromInt(Civilization.TotalStoredUnits); Stored.Scale = RowScale; Rows.Add(Stored);

            if (Civilization.RecentDiscoveries.Num() > 0)
            {
                const FLLCoreCivilizationDiscoveryObservation& Latest = Civilization.RecentDiscoveries[0];
                FRow Discovery;
                Discovery.Left = LLObserverKorean::RecentDiscovery;
                Discovery.Right = EnumLabel(Latest.Technique) + TEXT(" · ") + Latest.DiscovererName;
                Discovery.Scale = RowScale;
                Rows.Add(Discovery);
            }
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
    const FSafeInsets Insets = SafeInsets(UIScale);
    const float PanelX = Insets.Left;
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

    const FString NameLine = FString::Printf(TEXT("%s   %d"), *Resident.DisplayName, Resident.AgeYears);
    const FString Action = CurrentActionFor(Resident);
    const FString NowLine = Action.IsEmpty() ? FString() : FString(LLObserverText::NowPrefix) + Action;

    LLObserverLabels::ENeedLevel WorstLevel = LLObserverLabels::ENeedLevel::Good;
    const FString SummaryLine = LLObserverLabels::StatusSummary(Resident.Needs, WorstLevel);
    const FLinearColor SummaryColor = LLObserverLabels::NeedColorForLevel(WorstLevel);
    const FString WordsLine = LLObserverLabels::PersonalityWords(Resident);

    struct FEntry { FString Text; FLinearColor Color; float Scale; };
    TArray<FEntry> Entries;
    Entries.Add({ NameLine, TextPrimary, NameScale });
    if (!NowLine.IsEmpty()) Entries.Add({ NowLine, TextAction, BodyScale });
    Entries.Add({ SummaryLine, SummaryColor, SummaryScale });

    // On narrow mobile viewports, keep the quick card about "what is happening
    // now"; personality remains one tap away in LEVEL 2 and no data is lost.
    const float LogicalWidth = Canvas->ClipX / FMath::Max(0.01f, UIScale);
    if (LogicalWidth >= 720.0f)
    {
        Entries.Add({ WordsLine, TextSecondary, WordsScale });
    }
    Entries.Add({ FString(LLObserverText::DetailsHint), TextHint, HintScale });

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

    struct FLine { FString Text; FLinearColor Color; float Scale; float Height; };
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
    const FSafeInsets Insets = SafeInsets(UIScale);
    const float PanelX = FMath::Max(Insets.Left, Canvas->ClipX - PanelWidth - Insets.Right);
    const float PanelY = TopY + Margin * UIScale;

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, InspectorAlpha * PanelFade), PanelX, PanelY, PanelWidth, PanelHeight);
    QuickInspectorRect = FBox2D(FVector2D(PanelX, PanelY), FVector2D(PanelX + PanelWidth, PanelY + PanelHeight));

    float CursorY = PanelY + Pad;
    const float TextX = PanelX + InnerPadX;
    for (const FLine& Line : Lines)
    {
        DrawText(Line.Text, Faded(Line.Color), TextX, CursorY, Font, Line.Scale, false);
        CursorY += Line.Height + Gap;
    }
}

void ALLObserverHUD::DrawDetailPanel(const FLLResidentData& Resident, float UIScale, float TopY)
{
    UFont* Font = HUDFont();

    if (LastDetailResidentId != Resident.ResidentId)
    {
        LastDetailResidentId = Resident.ResidentId;
        ActiveTab = ELLDetailTab::Overview;
        ResetDetailScroll();
    }

    const float NameScale    = 1.15f * UIScale;
    const float BodyScale    = 0.95f * UIScale;
    const float RowScale     = 0.90f * UIScale;
    const float SectionScale = 0.85f * UIScale;
    const float TabScale     = 0.90f * UIScale;

    const float Gap = LineGap * UIScale;
    const float Pad = PadY * UIScale;
    const float InnerPadX = PadX * UIScale;

    const FSafeInsets Insets = SafeInsets(UIScale);
    const float AvailableWidth = FMath::Max(0.0f, Canvas->ClipX - Insets.Left - Insets.Right);
    const float PreferredWidth = FMath::Min(DetailMaxWidth * UIScale, Canvas->ClipX * DetailWidthRatio);
    const float PanelWidth = FMath::Min(AvailableWidth, FMath::Max(PreferredWidth, DetailMinWidth * UIScale));
    const float PanelX = FMath::Max(Insets.Left, Canvas->ClipX - PanelWidth - Insets.Right);
    const float PanelY = TopY + Margin * UIScale;
    const float InnerWidth = PanelWidth - 2.0f * InnerPadX;
    const float MaxPanelBottom = Canvas->ClipY - Insets.Bottom;
    const float MinTouch = TouchTargetRadiusPixels(this);

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
    float TabRowY = 0.0f;
    float TabCursorX = 0.0f;
    float TabRowH = 0.0f;
    for (FTabBox& Box : Tabs)
    {
        const float BoxW = FMath::Max(Box.W + 2.0f * TabPadXs, MinTouch);
        const float BoxH = FMath::Max(Box.H + 2.0f * TabPadYs, MinTouch);
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

    const FString BackText(LLObserverMobilePolishText::BackHint);
    float BackW = 0.0f, BackH = 0.0f;
    GetTextSize(BackText, BackW, BackH, Font, TabScale);
    const float BackRowHeight = FMath::Max(BackH + 2.0f * TabPadYs, MinTouch);

    TArray<FRow> Rows;

    const FString Action = CurrentActionFor(Resident);
    LLObserverLabels::ENeedLevel WorstLevel = LLObserverLabels::ENeedLevel::Good;
    const FString SummaryLine = LLObserverLabels::StatusSummary(Resident.Needs, WorstLevel);

    ULLCoreBridgeSubsystem* Bridge = GetCoreBridge();
    FLLCoreResidentObservation CoreResident;
    FLLCoreFamilyObservation CoreFamily;
    FLLCoreResidentCivilizationObservation CoreCivilization;
    FLLCoreTraitPreferenceObservation CoreDisposition;
    const bool bHasCoreResident = Bridge && Bridge->GetResidentObservation(Resident.ResidentId, CoreResident);
    const bool bHasCoreFamily = Bridge && Bridge->GetFamilyObservation(Resident.ResidentId, CoreFamily);
    const bool bHasCoreCivilization = Bridge && Bridge->GetResidentCivilizationObservation(Resident.ResidentId, CoreCivilization);
    const bool bHasCoreDisposition = Bridge && Bridge->GetResidentTraitPreferenceObservation(Resident.ResidentId, CoreDisposition);

    switch (ActiveTab)
    {
        case ELLDetailTab::Needs:
        {
            if (!bHasCoreResident)
            {
                FRow None; None.Left = LLObserverKorean::CoreUnavailable; None.Scale = RowScale; Rows.Add(None);
                break;
            }

            auto AddNeed = [&Rows, RowScale](const TCHAR* Name, float Deficit)
            {
                FRow Row;
                Row.Left = Name;
                Row.Right = NeedSatisfactionBar(Deficit);
                Row.RightColor = LLObserverLabels::NeedColor(NeedSatisfaction100(Deficit));
                Row.Scale = RowScale;
                Rows.Add(Row);
            };
            AddNeed(LLObserverKorean::Hunger, CoreResident.Needs.Hunger);
            AddNeed(LLObserverKorean::Thirst, CoreResident.Needs.Thirst);
            AddNeed(LLObserverKorean::Energy, CoreResident.Needs.Sleep);
            AddNeed(LLObserverKorean::Hygiene, CoreResident.Needs.Hygiene);
            AddNeed(LLObserverKorean::Bladder, CoreResident.Needs.Bladder);
            break;
        }

        case ELLDetailTab::Personality:
        {
            if (!bHasCoreResident)
            {
                FRow None; None.Left = LLObserverKorean::CoreUnavailable; None.Scale = RowScale; Rows.Add(None);
                break;
            }

            auto AddAxis = [&Rows, RowScale](const TCHAR* Name, float Value)
            {
                FRow Row;
                Row.Left = Name;
                Row.Right = Percent01(Value);
                Row.RightColor = TextPrimary;
                Row.Scale = RowScale;
                Rows.Add(Row);
            };
            AddAxis(LLObserverKorean::Introversion, CoreResident.Personality.Introversion);
            AddAxis(LLObserverKorean::Conscientiousness, CoreResident.Personality.Conscientiousness);
            AddAxis(LLObserverKorean::Openness, CoreResident.Personality.Openness);
            AddAxis(LLObserverKorean::Agreeableness, CoreResident.Personality.Agreeableness);
            AddAxis(LLObserverKorean::EmotionalStability, CoreResident.Personality.EmotionalStability);
            AddAxis(LLObserverKorean::Empathy, CoreResident.Personality.Empathy);
            AddAxis(LLObserverKorean::Impulsiveness, CoreResident.Personality.Impulsiveness);
            AddAxis(LLObserverKorean::RiskTolerance, CoreResident.Personality.RiskTolerance);
            AddAxis(LLObserverKorean::Ambition, CoreResident.Personality.Ambition);
            AddAxis(LLObserverKorean::Patience, CoreResident.Personality.Patience);
            AddAxis(LLObserverKorean::Sociability, CoreResident.Personality.Sociability);
            AddAxis(LLObserverKorean::Curiosity, CoreResident.Personality.Curiosity);
            AddAxis(LLObserverKorean::Orderliness, CoreResident.Personality.Orderliness);
            AddAxis(LLObserverKorean::Adaptability, CoreResident.Personality.Adaptability);
            break;
        }

        case ELLDetailTab::TraitsSkills:
        {
            if (!bHasCoreDisposition)
            {
                FRow None; None.Left = LLObserverKorean::CoreTraitUnavailable; None.Scale = RowScale; Rows.Add(None);
                break;
            }

            FRow TraitsHeader;
            TraitsHeader.Left = LLObserverText::SectionTraits;
            TraitsHeader.LeftColor = TextSection;
            TraitsHeader.Scale = SectionScale;
            Rows.Add(TraitsHeader);

            auto AddProfilePair = [&Rows, RowScale](const TCHAR* FirstName, float FirstValue, const TCHAR* SecondName, float SecondValue)
            {
                FRow Row;
                Row.Left = FString::Printf(TEXT("%s %s · %s %s"), FirstName, *Percent01(FirstValue), SecondName, *Percent01(SecondValue));
                Row.Scale = RowScale;
                Rows.Add(Row);
            };
            AddProfilePair(LLObserverKorean::Resilience, CoreDisposition.Traits.Resilience, LLObserverKorean::Creativity, CoreDisposition.Traits.Creativity);
            AddProfilePair(LLObserverKorean::Discipline, CoreDisposition.Traits.Discipline, LLObserverKorean::Compassion, CoreDisposition.Traits.Compassion);
            AddProfilePair(LLObserverKorean::Adaptability, CoreDisposition.Traits.Adaptability, LLObserverKorean::Boldness, CoreDisposition.Traits.Boldness);
            AddProfilePair(LLObserverKorean::Perseverance, CoreDisposition.Traits.Perseverance, LLObserverKorean::Resourcefulness, CoreDisposition.Traits.Resourcefulness);

            FRow SkillsHeader;
            SkillsHeader.Left = LLObserverText::SectionSkills;
            SkillsHeader.LeftColor = TextSection;
            SkillsHeader.Scale = SectionScale;
            SkillsHeader.GapBefore = SectionGap;
            Rows.Add(SkillsHeader);

            if (!bHasCoreCivilization)
            {
                FRow None; None.Left = LLObserverKorean::CoreSkillUnavailable; None.Scale = RowScale; Rows.Add(None);
            }
            else
            {
                FRow Gathering; Gathering.Left = LLObserverKorean::Gathering; Gathering.Right = Percent01(CoreCivilization.GatheringSkill); Gathering.RightColor = TextPrimary; Gathering.Scale = RowScale; Rows.Add(Gathering);
                FRow Crafting; Crafting.Left = LLObserverKorean::Crafting; Crafting.Right = Percent01(CoreCivilization.CraftingSkill); Crafting.RightColor = TextPrimary; Crafting.Scale = RowScale; Rows.Add(Crafting);
                FRow Learning; Learning.Left = LLObserverKorean::Learning; Learning.Right = Percent01(CoreCivilization.LearningSkill); Learning.RightColor = TextPrimary; Learning.Scale = RowScale; Rows.Add(Learning);
            }

            FRow PreferencesHeader;
            PreferencesHeader.Left = LLObserverKorean::Preferences;
            PreferencesHeader.LeftColor = TextSection;
            PreferencesHeader.Scale = SectionScale;
            PreferencesHeader.GapBefore = SectionGap;
            Rows.Add(PreferencesHeader);

            AddProfilePair(LLObserverKorean::Socializing, CoreDisposition.Preferences.Socializing, LLObserverKorean::Solitude, CoreDisposition.Preferences.Solitude);
            AddProfilePair(LLObserverKorean::Exploration, CoreDisposition.Preferences.Exploration, LLObserverKorean::Crafting, CoreDisposition.Preferences.Crafting);
            AddProfilePair(LLObserverKorean::Gathering, CoreDisposition.Preferences.Gathering, LLObserverKorean::Comfort, CoreDisposition.Preferences.Comfort);
            AddProfilePair(LLObserverKorean::Novelty, CoreDisposition.Preferences.Novelty, LLObserverKorean::Order, CoreDisposition.Preferences.Order);
            break;
        }

        case ELLDetailTab::Emotion:
        {
            if (!bHasCoreResident)
            {
                FRow None; None.Left = LLObserverKorean::CoreUnavailable; None.Scale = RowScale; Rows.Add(None);
                break;
            }

            auto AddEmotion = [&Rows, RowScale](const TCHAR* Name, float Value)
            {
                FRow Row;
                Row.Left = Name;
                Row.Right = Percent01(Value);
                Row.RightColor = TextPrimary;
                Row.Scale = RowScale;
                Rows.Add(Row);
            };
            AddEmotion(LLObserverKorean::Joy, CoreResident.Emotion.Joy);
            AddEmotion(LLObserverKorean::Sadness, CoreResident.Emotion.Sadness);
            AddEmotion(LLObserverKorean::Anger, CoreResident.Emotion.Anger);
            AddEmotion(LLObserverKorean::Fear, CoreResident.Emotion.Fear);
            AddEmotion(LLObserverKorean::Affection, CoreResident.Emotion.Affection);
            AddEmotion(LLObserverKorean::Anxiety, CoreResident.Emotion.Anxiety);
            AddEmotion(LLObserverKorean::Grief, CoreResident.Emotion.Grief);
            AddEmotion(LLObserverKorean::Pride, CoreResident.Emotion.Pride);
            AddEmotion(LLObserverKorean::Jealousy, CoreResident.Emotion.Jealousy);
            AddEmotion(LLObserverKorean::Relief, CoreResident.Emotion.Relief);
            AddEmotion(LLObserverKorean::Embarrassment, CoreResident.Emotion.Embarrassment);
            break;
        }

        case ELLDetailTab::Relationships:
        {
            if (!bHasCoreResident || CoreResident.Relationships.Num() == 0)
            {
                FRow None; None.Left = LLObserverText::NoneListed; None.Scale = RowScale; Rows.Add(None);
                break;
            }

            TArray<FLLCoreRelationshipSnapshot> Relations = CoreResident.Relationships;
            Relations.Sort([](const FLLCoreRelationshipSnapshot& A, const FLLCoreRelationshipSnapshot& B)
            {
                return A.SocialBond > B.SocialBond;
            });

            for (const FLLCoreRelationshipSnapshot& Relation : Relations)
            {
                FRow Header;
                Header.Left = Relation.TargetName.IsEmpty() ? LLObserverKorean::Resident : Relation.TargetName;
                Header.Right = FString::Printf(TEXT("%s %s · %s %s"), LLObserverKorean::Bond, *Scalar02(Relation.SocialBond), LLObserverKorean::Romance, *Scalar02(Relation.RomancePotential));
                Header.RightColor = TextPrimary;
                Header.Scale = RowScale;
                Header.GapBefore = Rows.Num() > 0 ? SectionGap : 0.0f;
                Rows.Add(Header);

                FRow Social;
                Social.Left = FString::Printf(TEXT("%s %s · %s %s · %s %s · %s %s · %s %s"),
                    LLObserverKorean::Affection, *Scalar02(Relation.Affection), LLObserverKorean::Trust, *Scalar02(Relation.Trust),
                    LLObserverKorean::Respect, *Scalar02(Relation.Respect), LLObserverKorean::Comfort, *Scalar02(Relation.Comfort),
                    LLObserverKorean::Familiarity, *Scalar02(Relation.Familiarity));
                Social.Scale = SectionScale;
                Rows.Add(Social);

                FRow Attraction;
                Attraction.Left = FString::Printf(TEXT("%s %s · %s %s · %s %s · %s %s"),
                    LLObserverKorean::Attraction, *Scalar02(Relation.Attraction), LLObserverKorean::RomanticInterest, *Scalar02(Relation.RomanticInterest),
                    LLObserverKorean::SexualAttraction, *Scalar02(Relation.SexualAttraction), LLObserverKorean::Commitment, *Scalar02(Relation.Commitment));
                Attraction.Scale = SectionScale;
                Rows.Add(Attraction);

                FRow Friction;
                Friction.Left = FString::Printf(TEXT("%s %s · %s %s · %s %s · %s %s"),
                    LLObserverKorean::Conflict, *Scalar02(Relation.Conflict), LLObserverKorean::Jealousy, *Scalar02(Relation.Jealousy),
                    LLObserverKorean::Fear, *Scalar02(Relation.Fear), LLObserverKorean::Grudge, *Scalar02(Relation.Grudge));
                Friction.Scale = SectionScale;
                Rows.Add(Friction);
            }
            break;
        }

        case ELLDetailTab::Family:
        {
            if (!bHasCoreFamily)
            {
                FRow None; None.Left = LLObserverKorean::CoreUnavailable; None.Scale = RowScale; Rows.Add(None);
                break;
            }

            FRow Household;
            Household.Left = LLObserverKorean::Household;
            Household.Right = CoreFamily.HouseholdId != 0
                ? FString::Printf(TEXT("#%lld"), static_cast<long long>(CoreFamily.HouseholdId))
                : FString(LLObserverKorean::None);
            Household.Scale = RowScale;
            Rows.Add(Household);

            FRow Partner;
            Partner.Left = LLObserverKorean::Partner;
            Partner.Right = CoreFamily.bHasActivePartner
                ? CoreFamily.PartnerName + TEXT(" · ") + EnumLabel(CoreFamily.PartnerStage)
                : FString(LLObserverKorean::None);
            Partner.Scale = RowScale;
            Rows.Add(Partner);

            if (CoreFamily.bExpectingChild)
            {
                FRow Expecting;
                Expecting.Left = LLObserverKorean::ExpectingChild;
                Expecting.Right = CoreFamily.PregnancyPartnerName;
                Expecting.RightColor = TextAction;
                Expecting.Scale = RowScale;
                Rows.Add(Expecting);
            }

            FRow Parents; Parents.Left = LLObserverKorean::Parents; Parents.Right = JoinFamilyNames(CoreFamily.Parents); Parents.Scale = RowScale; Parents.GapBefore = SectionGap; Rows.Add(Parents);
            FRow Children; Children.Left = LLObserverKorean::Children; Children.Right = JoinFamilyNames(CoreFamily.Children); Children.Scale = RowScale; Rows.Add(Children);
            FRow Siblings; Siblings.Left = LLObserverKorean::Siblings; Siblings.Right = JoinFamilyNames(CoreFamily.Siblings); Siblings.Scale = RowScale; Rows.Add(Siblings);
            break;
        }

        case ELLDetailTab::Civilization:
        {
            if (!bHasCoreCivilization)
            {
                FRow None; None.Left = LLObserverKorean::CoreUnavailable; None.Scale = RowScale; Rows.Add(None);
                break;
            }

            FRow Gathering; Gathering.Left = LLObserverKorean::Gathering; Gathering.Right = Percent01(CoreCivilization.GatheringSkill); Gathering.Scale = RowScale; Rows.Add(Gathering);
            FRow Crafting; Crafting.Left = LLObserverKorean::Crafting; Crafting.Right = Percent01(CoreCivilization.CraftingSkill); Crafting.Scale = RowScale; Rows.Add(Crafting);
            FRow Learning; Learning.Left = LLObserverKorean::Learning; Learning.Right = Percent01(CoreCivilization.LearningSkill); Learning.Scale = RowScale; Rows.Add(Learning);
            FRow Carried; Carried.Left = LLObserverKorean::CarriedUnits; Carried.Right = FString::FromInt(CoreCivilization.TotalInventoryUnits); Carried.Scale = RowScale; Carried.GapBefore = SectionGap; Rows.Add(Carried);
            FRow Known; Known.Left = LLObserverKorean::KnownTechniques; Known.Right = FString::FromInt(CoreCivilization.KnownTechniqueCount); Known.Scale = RowScale; Rows.Add(Known);
            FRow Reproducible; Reproducible.Left = LLObserverKorean::Reproducible; Reproducible.Right = FString::FromInt(CoreCivilization.ReproducibleTechniqueCount); Reproducible.Scale = RowScale; Rows.Add(Reproducible);

            FRow InventoryHeader; InventoryHeader.Left = LLObserverKorean::Inventory; InventoryHeader.LeftColor = TextSection; InventoryHeader.Scale = SectionScale; InventoryHeader.GapBefore = SectionGap; Rows.Add(InventoryHeader);
            if (CoreCivilization.Inventory.Num() == 0)
            {
                FRow None; None.Left = LLObserverText::NoneListed; None.Scale = RowScale; Rows.Add(None);
            }
            else
            {
                for (const FLLCoreCivilizationItemStack& Stack : CoreCivilization.Inventory)
                {
                    FRow Row;
                    Row.Left = EnumLabel(Stack.Item) + TEXT(" · ") + EnumLabel(Stack.Material);
                    Row.Right = FString::Printf(TEXT("x%d"), Stack.Quantity);
                    Row.RightColor = TextPrimary;
                    Row.Scale = RowScale;
                    Rows.Add(Row);
                }
            }

            FRow TechniqueHeader; TechniqueHeader.Left = LLObserverKorean::Techniques; TechniqueHeader.LeftColor = TextSection; TechniqueHeader.Scale = SectionScale; TechniqueHeader.GapBefore = SectionGap; Rows.Add(TechniqueHeader);
            if (CoreCivilization.Techniques.Num() == 0)
            {
                FRow None; None.Left = LLObserverText::NoneListed; None.Scale = RowScale; Rows.Add(None);
            }
            else
            {
                for (const FLLCoreTechniqueKnowledgeObservation& Technique : CoreCivilization.Techniques)
                {
                    FRow Row;
                    Row.Left = EnumLabel(Technique.Technique);
                    Row.Right = EnumLabel(Technique.Level);
                    if (Technique.bHasProvenance)
                    {
                        Row.Right += TEXT(" · ") + EnumLabel(Technique.Source);
                    }
                    Row.RightColor = TextPrimary;
                    Row.Scale = RowScale;
                    Rows.Add(Row);
                }
            }
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

    const float DesiredHeight = Pad + BackRowHeight + TabBarHeight + Gap + ContentHeight + Pad;
    const float PanelHeight = FMath::Min(DesiredHeight, FMath::Max(0.0f, MaxPanelBottom - PanelY));

    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, DetailAlpha * PanelFade), PanelX, PanelY, PanelWidth, PanelHeight);
    DetailPanelRect = FBox2D(FVector2D(PanelX, PanelY), FVector2D(PanelX + PanelWidth, PanelY + PanelHeight));

    const float TextX = PanelX + InnerPadX;

    const float BackRowTop = PanelY + Pad;
    DrawText(BackText, Faded(TextMuted), TextX, BackRowTop + (BackRowHeight - BackH) * 0.5f, Font, TabScale, false);
    DetailBackRect = FBox2D(FVector2D(PanelX, BackRowTop), FVector2D(PanelX + PanelWidth, BackRowTop + BackRowHeight));

    const float TabBarTop = BackRowTop + BackRowHeight;
    for (const FTabBox& Box : Tabs)
    {
        const float BoxX = TextX + Box.X;
        const float BoxY = TabBarTop + Box.Y;
        const float BoxW = FMath::Max(Box.W + 2.0f * TabPadXs, MinTouch);
        const float BoxH = FMath::Max(Box.H + 2.0f * TabPadYs, MinTouch);
        const bool bActive = Box.Tab == ActiveTab;
        const float TitleX = BoxX + (BoxW - Box.W) * 0.5f;
        const float TitleY = BoxY + (BoxH - Box.H) * 0.5f;

        DrawText(Box.Title, Faded(bActive ? TextPrimary : TextMuted), TitleX, TitleY, Font, TabScale, false);
        if (bActive)
        {
            DrawRect(Faded(TabActiveLine), TitleX, BoxY + BoxH - TabUnderline * UIScale, Box.W, TabUnderline * UIScale);
        }
        DetailTabRects[static_cast<int32>(Box.Tab)] = FBox2D(FVector2D(BoxX, BoxY), FVector2D(BoxX + BoxW, BoxY + BoxH));
    }

    const float ContentTop = TabBarTop + TabBarHeight + Gap;
    const float PanelBottom = PanelY + PanelHeight - Pad;
    const float ContentViewportHeight = FMath::Max(0.0f, PanelBottom - ContentTop);
    DetailScrollMax = FMath::Max(0.0f, ContentHeight - ContentViewportHeight);
    DetailScrollOffset = FMath::Clamp(DetailScrollOffset, 0.0f, DetailScrollMax);
    if (ContentViewportHeight > 0.0f)
    {
        DetailContentRect = FBox2D(FVector2D(PanelX, ContentTop), FVector2D(PanelX + PanelWidth, PanelBottom));
    }
    else
    {
        DetailContentRect = FBox2D(ForceInit);
    }

    float CursorY = ContentTop - DetailScrollOffset;
    for (const FLine& Line : Lines)
    {
        CursorY += Line.GapBefore;
        const float LineTop = CursorY;
        const float LineBottom = LineTop + Line.Height;
        if (LineTop >= ContentTop && LineBottom <= PanelBottom)
        {
            DrawText(Line.Left, Faded(Line.LeftColor), TextX, LineTop, Font, Line.Scale, false);
            if (!Line.Right.IsEmpty())
            {
                DrawText(Line.Right, Faded(Line.RightColor), TextX + RightColumnX, LineTop, Font, Line.Scale, false);
            }
        }
        CursorY = LineBottom + Gap;
    }

    if (DetailScrollMax > KINDA_SMALL_NUMBER && ContentViewportHeight > 0.0f)
    {
        const float TrackWidth = FMath::Max(2.0f, 2.0f * UIScale);
        const float TrackX = PanelX + PanelWidth - TrackWidth - 2.0f * UIScale;
        const float TrackHeight = ContentViewportHeight;
        DrawRect(Faded(FLinearColor(0.65f, 0.70f, 0.78f, 0.22f)), TrackX, ContentTop, TrackWidth, TrackHeight);

        const float ThumbHeight = FMath::Clamp(
            TrackHeight * (ContentViewportHeight / FMath::Max(ContentHeight, ContentViewportHeight)),
            FMath::Min(24.0f * UIScale, TrackHeight),
            TrackHeight);
        const float Travel = FMath::Max(0.0f, TrackHeight - ThumbHeight);
        const float ThumbY = ContentTop + Travel * (DetailScrollOffset / DetailScrollMax);
        DrawRect(Faded(FLinearColor(0.78f, 0.86f, 1.0f, 0.72f)), TrackX, ThumbY, TrackWidth, ThumbHeight);
    }
}
