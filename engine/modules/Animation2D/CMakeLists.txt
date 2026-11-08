cmake_minimum_required(VERSION 3.10.0)

add_library(animation2d_module SHARED
        src/Animation2DModule.cpp
        src/components/AnimationClip.h
        src/components/AnimationSettings.h
        src/components/Animator.h
        src/systems/AnimationSystem.h
)

target_include_directories(animation2d_module PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(animation2d_module PRIVATE engine_core)

target_compile_features(animation2d_module PRIVATE cxx_std_23)

set_target_properties(animation2d_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Animation_Module"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
)

# Headers only: lets a game use the module's components and AnimationSystem
# without linking the shared library, which is loaded at run time.
add_library(animation2d_api INTERFACE)
target_include_directories(animation2d_api INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(Eng::animation2d ALIAS animation2d_api)
