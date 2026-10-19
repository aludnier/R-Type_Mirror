# r-type_server: the engine server runtime (engine_server, in
# game_engine/src/server) plus the R-Type game rules (RTypeServer).
add_executable(r-type_server
        src/main.cpp
        src/RTypeServer.cpp
        src/RTypeServer.h
)

# GameProtocol.h is shared with the client (game/src/network).
target_include_directories(r-type_server PRIVATE
        ${PROJECT_SOURCE_DIR}/game/src/network
)
target_link_libraries(r-type_server PRIVATE engine_server)
