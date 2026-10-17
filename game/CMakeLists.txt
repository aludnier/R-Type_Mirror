cmake_minimum_required(VERSION 3.10.0)

set(SRC
        src/game.cpp
)

add_executable(r-type_client ${SRC})

target_link_libraries(r-type_client PRIVATE engine_client)
