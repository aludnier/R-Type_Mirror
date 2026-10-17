cmake_minimum_required(VERSION 3.21)
project(R-Type VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build the tests" ON)
option(BUILD_SERVER "Build the server (r-type_server)" ON)
option(BUILD_CLIENT "Build the client (r-type_client)" ON)

include(CTest)
enable_testing()

include(cmake/CPM.cmake)

add_subdirectory(game_engine)
if (BUILD_CLIENT)
    add_subdirectory(game)
endif ()
if (BUILD_SERVER)
    add_subdirectory(server)
endif ()
add_subdirectory(modules/example)
add_subdirectory(modules/physics2D)
