#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Simulation.h"

namespace
{
int32 SafeEnvironmentCount(std::size_t Count)
{
    return Count > static_cast<std::size_t>(MAX_int32)
        ? MAX_int32
        : static_cast<int32>(Count);
}
}

FLLCoreEnvironmentObservation ULLCoreBridgeSubsystem::GetEnvironmentObservation(int32 MaxResidues) const
{
    FLLCoreEnvironmentObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const std::size_t Limit = MaxResidues <= 0
        ? 0u
        : static_cast<std::size_t>(MaxResidues);
    const lifelens::EnvironmentObservation CoreEnvironment =
        CoreSimulation->observeEnvironment(Limit);

    Result.SimulationMinute = static_cast<int64>(CoreEnvironment.minute);
    Result.TotalResidues = SafeEnvironmentCount(CoreEnvironment.totalResidues);
    Result.HumanWasteResidues = SafeEnvironmentCount(CoreEnvironment.humanWasteResidues);
    Result.AggregateAmount = static_cast<float>(CoreEnvironment.aggregateAmount);
    Result.PeakIntensity = static_cast<float>(CoreEnvironment.peakIntensity);
    Result.Residues.Reserve(SafeEnvironmentCount(CoreEnvironment.residues.size()));

    for (const lifelens::EnvironmentalResidueObservation& CoreResidue : CoreEnvironment.residues)
    {
        FLLCoreEnvironmentalResidueObservation Residue;
        Residue.ResidueId = CoreResidue.id > static_cast<uint64>(MAX_int64)
            ? MAX_int64
            : static_cast<int64>(CoreResidue.id);
        Residue.Kind = ELLCoreEnvironmentalResidueKind::HumanWaste;
        Residue.GridX = CoreResidue.pos.x;
        Residue.GridY = CoreResidue.pos.y;
        Residue.SourceResidentId = MakeStableResidentGuid(static_cast<uint64>(CoreResidue.sourceCharacter));
        Residue.AgeMinutes = CoreResidue.ageMinutes;
        Residue.Amount = static_cast<float>(CoreResidue.amount);
        Residue.Intensity = static_cast<float>(CoreResidue.intensity);
        Residue.RadiusTiles = CoreResidue.radiusTiles;
        Result.Residues.Add(MoveTemp(Residue));
    }

    return Result;
}
