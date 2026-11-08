cmake_minimum_required(VERSION 3.21)
project(Eng VERSION 0.1.0 LANGUAGES C CXX)

if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    option(BUILD_TESTS "Build the tests" ON)
    include(CTest)
endif ()

# Every engine module (.so/.dll) is built into this folder.
set(ENGINE_MODULE_DIR "${CMAKE_BINARY_DIR}/modules" CACHE INTERNAL "")

# ---------------------------------------------------------------------------
# engine_core: used by the server AND the client. Must never depend on a
# graphics library (the server links it).
# ---------------------------------------------------------------------------
set(CORE_SRC
        src/Module.h
        src/audio/IAudioPlayer.h
        src/ecs/Entity.cpp
        src/ecs/Entity.h
        src/ecs/SparseArray.h
        src/ecs/Zipper.h
        src/ecs/IndexedZipper.h
        src/math/Vector2.h
        src/math/Vector3.h
        src/components/Action.h
        src/components/AudioSource.h
        src/components/Transform2D.h
        src/components/Transform3D.h
        src/components/Controllable.h
        src/components/Health.h
        src/components/Hitbox.h
        src/components/Name.h
        src/components/MusicPlayer.h
        src/components/Sprite.h
        src/reflection/Reflection.cpp
        src/reflection/Reflection.h
        src/scene/Scene.cpp
        src/scene/Scene.h
        src/systems/ControllableSystem.cpp
        src/systems/ControllableSystem.h
        src/library/AssetManager.cpp
        src/library/AssetManager.h
        src/library/AssetLoaderRegistry.h
        src/configuration/Configuration.cpp
        src/configuration/Configuration.hpp
        src/library/PluginManager.cpp
        src/library/PluginManager.h
        src/library/NlohmanReader.cpp
        src/library/NlohmanReader.h
        src/library/InputManager.cpp
        src/library/InputManager.h
        src/library/Keys.h
        src/ui/UiDocument.cpp
        src/ui/UiDocument.h
        src/ui/UiManager.cpp
        src/ui/UiManager.h
        src/network/ByteReader.cpp
        src/network/ByteWriter.cpp
        src/network/ClientPackets.cpp
        src/network/NetworkThread.cpp
        src/network/Packet.cpp
        src/network/PacketHeader.cpp
        src/network/ServerPackets.cpp
        src/network/UdpSocket.cpp
        src/time/FixedStep.h
)

find_package(nlohmann_json CONFIG REQUIRED)

add_library(engine_core STATIC ${CORE_SRC})

