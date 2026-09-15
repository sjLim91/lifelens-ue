#include "Characters/LLLocomotionBlendSpaceBuilder.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"

int32 ULLLocomotionBlendSpaceBuilder::BuildSpeedBlendSpace(UBlendSpace* BlendSpace,
                                                           const TArray<UAnimSequence*>& Animations,
                                                           const TArray<float>& Speeds)
{
#if WITH_EDITOR
    if (!BlendSpace || Animations.Num() == 0 || Animations.Num() != Speeds.Num())
    {
        return 0;
    }

    BlendSpace->Modify();

    for (int32 Index = 0; Index < Animations.Num(); ++Index)
    {
        if (!Animations[Index])
        {
            return 0;
        }
        BlendSpace->AddSample(Animations[Index], FVector(Speeds[Index], 0.0f, 0.0f));
    }

    BlendSpace->ValidateSampleData();
    BlendSpace->ResampleData();
    BlendSpace->MarkPackageDirty();

    return BlendSpace->GetBlendSamples().Num();
#else
    return 0;
#endif
}

int32 ULLLocomotionBlendSpaceBuilder::CountResolvedSamples(UBlendSpace* BlendSpace, float Speed)
{
    if (!BlendSpace)
    {
        return 0;
    }
    TArray<FBlendSampleData> Resolved;
    int32 CachedTriangulationIndex = INDEX_NONE;
    BlendSpace->GetSamplesFromBlendInput(FVector(Speed, 0.0f, 0.0f), Resolved, CachedTriangulationIndex, true);
    return Resolved.Num();
}
