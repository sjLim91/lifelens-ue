#pragma once

#include "UI/LLSocialObserverHUD.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "LLRuntimeObserverHUD.generated.h"

/**
 * Production Observer HUD composition layer.
 *
 * Resident/detail chrome stays in ALLObserverHUD and social overlays stay in
 * ALLSocialObserverHUD. This final layer owns global runtime chrome that is
 * neither resident-specific nor social-specific: authoritative calendar/
 * weather readout and simulation-speed controls.
 *
 * It consumes Core/Simulation providers only; it does not calculate a second
 * time or weather truth.
 */
UCLASS()
class LIFELENS_API ALLRuntimeObserverHUD : public ALLSocialObserverHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    virtual bool HandleTap(const FVector2D& ScreenPosition, const FVector2D& ViewportSize = FVector2D::ZeroVector) override;

private:
    void DrawRuntimeChrome();

    static constexpr int32 SpeedButtonCount = 5;
    FBox2D SpeedButtonRects[SpeedButtonCount];
};
