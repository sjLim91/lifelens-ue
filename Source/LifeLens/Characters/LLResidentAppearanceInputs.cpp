#include "Characters/LLResidentAppearanceInputs.h"

namespace
{
    // SplitMix64 finaliser: cheap, well-distributed, platform-independent.
    uint64 Mix64(uint64 Value)
    {
        Value += 0x9E3779B97F4A7C15ull;
        Value = (Value ^ (Value >> 30)) * 0xBF58476D1CE4E5B9ull;
        Value = (Value ^ (Value >> 27)) * 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }

    uint64 SeedFrom(int32 WorldSeed, const FGuid& ResidentId)
    {
        const uint64 High = (static_cast<uint64>(ResidentId.A) << 32) | static_cast<uint32>(ResidentId.B);
        const uint64 Low = (static_cast<uint64>(ResidentId.C) << 32) | static_cast<uint32>(ResidentId.D);
        return Mix64(static_cast<uint64>(static_cast<uint32>(WorldSeed)) ^ Mix64(High) ^ Mix64(Low ^ 0xA5A5A5A5A5A5A5A5ull));
    }

    float Axis(uint64 Base, uint64 Salt)
    {
        return static_cast<float>(Mix64(Base ^ Salt) % 10000ull) / 9999.0f;
    }

    int32 Variant(uint64 Base, uint64 Salt, int32 Count)
    {
        return Count > 0 ? static_cast<int32>(Mix64(Base ^ Salt) % static_cast<uint64>(Count)) : 0;
    }
}

FLLResidentAppearanceInputs ULLResidentAppearanceInputSource::MakeTemporaryAppearanceInputs(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage)
{
    // TEMPORARY: presentation-only seed until PR #65 (FLLAppearanceProfile).
    FLLResidentAppearanceInputs Inputs;
    Inputs.ResidentId = ResidentId;
    Inputs.Sex = Sex;
    Inputs.LifeStage = LifeStage;

    const uint64 Base = SeedFrom(WorldSeed, ResidentId);
    Inputs.VisualSeed = static_cast<int32>(Base & 0x7FFFFFFFull);
    if (Inputs.VisualSeed == 0)
    {
        Inputs.VisualSeed = 1;
    }

    Inputs.FaceAxis      = Axis(Base, 0x01);
    Inputs.SkinToneAxis  = Axis(Base, 0x02);
    Inputs.EyeColorAxis  = Axis(Base, 0x03);
    Inputs.HairColorAxis = Axis(Base, 0x04);
    Inputs.HeightAxis    = Axis(Base, 0x05);
    Inputs.BuildAxis     = Axis(Base, 0x06);

    // Large variant spaces; consumers wrap to their catalogue sizes.
    Inputs.FaceVariant      = Variant(Base, 0x11, 1024);
    Inputs.HairStyleVariant = Variant(Base, 0x12, 1024);
    Inputs.OutfitVariant    = Variant(Base, 0x13, 1024);

    Inputs.bTemporaryPresentationSeed = true;
    return Inputs;
}

FLLResidentAppearanceInputs ULLResidentAppearanceInputSource::Resolve(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage)
{
    // Swap point for PR #65: return a 1:1 mapping of
    // ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(ResidentId, Sex, LifeStage)
    // once it exists on main. Until then the temporary presentation seed is used.
    return MakeTemporaryAppearanceInputs(WorldSeed, ResidentId, Sex, LifeStage);
}
