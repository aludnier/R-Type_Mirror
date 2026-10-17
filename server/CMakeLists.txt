set(SRC
        src/main.cpp
        src/ClientSession.cpp
        src/GameServer.cpp
)

add_executable(r-type_server ${SRC})

target_link_libraries(r-type_server PRIVATE engine_core)
