cmake_minimum_required(VERSION 3.10.0)

set(SRC
        src/game.cpp
)

add_executable(game ${SRC})

target_link_libraries(game PRIVATE game_engine)
