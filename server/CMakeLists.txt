# ---------------------------------------------------------------------------
# engine_server: server runtime (game loop, client sessions). Server only,
# never links engine_client.
# ---------------------------------------------------------------------------
add_library(engine_server STATIC
        src/ClientSession.cpp
        src/ClientSession.h
        src/GameServer.cpp
        src/GameServer.h
)

target_include_directories(engine_server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(engine_server PUBLIC engine_core)

# ---------------------------------------------------------------------------
# r-type_server: only the entry point, everything else is in the libraries.
# ---------------------------------------------------------------------------
add_executable(r-type_server src/main.cpp)

target_link_libraries(r-type_server PRIVATE engine_server)
