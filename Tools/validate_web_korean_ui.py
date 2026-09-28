#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCALIZATION = (ROOT / "web/src/localization/korean.ts").read_text(encoding="utf-8")


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//.*", "", text)


def enum_values(path: str, enum_name: str) -> set[str]:
    text = strip_comments(read(path))
    match = re.search(
        rf"enum\s+class\s+{re.escape(enum_name)}(?:\s*:\s*[^{{]+)?\s*{{(.*?)}}\s*;",
        text,
        flags=re.S,
    )
    assert match, f"enum not found: {path}::{enum_name}"
    values: set[str] = set()
    for raw in match.group(1).split(","):
        item = raw.strip()
        if not item:
            continue
        item = item.split("=", 1)[0].strip()
        token = re.match(r"([A-Za-z_]\w*)", item)
        if token:
            values.add(token.group(1))
    return values


def interface_keys(path: str, interface_name: str) -> set[str]:
    text = strip_comments(read(path))
    match = re.search(
        rf"export\s+interface\s+{re.escape(interface_name)}\s*{{(.*?)}}",
        text,
        flags=re.S,
    )
    assert match, f"interface not found: {path}::{interface_name}"
    return set(re.findall(r"^\s*([A-Za-z_]\w*)\??\s*:", match.group(1), flags=re.M))


def map_keys(map_name: str) -> set[str]:
    match = re.search(
        rf"export\s+const\s+{re.escape(map_name)}\s*:\s*Record<string,\s*string>\s*=\s*{{(.*?)}};",
        LOCALIZATION,
        flags=re.S,
    )
    assert match, f"Korean localization map not found: {map_name}"
    return set(re.findall(r"^\s*([A-Za-z_]\w*)\s*:", match.group(1), flags=re.M))


def assert_covered(source_values: set[str], map_name: str, source_name: str) -> None:
    registered = map_keys(map_name)
    missing = sorted(source_values - registered)
    assert not missing, (
        f"한글 번역 등록 누락: {source_name} -> {map_name}: "
        + ", ".join(missing)
    )


ENUM_MAPS = [
    ("Source/LifeLensCore/include/lifelens/Character.h", "Sex", "KOREAN_SEX_LABELS"),
    ("Source/LifeLensCore/include/lifelens/LifeStage.h", "LifeStage", "KOREAN_LIFE_STAGE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/UtilityAI.h", "Goal", "KOREAN_GOAL_LABELS"),
    ("Source/LifeLensCore/include/lifelens/ObserverReadModel.h", "ObservedActivityKind", "KOREAN_ACTIVITY_KIND_LABELS"),
    ("Source/LifeLensCore/include/lifelens/SocialUtility.h", "SocialIntent", "KOREAN_SOCIAL_INTENT_LABELS"),
    ("Source/LifeLensCore/include/lifelens/CivilizationDecision.h", "CivilizationIntent", "KOREAN_CIVILIZATION_INTENT_LABELS"),
    ("Source/LifeLensCore/include/lifelens/CivilizationDecision.h", "FacilityBuildAction", "KOREAN_FACILITY_ACTION_LABELS"),
    ("Source/LifeLensCore/include/lifelens/PresentationDirective.h", "PresentationActionKind", "KOREAN_PRESENTATION_KIND_LABELS"),
    ("Source/LifeLensCore/include/lifelens/PresentationDirective.h", "PresentationActionPhase", "KOREAN_PRESENTATION_PHASE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Parenting.h", "ParentingAction", "KOREAN_PARENTING_ACTION_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Civilization.h", "MaterialKind", "KOREAN_MATERIAL_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Civilization.h", "ItemKind", "KOREAN_ITEM_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Civilization.h", "TechniqueId", "KOREAN_TECHNIQUE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Civilization.h", "KnowledgeLevel", "KOREAN_KNOWLEDGE_LEVEL_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Facility.h", "FacilityKind", "KOREAN_FACILITY_KIND_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Facility.h", "FacilityState", "KOREAN_FACILITY_STATE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/SmartObject.h", "ObjectKind", "KOREAN_OBJECT_KIND_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Romance.h", "RomanceStage", "KOREAN_ROMANCE_STAGE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Genealogy.h", "KinshipType", "KOREAN_KINSHIP_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Pregnancy.h", "PregnancyStage", "KOREAN_PREGNANCY_STAGE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h", "CivilizationKnowledgeSource", "KOREAN_KNOWLEDGE_SOURCE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Memory.h", "MemorySource", "KOREAN_MEMORY_SOURCE_LABELS"),
    ("Source/LifeLensCore/include/lifelens/SocialCognition.h", "SocialEventType", "KOREAN_SOCIAL_EVENT_LABELS"),
    ("Source/LifeLensCore/include/lifelens/SocialCommunicationReadModel.h", "SocialPresentationLevel", "KOREAN_SOCIAL_PRESENTATION_LEVEL_LABELS"),
    ("Source/LifeLensCore/include/lifelens/PrimitiveSanitation.h", "PrimitiveSanitationSiteKind", "KOREAN_SANITATION_SITE_KIND_LABELS"),
    ("Source/LifeLensCore/include/lifelens/SimulationCalendar.h", "SeasonSummary", "KOREAN_SEASON_LABELS"),
    ("Source/LifeLensCore/include/lifelens/SimulationClimate.h", "PrecipitationType", "KOREAN_PRECIPITATION_LABELS"),
    ("Source/LifeLensCore/include/lifelens/SimulationClimate.h", "WeatherSummary", "KOREAN_WEATHER_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Hydrology.h", "SurfaceWaterKind", "KOREAN_SURFACE_WATER_KIND_LABELS"),
    ("Source/LifeLensCore/include/lifelens/Hydrology.h", "WaterSalinity", "KOREAN_WATER_SALINITY_LABELS"),
    ("Source/LifeLensCore/include/lifelens/ContinuousEcology.h", "ContinuousEcologyBiome", "KOREAN_BIOME_LABELS"),
    ("Source/LifeLensCore/include/lifelens/MacroWorldGenesis.h", "MacroBiome", "KOREAN_BIOME_LABELS"),
    ("Source/LifeLensCore/include/lifelens/LifeHistory.h", "LifeEventType", "KOREAN_LIFE_EVENT_LABELS"),
]

