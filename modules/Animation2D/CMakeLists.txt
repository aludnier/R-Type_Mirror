cmake_minimum_required(VERSION 3.10.0)

add_library(animation2d_module SHARED src/Animation2DModule.cpp)

target_include_directories(
        animation2d_module PUBLIC $ {PROJECT_SOURCE_DIR} / game_engine /
        src $ {CMAKE_CURRENT_SOURCE_DIR} / src)

target_link_libraries(animation2d_module PRIVATE engine_core)

target_compile_features(animation2d_module PRIVATE cxx_std_23)

set_target_properties(animation2d_module PROPERTIES PREFIX
        "" OUTPUT_NAME "Animation_Module")
