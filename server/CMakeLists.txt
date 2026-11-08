# r-type_server: the engine server runtime (engine_server, in
# engine/src/server) plus the R-Type game rules (RTypeServer), whose enemies
# are Lua scripts (engine_scripting).
add_executable(r-type_server
        src/main.cpp
        src/Enemy.h
        src/PlayerLives.cpp
        src/PlayerLives.h
        src/RTypeScripting.cpp
        src/RTypeScripting.h
        src/RTypeServer.cpp
        src/RTypeServer.h
)

# GameProtocol.h is shared with the client (game/src/network).
target_include_directories(r-type_server PRIVATE
        ${PROJECT_SOURCE_DIR}/game/src/network
)
target_link_libraries(r-type_server PRIVATE Eng::server Eng::scripting Eng::physics2d_scripting)
target_compile_definitions(r-type_server PRIVATE ENGINE_MODULE_DIR="${ENGINE_MODULE_DIR}")

# Same layout as the sources: config/scripts.json reads ../server/scripts.
set(SERVER_SCRIPTS
        scripts/bydo.lua
        scripts/waves.lua
        scripts/collision.lua
        scripts/cleanup.lua
)
foreach(script IN LISTS SERVER_SCRIPTS)
    configure_file(${script} ${CMAKE_CURRENT_BINARY_DIR}/${script} COPYONLY)
endforeach()

if (BUILD_TESTS)
    find_package(Catch2 REQUIRED)

    add_executable(r-type_server_tests
            tests/scripts_tests.cpp
            tests/player_lives_tests.cpp
            src/PlayerLives.cpp
            src/RTypeScripting.cpp
    )

    target_include_directories(r-type_server_tests PRIVATE
            src
            ${PROJECT_SOURCE_DIR}/game/src/network
    )
    target_link_libraries(r-type_server_tests PRIVATE Eng::scripting Catch2::Catch2WithMain)
    target_compile_definitions(r-type_server_tests PRIVATE
            SCRIPTS_CONFIG_FILE="${PROJECT_SOURCE_DIR}/config/scripts.json")

    add_test(NAME r-type_server_tests COMMAND r-type_server_tests)
endif ()
