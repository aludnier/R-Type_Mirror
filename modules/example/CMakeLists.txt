cmake_minimum_required(VERSION 3.10.0)

add_library(example_module SHARED
        src/ExampleModule.cpp
)

target_include_directories(example_module PUBLIC
        ${PROJECT_SOURCE_DIR}/game_engine/src
)

target_compile_features(example_module PRIVATE cxx_std_23)

set_target_properties(example_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "example_module"
)