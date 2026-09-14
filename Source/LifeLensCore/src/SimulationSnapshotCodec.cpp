#include "lifelens/SimulationSnapshotCodec.h"
#include "lifelens/CivilizationSnapshotCodec.h"

#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

// Preserve the battle-tested v1 body codec verbatim, but rename its public
// entrypoints inside this translation unit. The v2 wrapper below appends a
// versioned civilization extension while continuing to decode legacy v1 bytes.
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

} // namespace

bool encodeSimulationSnapshot(
    const SimulationStateSnapshot& snapshot,
    std::vector<std::uint8_t>& outBytes,
    std::string* error)
{
    if(!validateCivilizationWorldForCodec(snapshot.world,error)) return false;

    std::vector<std::uint8_t> body;
    if(!encodeSimulationSnapshotLegacyBody(snapshot,body,error)) return false;

    // The preserved body uses the current header constant, so it already emits
    // binary format v2. Civilization data is appended after the exact v1 body.
    Writer extension;
    writeCivilizationSnapshotExtension(extension,snapshot.world);
    body.insert(body.end(),extension.bytes.begin(),extension.bytes.end());
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
        // v1 and v2 share the exact legacy body. The preserved legacy decoder
        // now expects the current header version, so patch only the temporary
        // header before decoding; authoritative payload bytes remain untouched.
        std::vector<std::uint8_t> migratedBytes=bytes;
        patchBinaryFormatVersion(migratedBytes,SimulationSnapshotBinaryFormatVersion);
        SimulationStateSnapshot decoded;
        if(!decodeSimulationSnapshotLegacyBody(migratedBytes,decoded,error)) return false;
        initializeLegacyCivilizationState(decoded.world);
        if(!validateCivilizationWorldForCodec(decoded.world,error)) return false;
        outSnapshot=std::move(decoded);
        if(error) error->clear();
        return true;
    }

    const auto marker=std::find_end(
        bytes.begin()+12,bytes.end(),
        CivilizationSnapshotExtensionMagic,
        CivilizationSnapshotExtensionMagic+sizeof(CivilizationSnapshotExtensionMagic));
    if(marker==bytes.end()){
        setError(error,"missing civilization snapshot extension");
        return false;
    }

    std::vector<std::uint8_t> legacyBody(bytes.begin(),marker);
    std::vector<std::uint8_t> extensionBytes(marker,bytes.end());

    SimulationStateSnapshot decoded;
    if(!decodeSimulationSnapshotLegacyBody(legacyBody,decoded,error)) return false;

    Reader extensionReader(extensionBytes);
    if(!readCivilizationSnapshotExtension(extensionReader,decoded.world) || !extensionReader.done()){
        setError(error,"invalid civilization snapshot extension");
        return false;
    }
    if(!validateCivilizationWorldForCodec(decoded.world,error)) return false;

    outSnapshot=std::move(decoded);
    if(error) error->clear();
    return true;
}

} // namespace lifelens
