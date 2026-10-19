#pragma once

#include "Character.h"
#include "Civilization.h"
#include "Parenting.h" // requiresDirectCare: Baby/Toddler need direct household care

namespace lifelens {

// Social Comfort is still selected autonomously by the ordinary utility
// competition. Its REAL close-range Core completion may include a material
// gift, but does not magically heal, feed, hydrate or create a resource.
enum class InjuryProvisionGift { None, PlantFood, PortableWater };

inline const char* injuryProvisionGiftName(InjuryProvisionGift gift)
{
    switch(gift){
        case InjuryProvisionGift::PlantFood: return "PlantFood";
        case InjuryProvisionGift::PortableWater: return "PortableWater";
        default: return "None";
    }
}

inline InjuryProvisionGift provideInjuryRecoveryProvisions(
    Character& helper,Character& injured,int minute)
{
    if(minute<0 ||helper.id==0 ||injured.id==0 ||helper.id==injured.id
       ||!helper.alive ||!injured.alive
       ||requiresDirectCare(helper.lifeStage)
       ||requiresDirectCare(injured.lifeStage)
       ||injured.health.injurySeverity<0.18)
        return InjuryProvisionGift::None;

    Inventory& source=helper.civilization.inventory;
    Inventory& target=injured.civilization.inventory;

    // The helper retains at least one usable ration or filled vessel; its
    // own critical Needs always beat optional social generosity.
    const bool food=helper.needs.hunger<0.65
        &&injured.needs.hunger>=0.55
        &&source.count(ItemKind::RawMaterial,MaterialKind::PlantFood)>=2
        &&target.count(ItemKind::RawMaterial,MaterialKind::PlantFood)==0;
    const bool water=helper.needs.thirst<0.65
        &&injured.needs.thirst>=0.55
        &&portableWaterCount(source)>=2
        &&portableWaterCount(target)==0;
    if(!food && !water) return InjuryProvisionGift::None;

    InjuryProvisionGift gift=InjuryProvisionGift::None;
    if(water &&(!food ||injured.needs.thirst>=injured.needs.hunger)){
        if(transferPortableWater(source,target,1))
            gift=InjuryProvisionGift::PortableWater;
    }else if(food){
        if(source.transferTo(target,ItemKind::RawMaterial,
                             MaterialKind::PlantFood,1))
            gift=InjuryProvisionGift::PlantFood;
    }
    if(gift==InjuryProvisionGift::None) return gift;

    // Both participants directly observed this very real inventory change.
    // This is evidence of PROVISION, not evidence of consuming it or healing.
    for(Character* witness:{&helper,&injured}){
        MemoryRecord memory;
        memory.who=witness->id;
        memory.sourceCharacter=witness->id;
        memory.minute=minute;
        memory.source=MemorySource::DirectWitness;
        memory.witnessed=true;
        memory.what=witness==&helper
            ? "다친 주민에게 실제 보유한 생존 물품을 직접 전달했다"
            : "다른 주민이 실제 보유한 생존 물품을 나에게 전달했다";
        memory.where="Core 사회적 위로 행동 중 직접 전달";
        memory.importance=0.65;
        memory.confidence=1.0;
        memory.emotionValence=0.45;
        memory.emotionIntensity=0.4;
        memory.tags={"injury_care_provision",
                     injuryProvisionGiftName(gift),
                     witness==&helper?"given":"received"};
        witness->memory.add(memory);
    }
    return gift;
}

} // namespace lifelens
