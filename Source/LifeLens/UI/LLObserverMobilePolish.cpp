#include "UI/LLObserverHUD.h"
#include "Core/LLTypes.h"
#include "Characters/LLResidentCharacter.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "GameFramework/PlayerController.h"

namespace
{
    constexpr float ObserverMargin = 16.0f;
    constexpr float LevelFadeSeconds = 0.18f;
    constexpr float SelectFlashSeconds = 0.45f;
    constexpr float FocusUnderline = 2.0f;
    constexpr float FlashPadStart = 6.0f;
    constexpr float FlashPadGrow = 18.0f;

    const FLinearColor FocusColor(0.75f, 0.90f, 1.00f, 0.9f);

    ALLResidentCharacter* FindResidentActorForPolish(UWorld* World, const FGuid& ResidentId)
    {
        if (!World || !ResidentId.IsValid())
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

ALLObserverHUD::FSafeInsets ALLObserverHUD::SafeInsets(float UIScale) const
{
    FSafeInsets Insets;
    const float MarginPx = ObserverMargin * UIScale;
    Insets.Left = MarginPx;
    Insets.Top = MarginPx;
    Insets.Right = MarginPx;
    Insets.Bottom = MarginPx;

    if (FSlateApplication::IsInitialized())
    {
        FDisplayMetrics Metrics;
        FSlateApplication::Get().GetDisplayMetrics(Metrics);
        const FVector4& Padding = Metrics.TitleSafePaddingSize;
        Insets.Left = FMath::Max(Insets.Left, static_cast<float>(Padding.X));
        Insets.Top = FMath::Max(Insets.Top, static_cast<float>(Padding.Y));
        Insets.Right = FMath::Max(Insets.Right, static_cast<float>(Padding.Z));
        Insets.Bottom = FMath::Max(Insets.Bottom, static_cast<float>(Padding.W));
    }
    return Insets;
}

void ALLObserverHUD::UpdateFeedbackState(const ULLObservationSubsystem* Observation)
{
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const ELLObservationLevel Level = Observation ? Observation->GetObservationLevel() : ELLObservationLevel::World;
    const FGuid ObservedId = Observation ? Observation->GetObservedResidentId() : FGuid();

    if (Level != LastLevel)
    {
        LastLevel = Level;
        LevelChangeTime = Now;
    }
    if (ObservedId != LastObservedId)
    {
        LastObservedId = ObservedId;
        SelectionChangeTime = ObservedId.IsValid() ? Now : -100.0f;
    }

    PanelFade = FMath::Clamp((Now - LevelChangeTime) / LevelFadeSeconds, 0.0f, 1.0f);
}

void ALLObserverHUD::DrawSelectionFeedback(const FLLResidentData& Selected, float UIScale)
{
    const APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController || !GetWorld())
    {
        return;
    }

    const ALLResidentCharacter* Actor = FindResidentActorForPolish(GetWorld(), Selected.ResidentId);
    if (!Actor)
    {
        return;
    }

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PlayerController->GetViewportSize(ViewportX, ViewportY);
    const FVector2D ViewportSize(ViewportX, ViewportY);

    FBox2D BoundsScreen;
    FBox2D UnusedTapRect;
    if (!ProjectResidentTapRect(PlayerController, Actor, 0.0f, BoundsScreen, UnusedTapRect))
    {
        return;
    }

    FBox2D Bounds(
        ViewportToCanvas(BoundsScreen.Min, ViewportSize),
        ViewportToCanvas(BoundsScreen.Max, ViewportSize));
    if (!Bounds.bIsValid)
    {
        return;
    }

    // Keep the bracket close to the rendered resident, not the touch target.
    const float Pad = 9.0f * UIScale;
    Bounds.Min -= FVector2D(Pad, Pad);
    Bounds.Max += FVector2D(Pad, Pad);

    const float Width = Bounds.GetSize().X;
    const float Height = Bounds.GetSize().Y;
    const float Corner = FMath::Clamp(FMath::Min(Width, Height) * 0.24f, 10.0f * UIScale, 24.0f * UIScale);
    const float Stroke = FocusUnderline * UIScale;

    FLinearColor Persistent = FocusColor;
    Persistent.A *= 0.90f;

    auto DrawCorner = [this, Corner, Stroke](const FVector2D& P, const FVector2D& Horizontal, const FVector2D& Vertical, const FLinearColor& Color)
    {
        DrawLine(P.X, P.Y, P.X + Horizontal.X * Corner, P.Y + Horizontal.Y * Corner, Color, Stroke);
        DrawLine(P.X, P.Y, P.X + Vertical.X * Corner, P.Y + Vertical.Y * Corner, Color, Stroke);
    };

    DrawCorner(Bounds.Min, FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f), Persistent);
    DrawCorner(FVector2D(Bounds.Max.X, Bounds.Min.Y), FVector2D(-1.0f, 0.0f), FVector2D(0.0f, 1.0f), Persistent);
    DrawCorner(Bounds.Max, FVector2D(-1.0f, 0.0f), FVector2D(0.0f, -1.0f), Persistent);
    DrawCorner(FVector2D(Bounds.Min.X, Bounds.Max.Y), FVector2D(1.0f, 0.0f), FVector2D(0.0f, -1.0f), Persistent);

    // A small underline reinforces the feet direction while the existing
    // world-space selection ring remains the ground contact cue.
    const float UnderY = Bounds.Max.Y + 4.0f * UIScale;
    const float UnderHalf = FMath::Clamp(Width * 0.20f, 10.0f * UIScale, 28.0f * UIScale);
    const float CenterX = (Bounds.Min.X + Bounds.Max.X) * 0.5f;
    DrawLine(CenterX - UnderHalf, UnderY, CenterX + UnderHalf, UnderY, Persistent, Stroke);

    // Brief expanding bracket flash when the observed resident changes.
    const float Now = GetWorld()->GetTimeSeconds();
    const float T = (Now - SelectionChangeTime) / SelectFlashSeconds;
    if (T < 0.0f || T >= 1.0f)
    {
        return;
    }

    const float FlashPad = (FlashPadStart + FlashPadGrow * T) * UIScale;
    const FBox2D Flash(Bounds.Min - FVector2D(FlashPad), Bounds.Max + FVector2D(FlashPad));
    FLinearColor FlashColor = FocusColor;
    FlashColor.A *= (1.0f - T) * 0.72f;

    const float FlashCorner = Corner + 6.0f * UIScale;
    auto DrawFlashCorner = [this, FlashCorner](const FVector2D& P, const FVector2D& Horizontal, const FVector2D& Vertical, const FLinearColor& Color)
    {
        DrawLine(P.X, P.Y, P.X + Horizontal.X * FlashCorner, P.Y + Horizontal.Y * FlashCorner, Color, 1.0f);
        DrawLine(P.X, P.Y, P.X + Vertical.X * FlashCorner, P.Y + Vertical.Y * FlashCorner, Color, 1.0f);
    };

    DrawFlashCorner(Flash.Min, FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f), FlashColor);
    DrawFlashCorner(FVector2D(Flash.Max.X, Flash.Min.Y), FVector2D(-1.0f, 0.0f), FVector2D(0.0f, 1.0f), FlashColor);
    DrawFlashCorner(Flash.Max, FVector2D(-1.0f, 0.0f), FVector2D(0.0f, -1.0f), FlashColor);
    DrawFlashCorner(FVector2D(Flash.Min.X, Flash.Max.Y), FVector2D(1.0f, 0.0f), FVector2D(0.0f, -1.0f), FlashColor);
}
