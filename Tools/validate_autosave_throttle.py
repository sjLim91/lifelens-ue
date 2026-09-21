#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/World/LLWorldDirector.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/World/LLWorldDirector.cpp").read_text(encoding="utf-8")
config = (root / "Config/DefaultGame.ini").read_text(encoding="utf-8")

for token in (
    "AutosaveRealSecondsSinceLastWrite",
    "bAutosavePending",
    "AutosaveSimulationMinutes = 60",
    "AutosaveMinimumRealSeconds = 10.0f",
):
    assert token in header, f"missing autosave throttle state/config: {token}"

for token in (
    "AutosaveRealSecondsSinceLastWrite += RealDeltaSeconds",
    "FMath::Max(1, AutosaveSimulationMinutes)",
    "bAutosavePending = true",
    "FMath::Max(1.0f, AutosaveMinimumRealSeconds)",
    "bAutosavePending",
    "AutosaveRealSecondsSinceLastWrite >= MinimumAutosaveInterval",
    "const bool bSaved = Simulation->SaveGame()",
    "AutosaveRealSecondsSinceLastWrite = 0.0f",
):
    assert token in cpp, f"missing autosave coalescing behavior: {token}"

assert "if ((Simulation->GetSimulationMinute() % 60) == 0)" not in cpp, (
    "high-speed simulation must not write a full save directly at every simulated hour"
)
assert "Simulation->SaveGame();\n            }" not in cpp, (
    "autosave write must remain outside the per-minute catch-up loop"
)

# Runtime replacement must discard any pending write from the old snapshot.
replacement_start = cpp.index("void ALLWorldDirector::SynchronizeAfterCoreRuntimeReplacement")
replacement_end = cpp.index("void ALLWorldDirector::RefreshCorePresentationOrigin", replacement_start)
replacement = cpp[replacement_start:replacement_end]
for token in (
    "AutosaveRealSecondsSinceLastWrite = 0.0f",
    "bAutosavePending = false",
):
    assert token in replacement, f"runtime replacement misses autosave reset: {token}"

for token in (
    "AutosaveSimulationMinutes=60",
    "AutosaveMinimumRealSeconds=10.000000",
):
    assert token in config, f"missing production autosave throttle config: {token}"

# Failed writes keep the pending request but reset the real-time timer, preventing
# one failing disk write attempt per render frame.
save_start = cpp.index("const bool bSaved = Simulation->SaveGame()")
save_end = cpp.index("if (bAdvancedSimulation)", save_start)
save_block = cpp[save_start:save_end]
assert "AutosaveRealSecondsSinceLastWrite = 0.0f" in save_block
assert "if (bSaved)" in save_block
assert "bAutosavePending = false" in save_block

# Mobile/application lifecycle must flush the authoritative Core snapshot before
# the process can be suspended or destroyed. This complements, rather than
# bypasses, the normal high-speed write throttle.
sim_header = (root / "Source/LifeLens/Simulation/LLSimulationSubsystem.h").read_text(encoding="utf-8")
sim_cpp = (root / "Source/LifeLens/Simulation/LLSimulationSubsystem.cpp").read_text(encoding="utf-8")
for token in (
    "virtual void Deinitialize() override",
    "FlushAutosaveForApplicationLifecycle",
    "ApplicationWillEnterBackgroundHandle",
    "ApplicationWillTerminateHandle",
):
    assert token in sim_header, f"missing lifecycle autosave header contract: {token}"

for token in (
    "FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject",
    "FCoreDelegates::ApplicationWillTerminateDelegate.AddUObject",
    "FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove",
    "FCoreDelegates::ApplicationWillTerminateDelegate.Remove",
    "FlushAutosaveForApplicationLifecycle();",
    "if (SaveGame())",
):
    assert token in sim_cpp, f"missing lifecycle autosave implementation: {token}"

print("LifeLens autosave write throttle: PASS")
