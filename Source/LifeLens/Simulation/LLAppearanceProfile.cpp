#include "Simulation/LLAppearanceProfile.h"

namespace
{
uint64 MixAppearance64(uint64 Value)
{
    Value += 0x9E3779B97F4A7C15ull;
    Value = (Value ^ (Value >> 30)) * 0xBF58476D1CE4E5B9ull;
    Value = (Value ^ (Value >> 27)) * 0x94D049BB133111EBull;
    return Value ^ (Value >> 31);
}

uint64 GuidSeed(const FGuid& Id)
{
    const uint64 High = (static_cast<uint64>(Id.A) << 32) | static_cast<uint64>(Id.B);
    const uint64 Low = (static_cast<uint64>(Id.C) << 32) | static_cast<uint64>(Id.D);
    return MixAppearance64(High ^ MixAppearance64(Low));
}

float UnitAxis(uint64 Base, uint64 Salt)
{
    const uint64 Mixed = MixAppearance64(Base ^ Salt);
    const uint32 Value = static_cast<uint32>(Mixed & 0x00FFFFFFull);
    return static_cast<float>(Value) / static_cast<float>(0x00FFFFFFu);
}

int32 Variant(uint64 Base, uint64 Salt, int32 Count)
{
    if (Count <= 1)
    {
        return 0;
    }
    return static_cast<int32>(MixAppearance64(Base ^ Salt) % static_cast<uint64>(Count));
}
}

FLLAppearanceProfile ULLAppearanceProfileLibrary::MakeDeterministicAppearanceProfile(
    FGuid ResidentId,
    ELLCoreSex Sex,
    ELLCoreLifeStage LifeStage)
{
    FLLAppearanceProfile Result;
    Result.ResidentId = ResidentId;
    Result.Sex = Sex;
    Result.LifeStage = LifeStage;

    if (!ResidentId.IsValid())
    {
        return Result;
    }

    const uint64 Base = GuidSeed(ResidentId);
    Result.VisualSeed = static_cast<int32>((Base & 0x7FFFFFFFull) + 1ull);

    Result.FaceAxis = UnitAxis(Base, 0x46414345ull);
    Result.SkinToneAxis = UnitAxis(Base, 0x534B494Eull);
    Result.EyeColorAxis = UnitAxis(Base, 0x45594553ull);
    Result.HairColorAxis = UnitAxis(Base, 0x48414952ull);
    Result.HeightAxis = UnitAxis(Base, 0x484549474854ull);
    Result.BuildAxis = UnitAxis(Base, 0x4255494C44ull);

    Result.FaceVariant = Variant(Base, 0x46414345564152ull, 8);
    Result.HairStyleVariant = Variant(Base, 0x48414952564152ull, 12);
    Result.OutfitVariant = Variant(Base, 0x4F5554464954ull, 8);
    return Result;
}

FLLAppearanceProfile ULLAppearanceProfileLibrary::MakeGeneticAppearanceProfile(
    FGuid ResidentId,
    ELLCoreSex Sex,
    ELLCoreLifeStage LifeStage,
    const FLLCoreGeneticsSnapshot& Genetics)
{
    FLLAppearanceProfile Result =
        MakeDeterministicAppearanceProfile(ResidentId, Sex, LifeStage);

    // Inheritable phenotype comes directly from Core. Identity hashing remains
    // only for style/outfit catalogue choices that are not genetic truth.
    Result.FaceAxis = FMath::Clamp(Genetics.FaceShape, 0.0f, 1.0f);
    Result.SkinToneAxis = FMath::Clamp(Genetics.SkinTone, 0.0f, 1.0f);
    Result.EyeColorAxis = FMath::Clamp(Genetics.EyePigment, 0.0f, 1.0f);
    Result.HairColorAxis = FMath::Clamp(Genetics.HairPigment, 0.0f, 1.0f);
    Result.HeightAxis = FMath::Clamp(Genetics.HeightPotential, 0.0f, 1.0f);
    Result.BuildAxis = FMath::Clamp(Genetics.BuildPotential, 0.0f, 1.0f);
    return Result;
}
