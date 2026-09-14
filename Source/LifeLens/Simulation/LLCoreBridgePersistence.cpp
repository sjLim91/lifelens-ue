#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Simulation.h"
#include "lifelens/SimulationSnapshotCodec.h"

#include <memory>
#include <string>
#include <vector>

bool ULLCoreBridgeSubsystem::CaptureCoreSnapshotBytes(TArray<uint8>& OutBytes, FString& OutError) const
{
    OutBytes.Reset();
    OutError.Reset();

    if (!CoreSimulation)
    {
        OutError = TEXT("Core simulation is not running.");
        return false;
    }

    const lifelens::SimulationStateSnapshot Snapshot = CoreSimulation->captureSnapshot();
    std::vector<std::uint8_t> Encoded;
    std::string Error;
    if (!lifelens::encodeSimulationSnapshot(Snapshot, Encoded, &Error))
    {
        OutError = UTF8_TO_TCHAR(Error.c_str());
        return false;
    }

    if (Encoded.size() > static_cast<std::size_t>(MAX_int32))
    {
        OutError = TEXT("Core snapshot is too large for Unreal SaveGame storage.");
        return false;
    }

    OutBytes.SetNumUninitialized(static_cast<int32>(Encoded.size()));
    if (!Encoded.empty())
    {
        FMemory::Memcpy(OutBytes.GetData(), Encoded.data(), Encoded.size());
    }
    return true;
}

bool ULLCoreBridgeSubsystem::RestoreCoreSnapshotBytes(const TArray<uint8>& Bytes, FString& OutError)
{
    OutError.Reset();
    if (Bytes.IsEmpty())
    {
        OutError = TEXT("Core snapshot payload is empty.");
        return false;
    }

    std::vector<std::uint8_t> Encoded;
    Encoded.assign(Bytes.GetData(), Bytes.GetData() + Bytes.Num());

    lifelens::SimulationStateSnapshot Snapshot;
    std::string Error;
    if (!lifelens::decodeSimulationSnapshot(Encoded, Snapshot, &Error))
    {
        OutError = UTF8_TO_TCHAR(Error.c_str());
        return false;
    }

    // Restore into a temporary Core instance first. A corrupt/incompatible save
    // must never destroy the currently running world.
    std::unique_ptr<lifelens::Simulation> Candidate =
        std::make_unique<lifelens::Simulation>(Snapshot.world.seed);
    if (!Candidate->restoreSnapshot(Snapshot, &Error))
    {
        OutError = UTF8_TO_TCHAR(Error.c_str());
        return false;
    }

    ResetRuntime();
    CoreSimulation = Candidate.release();
    ActiveSeed = static_cast<int32>(static_cast<uint32>(CoreSimulation->world().seed));
    if (ActiveSeed == 0)
    {
        ActiveSeed = 1;
    }

    CoreSimulation->onEvent([this](const std::string& Line)
    {
        PushCoreEvent(UTF8_TO_TCHAR(Line.c_str()));
    });

    const auto& CoreLogs = CoreSimulation->logs();
    const std::size_t StartIndex = CoreLogs.size() > static_cast<std::size_t>(MaxRecentEvents)
        ? CoreLogs.size() - static_cast<std::size_t>(MaxRecentEvents)
        : 0;
    for (std::size_t Index = StartIndex; Index < CoreLogs.size(); ++Index)
    {
        PushCoreEvent(UTF8_TO_TCHAR(CoreLogs[Index].c_str()));
    }

    RebuildGuidIndex();
    OnCoreRuntimeStateChanged.Broadcast();
    return true;
}
