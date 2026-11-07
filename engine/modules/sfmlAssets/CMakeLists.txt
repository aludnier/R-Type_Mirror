cmake_minimum_required(VERSION 3.10.0)

find_package(SFML COMPONENTS Graphics Audio System CONFIG REQUIRED)

add_library(sfml_assets_module SHARED
        src/GameAssetTypes.h
        src/SfmlAssetsModule.cpp
)

target_include_directories(sfml_assets_module PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(sfml_assets_module PRIVATE
        engine_core SFML::Graphics SFML::Audio SFML::System
)
target_compile_features(sfml_assets_module PRIVATE cxx_std_23)

set_target_properties(sfml_assets_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "SFML_Assets_Module"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${ENGINE_MODULE_DIR}>"
        VCPKG_APPLOCAL_DEPS OFF
)

add_library(sfml_assets_api INTERFACE)
target_include_directories(sfml_assets_api INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_library(Eng::sfml_assets ALIAS sfml_assets_api)