target_include_directories(engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(engine_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(asio CONFIG REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(engine_core
        PRIVATE asio::asio
        PUBLIC Threads::Threads nlohmann_json::nlohmann_json)
target_link_libraries(engine_core PUBLIC ${CMAKE_DL_LIBS})
target_compile_definitions(engine_core PRIVATE ENGINE_MODULE_DIR="${ENGINE_MODULE_DIR}")
if (WIN32)
    target_compile_definitions(engine_core PRIVATE _WIN32_WINNT=0x0A00)
endif ()

# ---------------------------------------------------------------------------
# engine_client: client only (input, engine loop, rendering and audio).
# The graphics library is linked here, never in engine_core.
# ---------------------------------------------------------------------------
find_package(SFML COMPONENTS Graphics Window System Audio CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)

set(CLIENT_SRC
        src/audio/AudioManager.cpp
        src/audio/AudioManager.h
        src/game/GameApp.cpp
        src/game/GameApp.h
        src/library/ClientAssetManager.cpp
        src/library/ClientAssetManager.h
        src/audio/AudioManager.h
        src/audio/AudioManager.cpp
        src/library/engine.h
        src/ui/ImGuiUiRenderer.cpp
        src/ui/ImGuiUiRenderer.h
        src/systems/RenderSystem.cpp
        src/systems/RenderSystem.h
        src/systems/AudioSystem.cpp
        src/systems/AudioSystem.h
)

add_library(engine_client STATIC ${CLIENT_SRC})

target_link_libraries(engine_client PUBLIC engine_core SFML::Graphics SFML::Window SFML::System SFML::Audio ImGui-SFML::ImGui-SFML Eng::sfml_assets)

# ---------------------------------------------------------------------------
# engine_platform: window, OpenGL context and main loop (GLFW + glad). Client
# only, never linked by the server.
# ---------------------------------------------------------------------------
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

set(PLATFORM_SRC
        src/platform/Window.cpp
        src/platform/Window.h
        src/platform/Application.cpp
        src/platform/Application.h
        src/platform/KeyChecks.cpp
        src/platform/RenderTarget.cpp
        src/platform/RenderTarget.h
)

add_library(engine_platform STATIC ${PLATFORM_SRC})

target_link_libraries(engine_platform PUBLIC engine_core glfw glad::glad)
# glad provides the OpenGL headers: GLFW must not include its own.
target_compile_definitions(engine_platform PUBLIC GLFW_INCLUDE_NONE)

# ---------------------------------------------------------------------------
# engine_render2d: OpenGL sprites, camera and framebuffer. Client only.
# ---------------------------------------------------------------------------
find_path(STB_INCLUDE_DIRS "stb_image.h" REQUIRED)

set(RENDER2D_SRC
        src/render/Camera2D.h
        src/render/Framebuffer.cpp
        src/render/Framebuffer.h
        src/render/Renderer2D.cpp
        src/render/Renderer2D.h
        src/render/SceneRenderer.cpp
        src/render/SceneRenderer.h
        src/render/Shader.cpp
        src/render/Shader.h
        src/render/Texture.cpp
        src/render/Texture.h
)

add_library(engine_render2d STATIC ${RENDER2D_SRC})

target_link_libraries(engine_render2d PUBLIC engine_platform)
target_include_directories(engine_render2d PRIVATE ${STB_INCLUDE_DIRS})

# ---------------------------------------------------------------------------
# engine_editor: ImGui panels of the editor. Never linked by a game.
# ---------------------------------------------------------------------------
find_package(imgui CONFIG REQUIRED)

set(EDITOR_SRC
        src/editor/EditorContext.h
        src/editor/EditorShell.cpp
        src/editor/EditorShell.h
        src/editor/FileMenu.cpp
        src/editor/FileMenu.h
        src/editor/ImGuiLayer.cpp
        src/editor/ImGuiLayer.h
        src/editor/PlayMode.cpp
        src/editor/PlayMode.h
        src/editor/SceneFile.cpp
        src/editor/SceneFile.h
        src/editor/panels/HierarchyPanel.cpp
        src/editor/panels/HierarchyPanel.h
        src/editor/panels/InputPanel.cpp
        src/editor/panels/InputPanel.h
        src/editor/panels/InspectorPanel.cpp
        src/editor/panels/InspectorPanel.h
        src/editor/panels/ViewportPanel.cpp
        src/editor/panels/ViewportPanel.h
)

add_library(engine_editor STATIC ${EDITOR_SRC})

target_link_libraries(engine_editor PUBLIC engine_render2d engine_scripting imgui::imgui)

# ---------------------------------------------------------------------------
# engine_server: server runtime (game loop, client sessions). Server only,
# never links engine_client.
# ---------------------------------------------------------------------------
set(SERVER_SRC
        src/server/ClientSession.cpp
        src/server/ClientSession.h
        src/server/GameServer.cpp
        src/server/GameServer.h
        src/DLLoader.hpp
)

add_library(engine_server STATIC ${SERVER_SRC})

target_link_libraries(engine_server PUBLIC engine_core)

# ---------------------------------------------------------------------------
# engine_scripting: Lua scripting on top of the ECS (sol2 + Lua 5.5). No
# graphics dependency, so both the server and the client can link it.
# ---------------------------------------------------------------------------
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)

set(SCRIPTING_SRC
        src/scripting/ComponentBinding.cpp
        src/scripting/ComponentBinding.h
        src/scripting/AudioBindings.cpp
        src/scripting/AudioBindings.h
        src/scripting/CoreBindings.cpp
        src/scripting/CoreBindings.h
        src/scripting/ScriptComponent.h
        src/scripting/ScriptEngine.cpp
        src/scripting/ScriptEngine.h
        src/scripting/ScriptSystem.cpp
        src/scripting/ScriptSystem.h
        src/scripting/UiBindings.cpp
        src/scripting/UiBindings.h
)

add_library(engine_scripting STATIC ${SCRIPTING_SRC})

target_include_directories(engine_scripting PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(engine_scripting PUBLIC engine_core sol2::sol2 ${LUA_LIBRARIES})
target_compile_definitions(engine_scripting PUBLIC SOL_ALL_SAFETIES_ON=1)

# ---------------------------------------------------------------------------
# Namespaced names: games link Eng:: targets only.
# ---------------------------------------------------------------------------
add_library(Eng::core ALIAS engine_core)
add_library(Eng::client ALIAS engine_client)
add_library(Eng::server ALIAS engine_server)
add_library(Eng::scripting ALIAS engine_scripting)
add_library(Eng::platform ALIAS engine_platform)
add_library(Eng::render2d ALIAS engine_render2d)
add_library(Eng::editor ALIAS engine_editor)

if (BUILD_TESTS)
    find_package(Catch2 REQUIRED)

    add_executable(game_engine_tests
            tests/TestActions.h
            tests/TempConfigFile.hpp
            tests/plugins/TestPluginAsset.h
            tests/ecs_unit_tests.cpp
            tests/network_protocol_tests.cpp
            tests/network_transport_tests.cpp
            tests/library_manager_tests.cpp
            tests/audio_tests.cpp
            tests/audio_system_tests.cpp
            tests/plugin_manager_tests.cpp
            tests/physics2d_plugin_tests.cpp
            tests/lifetime_module_tests.cpp
            tests/nlohman_reader_test.cpp
            tests/fixed_step_tests.cpp
            tests/reflection_tests.cpp
            tests/scene_tests.cpp
            tests/ui_tests.cpp
    )

    target_link_libraries(game_engine_tests PRIVATE engine_client Catch2::Catch2WithMain)

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
        foreach (runtime_dll IN LISTS VCPKG_RUNTIME_DLLS)
            add_custom_command(TARGET game_engine_tests POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${runtime_dll}"
                    "$<TARGET_FILE_DIR:game_engine_tests>"
            )
        endforeach ()
    endif ()

    set(PLUGIN_TEST_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_plugins")
    add_library(test_plugin MODULE tests/plugins/TestPlugin.cpp)
    add_library(test_dependency_plugin MODULE tests/plugins/TestDependencyPlugin.cpp)
    add_library(test_failing_plugin MODULE tests/plugins/TestFailingPlugin.cpp)
    foreach (plugin_target IN ITEMS
            test_plugin test_dependency_plugin test_failing_plugin)
        set_target_properties(${plugin_target} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "$<1:${PLUGIN_TEST_DIRECTORY}>"
                RUNTIME_OUTPUT_DIRECTORY "$<1:${PLUGIN_TEST_DIRECTORY}>")
        target_include_directories(${plugin_target} PRIVATE
                src tests/plugins)
        target_link_libraries(${plugin_target} PRIVATE engine_core)
    endforeach ()
    add_dependencies(game_engine_tests
            test_plugin test_dependency_plugin test_failing_plugin)
    target_compile_definitions(game_engine_tests PRIVATE
            TEST_PLUGIN_DIRECTORY="${PLUGIN_TEST_DIRECTORY}")

    add_test(NAME game_engine_tests COMMAND game_engine_tests)

    # Separate binary: the server tests must not link engine_client.
    add_executable(engine_server_tests
            tests/server_sending_tests.cpp
    )

    target_link_libraries(engine_server_tests PRIVATE engine_server Catch2::Catch2WithMain)

    add_test(NAME engine_server_tests COMMAND engine_server_tests)

    add_executable(engine_client_tests
            tests/audio_tests.cpp
    )

    target_link_libraries(engine_client_tests PRIVATE engine_client Catch2::Catch2WithMain Eng::sfml_assets)

    add_test(NAME engine_client_tests COMMAND engine_client_tests)

    add_executable(engine_scripting_tests
            tests/scripting_tests.cpp
            tests/audio_binding_tests.cpp
            tests/physics2d_scripting_tests.cpp
    )

    # physics2d_scripting is defined in modules/physics2D (see below).
    target_link_libraries(engine_scripting_tests PRIVATE engine_scripting physics2d_scripting Catch2::Catch2WithMain)

    target_compile_definitions(engine_scripting_tests PRIVATE
            EXAMPLE_SCRIPT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/scripts/examples")

    add_test(NAME engine_scripting_tests COMMAND engine_scripting_tests)

    add_executable(engine_editor_tests
            tests/editor_shell_tests.cpp
            tests/editor_hierarchy_tests.cpp
            tests/editor_input_tests.cpp
            tests/editor_inspector_tests.cpp
            tests/editor_play_mode_tests.cpp
            tests/editor_scene_file_tests.cpp
    )

    target_link_libraries(engine_editor_tests PRIVATE engine_editor Catch2::Catch2WithMain)

    add_test(NAME engine_editor_tests COMMAND engine_editor_tests)
endif ()

add_subdirectory(apps/sandbox)
add_subdirectory(apps/editor)
add_subdirectory(modules/example)
add_subdirectory(modules/physics2D)
add_subdirectory(modules/environment2D)
add_subdirectory(modules/Animation2D)
add_subdirectory(modules/lifetime)
add_subdirectory(modules/sfmlAssets)

target_link_libraries(engine_client PUBLIC Eng::sfml_assets)
