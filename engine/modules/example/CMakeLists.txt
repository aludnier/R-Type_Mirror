cmake_minimum_required(VERSION 3.10.0)

add_library(example_module SHARED
        src/ExampleModule.cpp
)

target_link_libraries(example_module PRIVATE engine_core)

target_compile_features(example_module PRIVATE cxx_std_23)

set_target_properties(example_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Example_Module"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
)