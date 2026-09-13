#include "Simulation/LLCoreBridgeSubsystem.h"
#include "lifelens/Simulation.h"

void ULLCoreBridgeSubsystem::Deinitialize()
{
    delete CoreSimulation;
    CoreSimulation = nullptr;
    RecentEvents.Reset();
    Super::Deinitialize();
}

void ULLCoreBridgeSubsystem::StartFastTest(int32 Seed)
{
    delete CoreSimulation;
    CoreSimulation = nullptr;
    RecentEvents.Reset();

    const uint64 CoreSeed = Seed == 0 ? 42u : static_cast<uint64>(Seed);
    CoreSimulation = new lifelens::Simulation(CoreSeed);
    CoreSimulation->onEvent([this](const std::string& Line)
    {
        PushCoreEvent(UTF8_TO_TCHAR(Line.c_str()));
    });
    CoreSimulation->setupDemo();
}

void ULLCoreBridgeSubsystem::AdvanceCoreMinutes(int32 Minutes)
{
    if (!CoreSimulation || Minutes <= 0)
    {
        return;
    }

    CoreSimulation->runMinutes(Minutes);
}

void ULLCoreBridgeSubsystem::PushCoreEvent(const FString& Line)
{
    RecentEvents.Add(Line);
    while (RecentEvents.Num() > MaxRecentEvents)
    {
        RecentEvents.RemoveAt(0);
    }
}
