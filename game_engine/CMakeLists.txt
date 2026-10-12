cmake_minimum_required(VERSION 3.10.0)

set(SRC
        src/ecs/Registry.cpp
        src/network/ByteReader.cpp
        src/network/ByteWriter.cpp
        src/network/ClientPackets.cpp
        src/network/Packet.cpp
        src/network/PacketHeader.cpp
        src/network/ServerPackets.cpp
)

add_library(game_engine STATIC ${SRC})

target_include_directories(game_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (BUILD_TESTS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRITERION REQUIRED criterion)

    add_executable(game_engine_tests
            tests/network_protocol_tests.cpp
    )

    target_link_libraries(game_engine_tests PRIVATE game_engine ${CRITERION_LINK_LIBRARIES})
    target_include_directories(game_engine_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CRITERION_INCLUDE_DIRS})
    target_compile_options(game_engine_tests PRIVATE ${CRITERION_CFLAGS_OTHER})

    add_test(NAME game_engine_tests COMMAND game_engine_tests)
endif ()
