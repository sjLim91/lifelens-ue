from pathlib import Path

root = Path(__file__).resolve().parents[1]

types = (root / "Source/LifeLens/Core/LLTypes.h").read_text(encoding="utf-8")
projection = (root / "Source/LifeLens/Simulation/LLSimulationSubsystem.cpp").read_text(encoding="utf-8")
labels = (root / "Source/LifeLens/UI/LLObserverLabels.h").read_text(encoding="utf-8")

assert "float Thirst" in types, "FLLNeedState must expose authoritative Core thirst"
assert "Resident.Needs.Thirst = ToLegacyNeed(CoreResident.Needs.Thirst);" in projection, \
    "Core thirst must be projected into the observer DTO"
assert "case ELLActionIntent::Drink: Resident->Needs.Thirst" in projection, \
    "Drink compatibility must update thirst rather than fall through to idle/energy"

need_rows = labels.split("inline TArray<FNeedRow> NeedRows", 1)[1].split("// A named personality axis", 1)[0]
assert "NeedNameThirst" in need_rows and "Needs.Thirst" in need_rows, \
    "Observer need rows must show real thirst"
assert "Needs.Social" not in need_rows and "Needs.Fun" not in need_rows, \
    "Compatibility-only Social/Fun values must not be presented as authoritative needs"

summary = labels.split("inline FString StatusSummary(const FLLNeedState& Needs, ENeedLevel& OutWorstLevel)", 1)[1]
summary = summary.split("inline FString StatusSummary(const FLLNeedState& Needs)", 1)[0]
assert "Needs.Thirst" in summary and "ThirstPhrases" in summary, \
    "Status summary must include authoritative thirst"
assert "Needs.Social" not in summary and "Needs.Fun" not in summary, \
    "Status summary must not fabricate loneliness/boredom from compatibility placeholders"

print("Observer projection need contract: PASS")
