#include "Characters/LLResidentCharacter.h"
#include "AI/LLDecisionComponent.h"

ALLResidentCharacter::ALLResidentCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    DecisionComponent = CreateDefaultSubobject<ULLDecisionComponent>(TEXT("DecisionComponent"));
}

void ALLResidentCharacter::BindResident(const FLLResidentData& ResidentData)
{
    ResidentId = ResidentData.ResidentId;
    ResidentDisplayName = FText::FromString(ResidentData.DisplayName);
}
