cmake_minimum_required(VERSION 3.21)
project(R-Type VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build the tests" ON)

include(CTest)
enable_testing()

add_subdirectory(game_engine)
add_subdirectory(game)
add_subdirectory(server)
add_subdirectory(modules/example)
add_subdirectory(modules/physics2D)
add_subdirectory(modules/environment2D)
