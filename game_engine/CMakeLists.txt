cmake_minimum_required(VERSION 3.10.0)

set(SRC
        src/ecs/Entity.cpp
        src/ecs/Entity.h
        src/ecs/SparseArray.h
        src/ecs/Zipper.h
        src/ecs/IndexedZipper.h
)

add_library(game_engine STATIC ${SRC})

target_include_directories(game_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (BUILD_TESTS)
    CPMAddPackage("gh:catchorg/Catch2@3.4.0")

    add_executable(game_engine_tests
            tests/ecs_unit_tests.cpp
    )

    target_link_libraries(game_engine_tests PRIVATE game_engine Catch2::Catch2WithMain)

    add_test(NAME game_engine_tests COMMAND game_engine_tests)
endif ()
