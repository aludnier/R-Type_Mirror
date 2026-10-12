cmake_minimum_required(VERSION 3.10.0)

set(SRC
        src/ecs/Registry.cpp
        src/network/ByteReader.cpp
        src/network/ByteWriter.cpp
)

add_library(game_engine STATIC ${SRC})

target_include_directories(game_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
