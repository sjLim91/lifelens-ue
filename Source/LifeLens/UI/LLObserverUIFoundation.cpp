#include "UI/LLObserverUIFoundation.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GenericPlatform/GenericApplication.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtr.h"

namespace LLObserverUIFoundation
{
    float ComputeUIScale(const UCanvas* Canvas)
    {
        if (!Canvas)
        {
            return MinUIScale;
        }
        const float ShortSide = FMath::Min(Canvas->ClipX, Canvas->ClipY);
        return FMath::Clamp(ShortSide / ReferenceShortSide, MinUIScale, MaxUIScale);
    }

    FSafeArea GetSafeArea(const UCanvas* Canvas, float UIScale)
    {
        FSafeArea SafeArea;
        const float Margin = ScreenMargin * UIScale;
        SafeArea.Left = Margin;
        SafeArea.Top = Margin;
        SafeArea.Right = Margin;
        SafeArea.Bottom = Margin;

        if (Canvas)
        {
            // TitleSafePaddingSize is (Left, Top, Right, Bottom) in display pixels.
            FDisplayMetrics Metrics;
            FDisplayMetrics::RebuildDisplayMetrics(Metrics);
            const FVector4& Padding = Metrics.TitleSafePaddingSize;
            SafeArea.Left = FMath::Max(SafeArea.Left, static_cast<float>(Padding.X));
            SafeArea.Top = FMath::Max(SafeArea.Top, static_cast<float>(Padding.Y));
            SafeArea.Right = FMath::Max(SafeArea.Right, static_cast<float>(Padding.Z));
            SafeArea.Bottom = FMath::Max(SafeArea.Bottom, static_cast<float>(Padding.W));
        }
        return SafeArea;
    }

    float TouchTargetSize(float UIScale)
    {
        return MinTouchTarget * UIScale;
    }

    FBox2D ExpandToTouchTarget(const FBox2D& Rect, float UIScale)
    {
        if (!Rect.bIsValid)
        {
            return Rect;
        }
        const float Target = TouchTargetSize(UIScale);
        const FVector2D Size = Rect.GetSize();
        const float GrowX = FMath::Max(0.0f, (Target - static_cast<float>(Size.X)) * 0.5f);
        const float GrowY = FMath::Max(0.0f, (Target - static_cast<float>(Size.Y)) * 0.5f);
        return FBox2D(Rect.Min - FVector2D(GrowX, GrowY), Rect.Max + FVector2D(GrowX, GrowY));
    }

    FLinearColor ToneColor(EStateTone Tone)
    {
        switch (Tone)
        {
            case EStateTone::Primary:   return FLinearColor(1.00f, 1.00f, 1.00f, 1.00f);
            case EStateTone::Secondary: return FLinearColor(0.88f, 0.90f, 0.94f, 1.00f);
            case EStateTone::Muted:     return FLinearColor(0.80f, 0.84f, 0.90f, 0.55f);
            case EStateTone::Hint:      return FLinearColor(0.80f, 0.84f, 0.90f, 0.35f);
            case EStateTone::Action:    return FLinearColor(0.75f, 0.90f, 1.00f, 1.00f);
            case EStateTone::Section:   return FLinearColor(0.70f, 0.78f, 0.90f, 0.85f);
            case EStateTone::Attention: return FLinearColor(1.00f, 0.72f, 0.35f, 1.00f);
            case EStateTone::Critical:  return FLinearColor(1.00f, 0.42f, 0.38f, 1.00f);
            case EStateTone::Accent:    return FLinearColor(0.75f, 0.90f, 1.00f, 0.90f);
            default:                    return FLinearColor::White;
        }
    }

    UFont* GetHUDFont()
    {
        static TWeakObjectPtr<UFont> CachedFont;
        static bool bTriedLoad = false;

        if (!bTriedLoad)
        {
            bTriedLoad = true;
            CachedFont = LoadObject<UFont>(nullptr, HUDFontAssetPath);
        }

        if (CachedFont.IsValid())
        {
            return CachedFont.Get();
        }
        return GEngine ? GEngine->GetSmallFont() : nullptr;
    }
}
