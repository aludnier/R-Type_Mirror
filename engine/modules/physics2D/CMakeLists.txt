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
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(physics2d_module PRIVATE engine_core)

target_compile_features(physics2d_module PRIVATE cxx_std_23)

set_target_properties(physics2d_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Physics2D_Module"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
)

# Headers only: lets a game use the module's components without linking
# the shared library, which is loaded at run time.
add_library(physics2d_api INTERFACE)
target_include_directories(physics2d_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(Eng::physics2d ALIAS physics2d_api)

# Lua bindings of the components, linked by the host: Lua is a static
# library, so the module itself never touches the host's Lua state.
add_library(physics2d_scripting STATIC
        scripting/Physics2DBindings.cpp
        scripting/Physics2DBindings.h
)
target_include_directories(physics2d_scripting PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/scripting
)
target_link_libraries(physics2d_scripting PUBLIC engine_scripting physics2d_api)
add_library(Eng::physics2d_scripting ALIAS physics2d_scripting)

if (BUILD_TESTS)
    target_include_directories(game_engine_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(game_engine_tests PRIVATE
            PHYSICS2D_MODULE_PATH="$<TARGET_FILE:physics2d_module>")
    add_dependencies(game_engine_tests physics2d_module)
    target_compile_definitions(engine_scripting_tests PRIVATE
            PHYSICS2D_MODULE_PATH="$<TARGET_FILE:physics2d_module>")
    add_dependencies(engine_scripting_tests physics2d_module)
endif ()