for path, enum_name, map_name in ENUM_MAPS:
    assert_covered(enum_values(path, enum_name), map_name, f"{path}::{enum_name}")


INTERFACE_MAPS = [
    ("ResidentTraits", "KOREAN_TRAIT_LABELS"),
    ("ResidentLifeCondition", "KOREAN_LIFE_CONDITION_LABELS"),
    ("ResidentGenetics", "KOREAN_GENETICS_LABELS"),
    ("ResidentDevelopment", "KOREAN_DEVELOPMENT_LABELS"),
]
for interface_name, map_name in INTERFACE_MAPS:
    assert_covered(
        interface_keys("web/src/runtime/core-types.ts", interface_name),
        map_name,
        f"web/src/runtime/core-types.ts::{interface_name}",
    )


social = read("Source/LifeLensCore/include/lifelens/SocialCognition.h")
social_event_tag = re.search(
    r"inline\s+const\s+char\*\s+socialEventTag\s*\([^)]*\)\s*{(.*?)}",
    social,
    flags=re.S,
)
assert social_event_tag, "socialEventTag function missing"
event_tokens = set(re.findall(r'return\s+"([^"]+)"', social_event_tag.group(1)))
assert_covered(event_tokens, "KOREAN_MEMORY_TOKEN_LABELS", "socialEventTag")
assert_covered(
    event_tokens | {"social", "positive", "negative"},
    "KOREAN_MEMORY_TAG_LABELS",
    "memory tags",
)

belief_tokens = set(re.findall(r'effect\.proposition\s*=\s*"([^"]+)"', social))
assert belief_tokens, "belief proposition tokens missing"
assert_covered(belief_tokens, "KOREAN_BELIEF_LABELS", "belief propositions")


VISIBLE_FILES = [
    "web/index.html",
    "web/public/launch.html",
    "web/src/App.tsx",
    "web/src/render/legacy-canvas-world-renderer.ts",
    "web/src/render/resident-action-context.ts",
    "web/src/state/human-traces.ts",
    "web/src/state/observation-feed.ts",
    "web/src/ui/diagnostics-panel.tsx",
    "web/src/ui/fast-forward-control.tsx",
    "web/src/ui/observer-readout.tsx",
    "web/src/ui/render-mode-control.tsx",
    "web/src/ui/world-activity.tsx",
]

visible = "\n".join(read(path) for path in VISIBLE_FILES)
for forbidden in (
    ">LifeLens<",
    "LifeLensCore world truth loading",
    "Core 연결됨",
    "Core 연결 실패",
    "Core 불러오는 중",
    "Seed로 동일한",
    ">WorldSeed<",
    "재현할 WorldSeed",
    "재현할 Seed",
    "이 Seed로 생성",
    "<h2>Diagnostics</h2>",
    "<h2>Development renderer</h2>",
    ">Legacy<",
    ">Three World<",
    "summary: `${facility.kind",
    "detail: facility.state",
    "상태가 ${facility.state}",
    "summary: `${resource.material",
    "summary: `${discovererName}가 ${discovery.technique",
    "memory.what ||",
    "${memory.where}",
    "#${tag}",
    "belief.proposition ||",
    "?? member.kinship",
    "?? pregnancy.stage",
):
    assert forbidden not in visible, f"사용자 화면 영문/원문 직접 노출 금지 위반: {forbidden}"

assert "return labels[key] ?? key" not in LOCALIZATION
assert "번역 등록 누락:" in LOCALIZATION
assert "console.error" in LOCALIZATION

print("LifeLens Web Korean-only UI contract: PASS")
