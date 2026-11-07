cmake_minimum_required(VERSION 3.21)

add_library(lifetime_module SHARED
        src/LifetimeModule.cpp
        src/systems/LifetimeSystem.cpp
        src/systems/TimerClock.cpp
        src/systems/TimerSystem.cpp
)

target_include_directories(lifetime_module PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(lifetime_module PRIVATE engine_core)
target_compile_features(lifetime_module PRIVATE cxx_std_23)
set_target_properties(lifetime_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Lifetime_Module"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
)

add_library(lifetime_api INTERFACE)
target_include_directories(lifetime_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(Eng::lifetime ALIAS lifetime_api)

if (BUILD_TESTS)
    target_include_directories(game_engine_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(game_engine_tests PRIVATE
            LIFETIME_MODULE_PATH="$<TARGET_FILE:lifetime_module>")
    add_dependencies(game_engine_tests lifetime_module)
endif ()
