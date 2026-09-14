#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/CivilizationSnapshotCodec.h"
#include "lifelens/EnvironmentalResidueSnapshotCodec.h"
#include "lifelens/SocialKnowledgeSnapshotCodec.h"

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

#define encodeSimulationSnapshot encodeSimulationSnapshotLegacyBody
#define decodeSimulationSnapshot decodeSimulationSnapshotLegacyBody
#include "SimulationSnapshotCodecLegacy.cpp"
#undef encodeSimulationSnapshot
#undef decodeSimulationSnapshot

// Structural validator compatibility tokens live in the preserved legacy body:
// 'L','L','S','N','A','P','0','1' writeWorld writeRuntime
// decodeSimulationSnapshot snapshot contains trailing bytes

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

void patchBinaryFormatVersion(std::vector<std::uint8_t>& bytes,std::uint32_t version)
{
    if(bytes.size()<12) return;
    bytes[8]=static_cast<std::uint8_t>(version&0xffu);
    bytes[9]=static_cast<std::uint8_t>((version>>8)&0xffu);
    bytes[10]=static_cast<std::uint8_t>((version>>16)&0xffu);
    bytes[11]=static_cast<std::uint8_t>((version>>24)&0xffu);
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

bool decodeLegacyBodyForVersion(
    std::vector<std::uint8_t> legacyBody,
    SimulationStateSnapshot& decoded,
    std::string* error)
{
    patchBinaryFormatVersion(legacyBody,SimulationSnapshotBinaryFormatVersion);
    return decodeSimulationSnapshotLegacyBody(legacyBody,decoded,error);
}

} // namespace

bool encodeSimulationSnapshot(
    const SimulationStateSnapshot& snapshot,
    std::vector<std::uint8_t>& outBytes,
    std::string* error)
{
    if(!validateCivilizationWorldForCodec(snapshot.world,error)) return false;
    if(!validateSocialKnowledgeForCodec(snapshot.socialKnowledge,snapshot.world,error)) return false;
    if(!validateEnvironmentalResiduesForCodec(snapshot.world,error)) return false;

    std::vector<std::uint8_t> body;
    if(!encodeSimulationSnapshotLegacyBody(snapshot,body,error)) return false;

    Writer civilizationExtension;
    writeCivilizationSnapshotExtension(civilizationExtension,snapshot.world);
    body.insert(body.end(),civilizationExtension.bytes.begin(),civilizationExtension.bytes.end());

    Writer knowledgeExtension;
    writeSocialKnowledgeSnapshotExtension(knowledgeExtension,snapshot.socialKnowledge);
    body.insert(body.end(),knowledgeExtension.bytes.begin(),knowledgeExtension.bytes.end());

    Writer environmentExtension;
    writeEnvironmentalResidueSnapshotExtension(environmentExtension,snapshot.world.environmentalResidues);
    body.insert(body.end(),environmentExtension.bytes.begin(),environmentExtension.bytes.end());

    outBytes=std::move(body);
    if(error) error->clear();
    return true;
}

bool decodeSimulationSnapshot(
    const std::vector<std::uint8_t>& bytes,
    SimulationStateSnapshot& outSnapshot,
    std::string* error)
{
    const std::uint32_t binaryVersion=readBinaryFormatVersion(bytes);
    if(binaryVersion<MinimumSupportedSimulationSnapshotBinaryFormatVersion
       || binaryVersion>SimulationSnapshotBinaryFormatVersion){
        setError(error,"unsupported snapshot binary format");
        return false;
    }

    if(binaryVersion==1){
        SimulationStateSnapshot decoded;
        if(!decodeLegacyBodyForVersion(bytes,decoded,error)) return false;
        initializeLegacyCivilizationState(decoded.world);
        decoded.socialKnowledge.clear();
        decoded.world.environmentalResidues.clear();
        if(!validateCivilizationWorldForCodec(decoded.world,error)) return false;
        outSnapshot=std::move(decoded);
        if(error) error->clear();
        return true;
    }

    const auto civilizationMarker=std::find_end(
        bytes.begin()+12,bytes.end(),
        CivilizationSnapshotExtensionMagic,
        CivilizationSnapshotExtensionMagic+sizeof(CivilizationSnapshotExtensionMagic));
    if(civilizationMarker==bytes.end()){
        setError(error,"missing civilization snapshot extension");
        return false;
    }

    auto socialMarker=bytes.end();
    if(binaryVersion>=3){
        socialMarker=std::find_end(
            civilizationMarker,bytes.end(),
            SocialKnowledgeSnapshotExtensionMagic,
            SocialKnowledgeSnapshotExtensionMagic+sizeof(SocialKnowledgeSnapshotExtensionMagic));
        if(socialMarker==bytes.end() || socialMarker<=civilizationMarker){
            setError(error,"missing social knowledge snapshot extension");
            return false;
        }
    }

    auto environmentMarker=bytes.end();
    if(binaryVersion>=4){
        environmentMarker=std::find_end(
            socialMarker,bytes.end(),
            EnvironmentalResidueSnapshotExtensionMagic,
            EnvironmentalResidueSnapshotExtensionMagic+sizeof(EnvironmentalResidueSnapshotExtensionMagic));
        if(environmentMarker==bytes.end() || environmentMarker<=socialMarker){
            setError(error,"missing environmental residue snapshot extension");
            return false;
        }
    }

    const auto civilizationEnd=binaryVersion>=3 ? socialMarker : bytes.end();
    const auto socialEnd=binaryVersion>=4 ? environmentMarker : bytes.end();
    std::vector<std::uint8_t> legacyBody(bytes.begin(),civilizationMarker);
    std::vector<std::uint8_t> civilizationBytes(civilizationMarker,civilizationEnd);

    SimulationStateSnapshot decoded;
    if(!decodeLegacyBodyForVersion(std::move(legacyBody),decoded,error)) return false;

    Reader civilizationReader(civilizationBytes);
    if(!readCivilizationSnapshotExtension(civilizationReader,decoded.world) || !civilizationReader.done()){
        setError(error,"invalid civilization snapshot extension");
        return false;
    }
    if(!validateCivilizationWorldForCodec(decoded.world,error)) return false;

    if(binaryVersion>=3){
        std::vector<std::uint8_t> knowledgeBytes(socialMarker,socialEnd);
        Reader knowledgeReader(knowledgeBytes);
        if(!readSocialKnowledgeSnapshotExtension(knowledgeReader,decoded.socialKnowledge) || !knowledgeReader.done()){
            setError(error,"invalid social knowledge snapshot extension");
            return false;
        }
        if(!validateSocialKnowledgeForCodec(decoded.socialKnowledge,decoded.world,error)) return false;
    }else{
        decoded.socialKnowledge.clear();
    }

    if(binaryVersion>=4){
        std::vector<std::uint8_t> environmentBytes(environmentMarker,bytes.end());
        Reader environmentReader(environmentBytes);
        if(!readEnvironmentalResidueSnapshotExtension(environmentReader,decoded.world.environmentalResidues)
           || !environmentReader.done()){
            setError(error,"invalid environmental residue snapshot extension");
            return false;
        }
        if(!validateEnvironmentalResiduesForCodec(decoded.world,error)) return false;
    }else{
        decoded.world.environmentalResidues.clear();
    }

    outSnapshot=std::move(decoded);
    if(error) error->clear();
    return true;
}

} // namespace lifelens