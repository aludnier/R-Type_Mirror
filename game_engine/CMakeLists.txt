cmake_minimum_required(VERSION 3.10.0)

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
        src/network/ByteReader.cpp
        src/network/ByteWriter.cpp
        src/network/ClientPackets.cpp
        src/network/NetworkThread.cpp
        src/network/Packet.cpp
        src/network/PacketHeader.cpp
        src/network/ServerPackets.cpp
        src/network/UdpSocket.cpp
)

add_library(engine_core STATIC ${CORE_SRC})

target_include_directories(engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Linked into the modules, which are shared libraries.
set_target_properties(engine_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Asio is installed by vcpkg (vcpkg.json). PRIVATE: only UdpSocket.cpp
# includes it, the users of the engine never see it.
find_package(asio CONFIG REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(engine_core PRIVATE asio::asio PUBLIC Threads::Threads)
if (WIN32)
    # Windows 10 API, otherwise Asio warns about a missing _WIN32_WINNT.
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
)

add_library(engine_server STATIC ${SERVER_SRC})

target_link_libraries(engine_server PUBLIC engine_core)

# ---------------------------------------------------------------------------
# Temporary: keeps the old target name working for branches opened before
# the split. Remove once every branch links engine_core / engine_client.
# ---------------------------------------------------------------------------
add_library(game_engine INTERFACE)
target_link_libraries(game_engine INTERFACE engine_core engine_client)

if (BUILD_TESTS)
    add_executable(game_engine_tests
            tests/ecs_unit_tests.cpp
            tests/network_protocol_tests.cpp
            tests/network_transport_tests.cpp
            tests/library_manager_tests.cpp
    )

    # engine_client brings engine_core with it.
    target_link_libraries(game_engine_tests PRIVATE engine_client Catch2::Catch2WithMain)

    add_test(NAME game_engine_tests COMMAND game_engine_tests)

    # Separate binary: the server tests must not link engine_client.
    add_executable(engine_server_tests
            tests/server_sending_tests.cpp
    )

    target_link_libraries(engine_server_tests PRIVATE engine_server Catch2::Catch2WithMain)

    add_test(NAME engine_server_tests COMMAND engine_server_tests)
endif ()
