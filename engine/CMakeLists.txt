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
        src/ecs/Entity.cpp
        src/ecs/Entity.h
        src/ecs/SparseArray.h
        src/ecs/Zipper.h
        src/ecs/IndexedZipper.h
        src/math/Vector2.h
        src/math/Vector3.h
        src/components/Action.h
        src/components/Transform2D.h
        src/components/Transform3D.h
        src/components/Controllable.h
        src/components/Health.h
        src/components/Hitbox.h
        src/systems/ControllableSystem.cpp
        src/systems/ControllableSystem.h
        src/library/RessourceManager.cpp
        src/library/RessourceManager.h
        src/library/PluginManager.cpp
        src/library/PluginManager.h
        src/library/NlohmanReader.cpp
        src/library/NlohmanReader.h
        src/network/ByteReader.cpp
        src/network/ByteWriter.cpp
        src/network/ClientPackets.cpp
        src/network/NetworkThread.cpp
        src/network/Packet.cpp
        src/network/PacketHeader.cpp
        src/network/ServerPackets.cpp
        src/network/UdpSocket.cpp
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
if (WIN32)
    target_compile_definitions(engine_core PRIVATE _WIN32_WINNT=0x0A00)
endif ()

# ---------------------------------------------------------------------------
# engine_client: client only (input, engine loop, rendering and audio).
# The graphics library is linked here, never in engine_core.
# ---------------------------------------------------------------------------
find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

set(CLIENT_SRC
        src/library/engine.h
        src/library/InputManager.cpp
        src/library/InputManager.h
        src/components/Sprite.h
        src/systems/RenderSystem.cpp
        src/systems/RenderSystem.h
)

add_library(engine_client STATIC ${CLIENT_SRC})

target_link_libraries(engine_client PUBLIC engine_core SFML::Graphics SFML::Window SFML::System)

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
# Namespaced names: games link Eng:: targets only.
# ---------------------------------------------------------------------------
add_library(Eng::core ALIAS engine_core)
add_library(Eng::client ALIAS engine_client)
add_library(Eng::server ALIAS engine_server)

if (BUILD_TESTS)
    find_package(Catch2 REQUIRED)

    add_executable(game_engine_tests
            tests/TestActions.h
            tests/TempConfigFile.hpp
            tests/ecs_unit_tests.cpp
            tests/network_protocol_tests.cpp
            tests/network_transport_tests.cpp
            tests/library_manager_tests.cpp
            tests/plugin_manager_tests.cpp
            tests/physics2d_plugin_tests.cpp
            tests/nlohman_reader_test.cpp
    )

    target_link_libraries(game_engine_tests PRIVATE engine_client Catch2::Catch2WithMain)

    set(PLUGIN_TEST_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_plugins")
    add_library(test_plugin MODULE tests/plugins/TestPlugin.cpp)
    add_library(test_dependency_plugin MODULE tests/plugins/TestDependencyPlugin.cpp)
    add_library(test_failing_plugin MODULE tests/plugins/TestFailingPlugin.cpp)
    foreach(plugin_target IN ITEMS
            test_plugin test_dependency_plugin test_failing_plugin)
        set_target_properties(${plugin_target} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "$<1:${PLUGIN_TEST_DIRECTORY}>"
                RUNTIME_OUTPUT_DIRECTORY "$<1:${PLUGIN_TEST_DIRECTORY}>")
        target_include_directories(${plugin_target} PRIVATE
                src tests/plugins)
        target_link_libraries(${plugin_target} PRIVATE engine_core)
    endforeach()
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
endif ()

add_subdirectory(modules/example)
add_subdirectory(modules/physics2D)
add_subdirectory(modules/environment2D)
