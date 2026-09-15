#include "UI/LLObserverHUD.h"
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
    FVector2D OriginScreen;
    if (!Actor || !PlayerController->ProjectWorldLocationToScreen(Actor->GetActorLocation(), OriginScreen, false))
    {
        return;
    }

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PlayerController->GetViewportSize(ViewportX, ViewportY);
    const FVector2D Origin = ViewportToCanvas(OriginScreen, FVector2D(ViewportX, ViewportY));
    const float Radius = TouchTargetRadiusPixels(this);

    // Small persistent focus mark while LEVEL 1/2 is active.
    const float UnderY = Origin.Y + Radius * 0.5f;
    DrawLine(Origin.X - Radius * 0.5f, UnderY, Origin.X + Radius * 0.5f, UnderY, FocusColor, FocusUnderline * UIScale);

    // Brief flash when the observed resident changes.
    const float Now = GetWorld()->GetTimeSeconds();
    const float T = (Now - SelectionChangeTime) / SelectFlashSeconds;
    if (T < 0.0f || T >= 1.0f)
    {
        return;
    }

    const float Half = Radius * 0.4f + (FlashPadStart + FlashPadGrow * T) * UIScale;
    FLinearColor Color = FocusColor;
    Color.A *= (1.0f - T) * 0.8f;
    const FBox2D Flash(Origin - FVector2D(Half, Half), Origin + FVector2D(Half, Half));
    DrawLine(Flash.Min.X, Flash.Min.Y, Flash.Max.X, Flash.Min.Y, Color, 1.0f);
    DrawLine(Flash.Max.X, Flash.Min.Y, Flash.Max.X, Flash.Max.Y, Color, 1.0f);
    DrawLine(Flash.Max.X, Flash.Max.Y, Flash.Min.X, Flash.Max.Y, Color, 1.0f);
    DrawLine(Flash.Min.X, Flash.Max.Y, Flash.Min.X, Flash.Min.Y, Color, 1.0f);
}
