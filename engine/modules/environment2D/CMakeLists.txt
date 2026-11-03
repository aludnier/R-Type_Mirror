cmake_minimum_required(VERSION 3.10.0)

add_library(environment2d_module SHARED
        src/Environment2DModule.cpp
        src/components/ParallaxBackground.h
        src/systems/ParallaxSystem.h
)

target_include_directories(environment2d_module PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/game_engine/src
)

target_link_libraries(environment2d_module PRIVATE engine_core)

target_compile_features(environment2d_module PRIVATE cxx_std_23)

set_target_properties(environment2d_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Environment2D_Module"
)
