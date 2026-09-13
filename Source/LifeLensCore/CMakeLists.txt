cmake_minimum_required(VERSION 3.16)
project(LifeLensCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(lifelens_core STATIC
    src/Simulation.cpp
)

target_include_directories(lifelens_core PUBLIC include)

add_executable(ll_harness harness/main.cpp)
target_link_libraries(ll_harness PRIVATE lifelens_core)

add_executable(test_needs tests/test_needs.cpp)
target_link_libraries(test_needs PRIVATE lifelens_core)
add_test(NAME test_needs COMMAND test_needs)

add_executable(test_utility tests/test_utility.cpp)
target_link_libraries(test_utility PRIVATE lifelens_core)
add_test(NAME test_utility COMMAND test_utility)

add_executable(test_determinism tests/test_determinism.cpp)
target_link_libraries(test_determinism PRIVATE lifelens_core)
add_test(NAME test_determinism COMMAND test_determinism)

add_executable(test_relationship tests/test_relationship.cpp)
target_link_libraries(test_relationship PRIVATE lifelens_core)
add_test(NAME test_relationship COMMAND test_relationship)

add_executable(test_emotion tests/test_emotion.cpp)
target_link_libraries(test_emotion PRIVATE lifelens_core)
add_test(NAME test_emotion COMMAND test_emotion)

add_executable(test_memory tests/test_memory.cpp)
target_link_libraries(test_memory PRIVATE lifelens_core)
add_test(NAME test_memory COMMAND test_memory)

add_executable(test_belief tests/test_belief.cpp)
target_link_libraries(test_belief PRIVATE lifelens_core)
add_test(NAME test_belief COMMAND test_belief)

add_executable(test_social_cognition tests/test_social_cognition.cpp)
target_link_libraries(test_social_cognition PRIVATE lifelens_core)
add_test(NAME test_social_cognition COMMAND test_social_cognition)

add_executable(test_social_utility tests/test_social_utility.cpp)
target_link_libraries(test_social_utility PRIVATE lifelens_core)
add_test(NAME test_social_utility COMMAND test_social_utility)
