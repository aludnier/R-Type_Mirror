cmake_minimum_required(VERSION 3.10.0)

add_library(environment2d_module SHARED
        src/Environment2DModule.cpp
        src/components/ParallaxBackground.h
        src/systems/ParallaxSystem.h
)

target_include_directories(environment2d_module PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(environment2d_module PRIVATE engine_core)

target_compile_features(environment2d_module PRIVATE cxx_std_23)

set_target_properties(environment2d_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Environment2D_Module"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
)

# Headers only: lets a game use the module's components without linking
# the shared library, which is loaded at run time.
add_library(environment2d_api INTERFACE)
target_include_directories(environment2d_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(Eng::environment2d ALIAS environment2d_api)
