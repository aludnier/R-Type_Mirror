cmake_minimum_required(VERSION 3.10.0)
project(R-Type VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(game_engine)
add_subdirectory(game)

option(BUILD_TESTS "Build the tests" ON)

if (BUILD_TESTS)
    include(CTest)
    enable_testing()
endif ()
