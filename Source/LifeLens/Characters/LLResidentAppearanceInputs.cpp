#include "Characters/LLResidentAppearanceInputs.h"
#include "Simulation/LLAppearanceProfile.h"

// Named (not anonymous) namespace: the unity build merges this file with
// Simulation/LLAppearanceProfile.cpp, which has its own anonymous-namespace
// helpers of the same name.
namespace LLAppearanceInputsHash
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

    const uint64 Base = LLAppearanceInputsHash::SeedFrom(WorldSeed, ResidentId);
    Inputs.VisualSeed = static_cast<int32>(Base & 0x7FFFFFFFull);
    if (Inputs.VisualSeed == 0)
    {
        Inputs.VisualSeed = 1;
    }

    Inputs.FaceAxis      = LLAppearanceInputsHash::Axis(Base, 0x01);
    Inputs.SkinToneAxis  = LLAppearanceInputsHash::Axis(Base, 0x02);
    Inputs.EyeColorAxis  = LLAppearanceInputsHash::Axis(Base, 0x03);
    Inputs.HairColorAxis = LLAppearanceInputsHash::Axis(Base, 0x04);
    Inputs.HeightAxis    = LLAppearanceInputsHash::Axis(Base, 0x05);
    Inputs.BuildAxis     = LLAppearanceInputsHash::Axis(Base, 0x06);

    // Large variant spaces; consumers wrap to their catalogue sizes.
    Inputs.FaceVariant      = LLAppearanceInputsHash::Variant(Base, 0x11, 1024);
    Inputs.HairStyleVariant = LLAppearanceInputsHash::Variant(Base, 0x12, 1024);
    Inputs.OutfitVariant    = LLAppearanceInputsHash::Variant(Base, 0x13, 1024);

    Inputs.bTemporaryPresentationSeed = true;
    return Inputs;
}

FLLResidentAppearanceInputs ULLResidentAppearanceInputSource::Resolve(int32 WorldSeed, FGuid ResidentId, ELLCoreSex Sex, ELLCoreLifeStage LifeStage)
{
    // Bridge contract (PR #65): the profile is a deterministic projection of the
    // stable ResidentId (WorldSeed + Core CharacterId) plus authoritative
    // Sex/LifeStage, so it survives Save/Load without any presentation cache.
    if (ResidentId.IsValid())
    {
        const FLLAppearanceProfile Profile = ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(ResidentId, Sex, LifeStage);

        FLLResidentAppearanceInputs Inputs;
        Inputs.ResidentId       = Profile.ResidentId;
        Inputs.Sex              = Profile.Sex;
        Inputs.LifeStage        = Profile.LifeStage;
        Inputs.VisualSeed       = Profile.VisualSeed;
        Inputs.FaceAxis         = Profile.FaceAxis;
        Inputs.SkinToneAxis     = Profile.SkinToneAxis;
        Inputs.EyeColorAxis     = Profile.EyeColorAxis;
        Inputs.HairColorAxis    = Profile.HairColorAxis;
        Inputs.HeightAxis       = Profile.HeightAxis;
        Inputs.BuildAxis        = Profile.BuildAxis;
        Inputs.FaceVariant      = Profile.FaceVariant;
        Inputs.HairStyleVariant = Profile.HairStyleVariant;
        Inputs.OutfitVariant    = Profile.OutfitVariant;
        Inputs.bTemporaryPresentationSeed = false;
        return Inputs;
    }

    // Fallback only when no stable identity is available (should not happen
    // for spawned residents): temporary presentation seed.
    return MakeTemporaryAppearanceInputs(WorldSeed, ResidentId, Sex, LifeStage);
}

FLLResidentAppearanceInputs ULLResidentAppearanceInputSource::ResolveWithGenetics(
    int32 WorldSeed,
    FGuid ResidentId,
    ELLCoreSex Sex,
    ELLCoreLifeStage LifeStage,
    const FLLCoreGeneticsSnapshot& Genetics)
{
    if (!ResidentId.IsValid())
    {
        return MakeTemporaryAppearanceInputs(WorldSeed, ResidentId, Sex, LifeStage);
    }

    const FLLAppearanceProfile Profile =
        ULLAppearanceProfileLibrary::MakeGeneticAppearanceProfile(
            ResidentId, Sex, LifeStage, Genetics);

    FLLResidentAppearanceInputs Inputs;
    Inputs.ResidentId       = Profile.ResidentId;
    Inputs.Sex              = Profile.Sex;
    Inputs.LifeStage        = Profile.LifeStage;
    Inputs.VisualSeed       = Profile.VisualSeed;
    Inputs.FaceAxis         = Profile.FaceAxis;
    Inputs.SkinToneAxis     = Profile.SkinToneAxis;
    Inputs.EyeColorAxis     = Profile.EyeColorAxis;
    Inputs.HairColorAxis    = Profile.HairColorAxis;
    Inputs.HeightAxis       = Profile.HeightAxis;
    Inputs.BuildAxis        = Profile.BuildAxis;
    Inputs.FaceVariant      = Profile.FaceVariant;
    Inputs.HairStyleVariant = Profile.HairStyleVariant;
    Inputs.OutfitVariant    = Profile.OutfitVariant;
    Inputs.bTemporaryPresentationSeed = false;
    return Inputs;
}
