#pragma once

namespace lifelens {
enum class MaterialKind {
    Unknown,
    Stone,
    Flint,
    Wood,
    Fiber,
    Clay,
    Water,
    PlantFood,
    Bone,
    Hide,
    CopperOre,
    TinOre,
    IronOre,
    Charcoal,
    CopperMetal,
    // Appended for C1-F; persisted ordinals above remain stable.
    TinMetal,
    Bronze
};

struct MaterialProperties {
    double hardness=0.0;
    double sharpnessPotential=0.0;
    double brittleness=0.0;
    double flexibility=0.0;
    double combustibility=0.0;
    double heatResistance=0.0;
    double nutrition=0.0;
};

inline MaterialProperties materialProperties(MaterialKind kind)
{
    switch(kind){
        case MaterialKind::Stone: return {0.76,0.48,0.52,0.02,0.00,0.88,0.00};
        case MaterialKind::Flint: return {0.82,0.94,0.86,0.01,0.00,0.90,0.00};
        case MaterialKind::Wood: return {0.34,0.18,0.20,0.42,0.88,0.24,0.00};
        case MaterialKind::Fiber: return {0.08,0.02,0.08,0.92,0.72,0.08,0.00};
        case MaterialKind::Clay: return {0.12,0.01,0.18,0.66,0.00,0.34,0.00};
        case MaterialKind::Water: return {0.00,0.00,0.00,1.00,0.00,0.04,0.00};
        case MaterialKind::PlantFood: return {0.04,0.00,0.08,0.36,0.54,0.06,0.72};
        case MaterialKind::Bone: return {0.58,0.42,0.44,0.08,0.08,0.54,0.00};
        case MaterialKind::Hide: return {0.10,0.01,0.05,0.88,0.30,0.12,0.00};
        case MaterialKind::CopperOre: return {0.72,0.12,0.22,0.02,0.00,0.82,0.00};
        case MaterialKind::TinOre: return {0.62,0.10,0.26,0.02,0.00,0.76,0.00};
        case MaterialKind::IronOre: return {0.86,0.08,0.20,0.01,0.00,0.94,0.00};
        case MaterialKind::Charcoal: return {0.18,0.02,0.64,0.02,0.98,0.52,0.00};
        case MaterialKind::CopperMetal: return {0.56,0.18,0.08,0.42,0.00,0.70,0.00};
        case MaterialKind::TinMetal: return {0.42,0.12,0.06,0.55,0.00,0.58,0.00};
        case MaterialKind::Bronze: return {0.78,0.62,0.12,0.28,0.00,0.82,0.00};
        case MaterialKind::Unknown:
        default: return {};
    }
}

enum class ItemKind {
    RawMaterial,
    SharpFlake,
    StoneCuttingTool,
    Cordage,
    SimpleContainer,
    FuelBundle,
    DiggingStick,
    StoneHammer,
    // C1-F metal tools. Appended to preserve persisted item ordinals.
    BronzeAxe,
    BronzePick,
    // C5 durable proto-record medium. Appended to preserve persisted ordinals.
    RecordTablet,
    PrimitiveWrap // A3 persisted item: append-only
};

enum class ToolCapability {
    None,
    Cut,
    Chop,
    Dig,
    Strike,
    Carry,
    Heat
};

inline ToolCapability itemCapability(ItemKind kind)
{
    switch(kind){
        case ItemKind::SharpFlake: return ToolCapability::Cut;
        case ItemKind::StoneCuttingTool: return ToolCapability::Chop;
        case ItemKind::DiggingStick: return ToolCapability::Dig;
        case ItemKind::StoneHammer: return ToolCapability::Strike;
        case ItemKind::BronzeAxe: return ToolCapability::Chop;
        case ItemKind::BronzePick: return ToolCapability::Strike;
        case ItemKind::SimpleContainer: return ToolCapability::Carry;
        case ItemKind::FuelBundle: return ToolCapability::Heat;
        default: return ToolCapability::None;
    }
}

inline const char* materialName(MaterialKind material)
{
    switch(material){
        case MaterialKind::Stone: return "Stone";
        case MaterialKind::Flint: return "Flint";
        case MaterialKind::Wood: return "Wood";
        case MaterialKind::Fiber: return "Fiber";
        case MaterialKind::Clay: return "Clay";
        case MaterialKind::Water: return "Water";
        case MaterialKind::PlantFood: return "PlantFood";
        case MaterialKind::Bone: return "Bone";
        case MaterialKind::Hide: return "Hide";
        case MaterialKind::CopperOre: return "CopperOre";
        case MaterialKind::TinOre: return "TinOre";
        case MaterialKind::IronOre: return "IronOre";
        case MaterialKind::Charcoal: return "Charcoal";
        case MaterialKind::CopperMetal: return "CopperMetal";
        case MaterialKind::TinMetal: return "TinMetal";
        case MaterialKind::Bronze: return "Bronze";
        default: return "Unknown";
    }
}
}
