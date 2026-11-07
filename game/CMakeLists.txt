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
        src/ui/ConnectionDialog.cpp
        src/ui/ConnectionDialog.h
        src/systems/EnemyAISystem.cpp
        src/systems/EnemyAISystem.h
        src/systems/ForcePodSystem.cpp
        src/systems/ForcePodSystem.h
        src/systems/PlayerWeaponSystem.cpp
        src/systems/PlayerWeaponSystem.h
        src/systems/CollisionSystem.cpp
        src/systems/CollisionSystem.h
        src/systems/BoundaryCleanupSystem.cpp
        src/systems/BoundaryCleanupSystem.h
        src/systems/GameRenderSystem.cpp
        src/systems/GameRenderSystem.h
        src/systems/AudioSystem.cpp
        src/systems/AudioSystem.h
        src/systems/NetworkSyncSystem.cpp
        src/systems/NetworkSyncSystem.h
)

add_executable(r-type_client ${SRC})
target_include_directories(r-type_client PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_dependencies(r-type_client physics2d_module environment2d_module
        sfml_assets_module)
target_compile_definitions(r-type_client PRIVATE
        ENGINE_MODULE_DIR="${ENGINE_MODULE_DIR}"
        GAME_ASSET_MODULE_PATH="$<TARGET_FILE:sfml_assets_module>")
target_link_libraries(r-type_client PRIVATE Eng::client Eng::physics2d Eng::environment2d SFML::Graphics SFML::Window SFML::Audio SFML::System ImGui-SFML::ImGui-SFML)
target_link_libraries(r-type_client PRIVATE Eng::sfml_assets)

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
