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
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRITERION REQUIRED criterion)

    add_executable(game_engine_tests
            tests/ecs_unit_tests.cpp
    )

    target_link_libraries(game_engine_tests PRIVATE game_engine ${CRITERION_LINK_LIBRARIES})
    target_include_directories(game_engine_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CRITERION_INCLUDE_DIRS})
    target_compile_options(game_engine_tests PRIVATE ${CRITERION_CFLAGS_OTHER})

    add_test(NAME game_engine_tests COMMAND game_engine_tests)
endif ()
