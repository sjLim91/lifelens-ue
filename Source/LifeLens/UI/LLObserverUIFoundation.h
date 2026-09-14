#pragma once

#include "CoreMinimal.h"

class UCanvas;
class UFont;

// Observer UI foundation for Android landscape (Issue #24, DQ-01).
// Single source of the layout, typography, touch and density rules the
// observer HUD panels follow. Read-only helpers; no simulation access.
namespace LLObserverUIFoundation
{
    constexpr float ReferenceShortSide = 540.0f;
    constexpr float MinUIScale = 1.0f;
    constexpr float MaxUIScale = 2.5f;
    float ComputeUIScale(const UCanvas* Canvas);

    struct FSafeArea
    {
        float Left = 0.0f;
        float Top = 0.0f;
        float Right = 0.0f;
        float Bottom = 0.0f;
    };
    FSafeArea GetSafeArea(const UCanvas* Canvas, float UIScale);

    constexpr float TextTitle = 1.15f;
    constexpr float TextBody = 1.00f;
    constexpr float TextSecondary = 0.90f;
    constexpr float TextCaption = 0.85f;
    constexpr float TextHint = 0.80f;

    constexpr float Space1 = 4.0f;
    constexpr float Space2 = 8.0f;
    constexpr float Space3 = 12.0f;
    constexpr float Space4 = 16.0f;
    constexpr float Space6 = 24.0f;

    constexpr float ScreenMargin = Space4;
    constexpr float PanelPadX = Space3;
    constexpr float PanelPadY = Space2;
    constexpr float LineGap = 6.0f;
    constexpr float SectionGap = Space3;

    constexpr float QuickPanelMaxRatio = 0.42f;
    constexpr float DetailPanelMaxRatio = 0.60f;
    constexpr float QuickPanelMaxWidth = 360.0f;
    constexpr float QuickPanelMinWidth = 200.0f;
    constexpr float DetailPanelMaxWidth = 520.0f;
    constexpr float DetailPanelMinWidth = 280.0f;

    constexpr float OverviewBandAlpha = 0.22f;
    constexpr float QuickPanelAlpha = 0.45f;
    constexpr float DetailPanelAlpha = 0.60f;

    constexpr float MinTouchTarget = 48.0f;
    float TouchTargetSize(float UIScale);
    FBox2D ExpandToTouchTarget(const FBox2D& Rect, float UIScale);

    enum class EStateTone : uint8
    {
        Primary,
        Secondary,
        Muted,
        Hint,
        Action,
        Section,
        Attention,
        Critical,
        Accent
    };
    FLinearColor ToneColor(EStateTone Tone);

    constexpr int32 MaxOverviewStripItems = 8;
    constexpr int32 MaxDetailRows = 24;
    constexpr int32 MaxLevel0Lines = 2;

    constexpr const TCHAR* HUDFontAssetPath = TEXT("/Game/UI/Fonts/F_LifeLensHUD.F_LifeLensHUD");
    UFont* GetHUDFont();
}
