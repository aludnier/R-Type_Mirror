cmake_minimum_required(VERSION 3.10.0)

add_library(physics2d_module SHARED
        src/Physics2DModule.cpp
        src/components/PhysicsSettings2D.cpp
        src/systems/OverlapTests.cpp
        src/systems/MovementSystem.cpp
        src/systems/CollisionDetectionSystem.cpp
        src/systems/CollisionEventEmitter.cpp
        src/systems/CollisionResponseSystem.cpp
)

target_include_directories(physics2d_module PUBLIC
        ${PROJECT_SOURCE_DIR}/game_engine/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(physics2d_module PRIVATE engine_core)

target_compile_features(physics2d_module PRIVATE cxx_std_23)

set_target_properties(physics2d_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Physics2D_Module"
)

if (BUILD_TESTS)
    target_include_directories(game_engine_tests PRIVATE
            ${PROJECT_SOURCE_DIR}/game_engine/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(game_engine_tests PRIVATE
            PHYSICS2D_MODULE_PATH="$<TARGET_FILE:physics2d_module>")
    add_dependencies(game_engine_tests physics2d_module)
endif ()