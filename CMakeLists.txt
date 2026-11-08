cmake_minimum_required(VERSION 3.21)
project(R-Type VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build the tests" ON)

include(CTest)
enable_testing()

configure_file(config/game.json ${CMAKE_BINARY_DIR}/config/game.json COPYONLY)
configure_file(config/assets.json ${CMAKE_BINARY_DIR}/config/assets.json COPYONLY)
configure_file(config/scripts.json ${CMAKE_BINARY_DIR}/config/scripts.json COPYONLY)
configure_file(config/game_scripts.json ${CMAKE_BINARY_DIR}/config/game_scripts.json COPYONLY)
configure_file(config/inputs.json ${CMAKE_BINARY_DIR}/config/inputs.json COPYONLY)

file(GLOB _GAME_SCRIPTS "game/scripts/*.lua")
file(COPY ${_GAME_SCRIPTS} DESTINATION ${CMAKE_BINARY_DIR}/game/scripts)

file(GLOB _UI_DOCS "config/ui/*.json")
file(COPY ${_UI_DOCS} DESTINATION ${CMAKE_BINARY_DIR}/config/ui)

add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(server)
