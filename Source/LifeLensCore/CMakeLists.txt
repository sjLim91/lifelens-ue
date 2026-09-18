cmake_minimum_required(VERSION 3.16)
project(LifeLensCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(lifelens_core STATIC
    src/Simulation.cpp
    src/ContextActionRuntime.cpp
    src/CivilizationKnowledgeTransmission.cpp
    src/SimulationSnapshot.cpp
    src/SimulationSnapshotCodec.cpp
)

target_include_directories(lifelens_core PUBLIC include)

add_executable(ll_harness harness/main.cpp)
target_link_libraries(ll_harness PRIVATE lifelens_core)

# Keep assertions active for test executables in every configuration. Release
# normally defines NDEBUG and would remove both checks and setup side effects.
function(lifelens_add_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name} PRIVATE lifelens_core)
    if(MSVC)
        target_compile_options(${name} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${name} PRIVATE -UNDEBUG)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endfunction()

lifelens_add_test(test_assertions_enabled)

lifelens_add_test(test_needs)

lifelens_add_test(test_utility)

lifelens_add_test(test_determinism)

lifelens_add_test(test_simulation_calendar)

lifelens_add_test(test_simulation_climate)

lifelens_add_test(test_environmental_consequences)

lifelens_add_test(test_relationship)

lifelens_add_test(test_emotion)

lifelens_add_test(test_emotion_runtime)

lifelens_add_test(test_memory)

lifelens_add_test(test_belief)

lifelens_add_test(test_social_cognition)

lifelens_add_test(test_social_utility)

lifelens_add_test(test_decision_execution)

lifelens_add_test(test_simulation_social_loop)

lifelens_add_test(test_social_communication_read_model)

lifelens_add_test(test_observer_read_model)

lifelens_add_test(test_romance)

lifelens_add_test(test_household)

lifelens_add_test(test_marriage)

lifelens_add_test(test_pregnancy)

lifelens_add_test(test_birth_genetics)

lifelens_add_test(test_lifecycle_growth)

lifelens_add_test(test_parenting)

lifelens_add_test(test_genealogy)

lifelens_add_test(test_aging)

lifelens_add_test(test_death)

lifelens_add_test(test_life_history)

lifelens_add_test(test_life_history_wiring)

lifelens_add_test(test_generation_continuity)

lifelens_add_test(test_observer_read_model_v2)

lifelens_add_test(test_simulation_family_state)

lifelens_add_test(test_production_new_game)

lifelens_add_test(test_autonomous_family_progression)

lifelens_add_test(test_lifecycle_runtime_correctness)

lifelens_add_test(test_context_action_completion)

lifelens_add_test(test_context_action_competing_parenting)

lifelens_add_test(test_core_save_load)

lifelens_add_test(test_snapshot_codec)

lifelens_add_test(test_witness_rumor)

lifelens_add_test(test_civilization_foundation)

lifelens_add_test(test_civilization_persistence)

lifelens_add_test(test_civilization_spatial_targets)

lifelens_add_test(test_traits_preferences)

lifelens_add_test(test_autonomous_civilization_loop)

lifelens_add_test(test_civilization_knowledge_transmission)

lifelens_add_test(test_civilization_knowledge_spatial_authority)

lifelens_add_test(test_civilization_observer_read_model)

lifelens_add_test(test_facility_authority)

lifelens_add_test(test_settlement_autonomy)

lifelens_add_test(test_settlement_facility_effects)

lifelens_add_test(test_primitive_storage_progression)

lifelens_add_test(test_tool_effectiveness)

lifelens_add_test(test_primitive_fire_progression)

lifelens_add_test(test_primitive_furnace_smelting)

lifelens_add_test(test_natural_physical_obstacles)

lifelens_add_test(test_environmental_residue)

lifelens_add_test(test_external_physical_execution)

lifelens_add_test(test_environmental_exposure)

lifelens_add_test(test_sanitation_problem_recognition)

lifelens_add_test(test_primitive_sanitation_progression)

lifelens_add_test(test_designated_sanitation_affordance)

lifelens_add_test(test_primitive_latrine_progression)

lifelens_add_test(test_world_genesis_wg1)

lifelens_add_test(test_world_genesis_wg2)

lifelens_add_test(test_world_generation_milestone_a)

lifelens_add_test(test_world_generation_sanitation_reference)

lifelens_add_test(test_early_survival)
