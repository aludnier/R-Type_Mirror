# r-type_server: only the entry point, the server runtime is engine_server
# (game_engine/src/server).
add_executable(r-type_server src/main.cpp)

target_link_libraries(r-type_server PRIVATE engine_server)
