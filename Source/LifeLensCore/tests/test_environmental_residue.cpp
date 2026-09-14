#include <cassert>
#include "lifelens/EnvironmentalResidue.h"

using namespace lifelens;

int main()
{
    EnvironmentalResidueField field;
    const GridPos site{6,-2};

    auto& first=field.deposit(EnvironmentalResidueKind::HumanWaste,site,1,480,1.0,0.40,3);
    assert(first.id!=0);
    assert(field.all().size()==1);
    assert(field.exposureAt(site)>0.0);

    const double initialAmount=first.amount;
    field.deposit(EnvironmentalResidueKind::HumanWaste,site,2,490,1.0,0.40,3);
    assert(field.all().size()==1);
    assert(field.all().front().amount>initialAmount);
    assert(field.all().front().sourceCharacter==2);

    const double accumulated=field.all().front().amount;
    field.advanceToMinute(490+24*60);
    assert(field.all().front().amount<accumulated);
    assert(field.exposureAt({100,100})==0.0);

    const auto observation=buildEnvironmentObservation(490+24*60,field);
    assert(observation.totalResidues==1);
    assert(observation.humanWasteResidues==1);
    assert(observation.aggregateAmount>0.0);
    assert(!observation.residues.empty());

    EnvironmentalResidueField restored;
    assert(restored.restoreState(field.all()));
    assert(restored.all().size()==field.all().size());
    assert(restored.all().front().id==field.all().front().id);

    return 0;
}
