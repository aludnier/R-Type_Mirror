cmake_minimum_required(VERSION 3.10.0)

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
        ${PROJECT_SOURCE_DIR}/modules/environment2D/src
        ${PROJECT_SOURCE_DIR}/modules/physics2D/src
)
find_package(SFML COMPONENTS Graphics Window Audio System CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)

add_dependencies(r-type_client physics2d_module environment2d_module)
target_link_libraries(r-type_client PRIVATE engine_client SFML::Graphics SFML::Window SFML::Audio SFML::System ImGui-SFML::ImGui-SFML)
