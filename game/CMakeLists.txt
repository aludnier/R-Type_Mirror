cmake_minimum_required(VERSION 3.10.0)

find_package(SFML COMPONENTS Graphics Window Audio System CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)

set(SRC
        src/main.cpp
        src/GameApp.cpp
        src/GameApp.h
        src/components/Player.cpp
        src/components/Player.h
        src/components/Enemy.cpp
        src/components/Enemy.h
        src/components/Projectile.cpp
        src/components/Projectile.h
        src/components/ForcePod.cpp
        src/components/ForcePod.h
        src/components/NetworkEntity.cpp
        src/components/NetworkEntity.h
        src/network/NetworkClient.cpp
        src/network/NetworkClient.h
        src/systems/NetworkSyncSystem.cpp
        src/systems/NetworkSyncSystem.h
        src/systems/ShipTiltSystem.cpp
        src/systems/ShipTiltSystem.h
)

add_executable(r-type_client ${SRC})
target_include_directories(r-type_client PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_dependencies(r-type_client physics2d_module environment2d_module
        sfml_assets_module animation2d_module)
target_compile_definitions(r-type_client PRIVATE
        ENGINE_MODULE_DIR="${ENGINE_MODULE_DIR}"
        GAME_ASSET_MODULE_PATH="$<TARGET_FILE:sfml_assets_module>")
target_link_libraries(r-type_client PRIVATE Eng::client Eng::physics2d Eng::physics2d_scripting Eng::environment2d Eng::scripting SFML::Graphics SFML::Window SFML::Audio SFML::System ImGui-SFML::ImGui-SFML)
target_link_libraries(r-type_client PRIVATE Eng::sfml_assets r-type_game_scripting animation2d_module)

# Lua bindings of the game components (Game::bindGameComponents), for the
# hosts that script the game. No graphics dependency.
add_library(r-type_game_scripting STATIC
        src/scripting/GameBindings.cpp
        src/scripting/GameBindings.h
        src/components/Enemy.cpp
        src/components/ForcePod.cpp
        src/components/Player.cpp
        src/components/Projectile.cpp
)
target_include_directories(r-type_game_scripting PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(r-type_game_scripting PUBLIC Eng::scripting)

if (BUILD_TESTS)
    find_package(Catch2 REQUIRED)

    add_executable(r-type_game_tests
            tests/game_bindings_tests.cpp
    )

    target_link_libraries(r-type_game_tests PRIVATE r-type_game_scripting Catch2::Catch2WithMain)

    add_test(NAME r-type_game_tests COMMAND r-type_game_tests)


    add_executable(game_network_tests
            tests/network_sync_audio_tests.cpp
            tests/network_sync_players_tests.cpp
            src/components/Enemy.cpp
            src/components/Player.cpp
            src/components/Projectile.cpp
            src/network/NetworkClient.cpp
            src/systems/NetworkSyncSystem.cpp
            src/systems/ShipTiltSystem.cpp
            tests/ship_tilt_tests.cpp
    )
    target_include_directories(game_network_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(game_network_tests PRIVATE
            Eng::core Eng::physics2d animation2d_module Catch2::Catch2WithMain)
    add_test(NAME game_network_tests COMMAND game_network_tests)
endif ()

if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(VCPKG_RUNTIME_BIN_DIR
                "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin")
    else ()
        set(VCPKG_RUNTIME_BIN_DIR
                "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    endif ()
    file(GLOB VCPKG_RUNTIME_DLLS
            "${VCPKG_RUNTIME_BIN_DIR}/*.dll")

    add_custom_command(TARGET r-type_client POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:r-type_client>"
            "$<TARGET_FILE_DIR:r-type_client>"
            COMMAND_EXPAND_LISTS
    )
    foreach (runtime_dll IN LISTS VCPKG_RUNTIME_DLLS)
        add_custom_command(TARGET r-type_client POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dll}"
                "$<TARGET_FILE_DIR:r-type_client>"
        )
    endforeach ()
endif ()
