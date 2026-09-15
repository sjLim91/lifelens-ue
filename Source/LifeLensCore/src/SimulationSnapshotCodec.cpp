#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/CivilizationSnapshotCodec.h"
#include "lifelens/EnvironmentalResidueSnapshotCodec.h"
#include "lifelens/PrimitiveSanitationSnapshotCodec.h"
#include "lifelens/SimulationRulesetSnapshotCodec.h"
#include "lifelens/SocialKnowledgeSnapshotCodec.h"
#include "lifelens/WorldGenerationSnapshotCodec.h"

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

#define encodeSimulationSnapshot encodeSimulationSnapshotBaseBody
#define decodeSimulationSnapshot decodeSimulationSnapshotBaseBody
#include "SimulationSnapshotCodecLegacy.cpp"
#undef encodeSimulationSnapshot
#undef decodeSimulationSnapshot

namespace lifelens {
namespace {

std::uint32_t readBinaryFormatVersion(const std::vector<std::uint8_t>& bytes)
{
    if(bytes.size()<12) return 0;
    return static_cast<std::uint32_t>(bytes[8])
        | (static_cast<std::uint32_t>(bytes[9])<<8)
        | (static_cast<std::uint32_t>(bytes[10])<<16)
        | (static_cast<std::uint32_t>(bytes[11])<<24);
}

bool validateCivilizationWorldForCodec(const World& world,std::string* error)
{
    std::unordered_set<CharacterId> characterIds;
    for(const Character& character:world.characters){
        if(character.id==0 || !characterIds.insert(character.id).second){
            setError(error,"invalid civilization character id");
            return false;
        }
        if(!validateIndividualCivilizationState(character.civilization,character.id)){
            setError(error,"invalid character civilization state");
            return false;
        }
    }

    std::unordered_set<ResourceNodeId> resourceIds;
    for(const ResourceNode& node:world.resourceNodes){
        if(node.id==0 || !resourceIds.insert(node.id).second
           || !validMaterialKind(node.material) || node.material==MaterialKind::Unknown
           || node.quantity<0 || node.maxQuantity<node.quantity || node.regenerationPerDay<0){
            setError(error,"invalid civilization resource node");
            return false;
        }
    }

    std::unordered_set<StorageId> storageIds;
    for(const StorageSite& storage:world.storageSites){
        if(storage.id==0 || !storageIds.insert(storage.id).second || !validateInventoryState(storage.inventory)){
            setError(error,"invalid civilization storage state");
            return false;
        }
    }
    return true;
}

bool validateEnvironmentalResiduesForCodec(const World& world,std::string* error)
{
    EnvironmentalResidueField rebuilt;
    if(!rebuilt.restoreState(world.environmentalResidues.all())){
        setError(error,"invalid environmental residue state");
        return false;
    }

    std::unordered_set<CharacterId> characterIds;
    for(const Character& character:world.characters) characterIds.insert(character.id);
    for(const auto& residue:world.environmentalResidues.all()){
        if(characterIds.count(residue.sourceCharacter)==0
           || residue.createdMinute>world.minute
           || residue.lastUpdatedMinute>world.minute){
            setError(error,"environmental residue references invalid character or minute");
            return false;
        }
    }
    return true;
}

bool validatePrimitiveSanitationSitesForCodec(const World& world,std::string* error)
{
    std::unordered_set<CharacterId> characterIds;
    for(const Character& character:world.characters) characterIds.insert(character.id);

    std::unordered_set<SanitationSiteId> siteIds;
    for(const PrimitiveSanitationSite& site:world.primitiveSanitationSites){
        if(!validPrimitiveSanitationSite(site)
           || !siteIds.insert(site.id).second
           || characterIds.count(site.establishedBy)==0
           || site.establishedMinute>world.minute){
            setError(error,"invalid primitive sanitation site state");
            return false;
        }
    }
    return true;
}

bool validateSocialKnowledgeForCodec(
    const SocialKnowledgeBook& book,
    const World& world,
    std::string* error)
{
    SocialKnowledgeBook rebuilt;
    if(!rebuilt.restoreState(book.facts(),book.receipts())){
        setError(error,"invalid social knowledge state");
        return false;
    }

    std::unordered_set<CharacterId> characterIds;
    for(const Character& character:world.characters) characterIds.insert(character.id);

    for(const SocialFact& fact:book.facts()){
        if(characterIds.count(fact.subject)==0 || fact.eventMinute<0 || fact.eventMinute>world.minute){
            setError(error,"social knowledge fact references invalid character or minute");
            return false;
        }
    }
    for(const KnowledgeReceipt& receipt:book.receipts()){
        if(characterIds.count(receipt.holder)==0 ||
           characterIds.count(receipt.originWitness)==0 ||
           characterIds.count(receipt.immediateSource)==0 ||
           characterIds.count(receipt.subject)==0 ||
           receipt.learnedMinute<0 || receipt.learnedMinute>world.minute){
            setError(error,"social knowledge receipt references invalid character or minute");
            return false;
        }
        for(CharacterId id:receipt.transmissionPath){
            if(characterIds.count(id)==0){
                setError(error,"social knowledge path references invalid character");
                return false;
            }
        }
    }
    return true;
}

} // namespace

bool encodeSimulationSnapshot(
    const SimulationStateSnapshot& snapshot,
    std::vector<std::uint8_t>& outBytes,
    std::string* error)
{
    if(!validSimulationRuleset(snapshot.ruleset)){
        setError(error,"invalid simulation ruleset");
        return false;
    }
    if(!validateCivilizationWorldForCodec(snapshot.world,error)) return false;
    if(!validateSocialKnowledgeForCodec(snapshot.socialKnowledge,snapshot.world,error)) return false;
    if(!validateEnvironmentalResiduesForCodec(snapshot.world,error)) return false;
    if(!validatePrimitiveSanitationSitesForCodec(snapshot.world,error)) return false;
    if(!validateWorldGenerationSnapshotState(snapshot.world)){
        setError(error,"invalid world generation state");
        return false;
    }

    std::vector<std::uint8_t> body;
    if(!encodeSimulationSnapshotBaseBody(snapshot,body,error)) return false;

    Writer civilizationExtension;
    writeCivilizationSnapshotExtension(civilizationExtension,snapshot.world);
    body.insert(body.end(),civilizationExtension.bytes.begin(),civilizationExtension.bytes.end());

    Writer knowledgeExtension;
    writeSocialKnowledgeSnapshotExtension(knowledgeExtension,snapshot.socialKnowledge);
    body.insert(body.end(),knowledgeExtension.bytes.begin(),knowledgeExtension.bytes.end());

    Writer environmentExtension;
    writeEnvironmentalResidueSnapshotExtension(environmentExtension,snapshot.world.environmentalResidues);
    body.insert(body.end(),environmentExtension.bytes.begin(),environmentExtension.bytes.end());

    Writer sanitationExtension;
    writePrimitiveSanitationSnapshotExtension(
        sanitationExtension,snapshot.world.primitiveSanitationSites);
    body.insert(body.end(),sanitationExtension.bytes.begin(),sanitationExtension.bytes.end());

    Writer worldGenerationExtension;
    writeWorldGenerationSnapshotExtension(worldGenerationExtension,snapshot.world);
    body.insert(body.end(),worldGenerationExtension.bytes.begin(),worldGenerationExtension.bytes.end());

    Writer rulesetExtension;
    writeSimulationRulesetSnapshotExtension(rulesetExtension,snapshot.ruleset);
    body.insert(body.end(),rulesetExtension.bytes.begin(),rulesetExtension.bytes.end());

    outBytes=std::move(body);
    if(error) error->clear();
    return true;
}

bool decodeSimulationSnapshot(
    const std::vector<std::uint8_t>& bytes,
    SimulationStateSnapshot& outSnapshot,
    std::string* error)
{
    if(readBinaryFormatVersion(bytes)!=SimulationSnapshotBinaryFormatVersion){
        setError(error,"unsupported snapshot binary format");
        return false;
    }

    const auto civilizationMarker=std::find_end(
        bytes.begin()+12,bytes.end(),
        CivilizationSnapshotExtensionMagic,
        CivilizationSnapshotExtensionMagic+sizeof(CivilizationSnapshotExtensionMagic));
    if(civilizationMarker==bytes.end()){
        setError(error,"missing civilization snapshot extension");
        return false;
    }

    const auto socialMarker=std::find_end(
        civilizationMarker,bytes.end(),
        SocialKnowledgeSnapshotExtensionMagic,
        SocialKnowledgeSnapshotExtensionMagic+sizeof(SocialKnowledgeSnapshotExtensionMagic));
    if(socialMarker==bytes.end() || socialMarker<=civilizationMarker){
        setError(error,"missing social knowledge snapshot extension");
        return false;
    }

    const auto environmentMarker=std::find_end(
        socialMarker,bytes.end(),
        EnvironmentalResidueSnapshotExtensionMagic,
        EnvironmentalResidueSnapshotExtensionMagic+sizeof(EnvironmentalResidueSnapshotExtensionMagic));
    if(environmentMarker==bytes.end() || environmentMarker<=socialMarker){
        setError(error,"missing environmental residue snapshot extension");
        return false;
    }

    const auto sanitationMarker=std::find_end(
        environmentMarker,bytes.end(),
        PrimitiveSanitationSnapshotExtensionMagic,
        PrimitiveSanitationSnapshotExtensionMagic+sizeof(PrimitiveSanitationSnapshotExtensionMagic));
    if(sanitationMarker==bytes.end() || sanitationMarker<=environmentMarker){
        setError(error,"missing primitive sanitation snapshot extension");
        return false;
    }

    const auto worldGenerationMarker=std::find_end(
        sanitationMarker,bytes.end(),
        WorldGenerationSnapshotExtensionMagic,
        WorldGenerationSnapshotExtensionMagic+sizeof(WorldGenerationSnapshotExtensionMagic));
    if(worldGenerationMarker==bytes.end() || worldGenerationMarker<=sanitationMarker){
        setError(error,"missing world generation snapshot extension");
        return false;
    }

    const auto rulesetMarker=std::find_end(
        worldGenerationMarker,bytes.end(),
        SimulationRulesetSnapshotExtensionMagic,
        SimulationRulesetSnapshotExtensionMagic+sizeof(SimulationRulesetSnapshotExtensionMagic));
    if(rulesetMarker==bytes.end() || rulesetMarker<=worldGenerationMarker){
        setError(error,"missing simulation ruleset snapshot extension");
        return false;
    }

    std::vector<std::uint8_t> baseBody(bytes.begin(),civilizationMarker);
    std::vector<std::uint8_t> civilizationBytes(civilizationMarker,socialMarker);
    std::vector<std::uint8_t> knowledgeBytes(socialMarker,environmentMarker);
    std::vector<std::uint8_t> environmentBytes(environmentMarker,sanitationMarker);
    std::vector<std::uint8_t> sanitationBytes(sanitationMarker,worldGenerationMarker);
    std::vector<std::uint8_t> worldGenerationBytes(worldGenerationMarker,rulesetMarker);
    std::vector<std::uint8_t> rulesetBytes(rulesetMarker,bytes.end());

    SimulationStateSnapshot decoded;
    if(!decodeSimulationSnapshotBaseBody(baseBody,decoded,error)) return false;

    Reader civilizationReader(civilizationBytes);
    if(!readCivilizationSnapshotExtension(civilizationReader,decoded.world) || !civilizationReader.done()){
        setError(error,"invalid civilization snapshot extension");
        return false;
    }
    if(!validateCivilizationWorldForCodec(decoded.world,error)) return false;

    Reader knowledgeReader(knowledgeBytes);
    if(!readSocialKnowledgeSnapshotExtension(knowledgeReader,decoded.socialKnowledge) || !knowledgeReader.done()){
        setError(error,"invalid social knowledge snapshot extension");
        return false;
    }
    if(!validateSocialKnowledgeForCodec(decoded.socialKnowledge,decoded.world,error)) return false;

    Reader environmentReader(environmentBytes);
    if(!readEnvironmentalResidueSnapshotExtension(environmentReader,decoded.world.environmentalResidues)
       || !environmentReader.done()){
        setError(error,"invalid environmental residue snapshot extension");
        return false;
    }
    if(!validateEnvironmentalResiduesForCodec(decoded.world,error)) return false;

    Reader sanitationReader(sanitationBytes);
    if(!readPrimitiveSanitationSnapshotExtension(
        sanitationReader,decoded.world.primitiveSanitationSites)
       || !sanitationReader.done()){
        setError(error,"invalid primitive sanitation snapshot extension");
        return false;
    }
    if(!validatePrimitiveSanitationSitesForCodec(decoded.world,error)) return false;

    Reader worldGenerationReader(worldGenerationBytes);
    if(!readWorldGenerationSnapshotExtension(worldGenerationReader,decoded.world)
       || !worldGenerationReader.done()){
        setError(error,"invalid world generation snapshot extension");
        return false;
    }

    Reader rulesetReader(rulesetBytes);
    if(!readSimulationRulesetSnapshotExtension(rulesetReader,decoded.ruleset)
       || !rulesetReader.done()){
        setError(error,"invalid simulation ruleset snapshot extension");
        return false;
    }

    outSnapshot=std::move(decoded);
    if(error) error->clear();
    return true;
}

} // namespace lifelens
