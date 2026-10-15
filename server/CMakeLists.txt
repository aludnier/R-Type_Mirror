set(SRC
        src/main.cpp
        src/ClientSession.cpp
        src/GameServer.cpp
)

add_executable(r-type_server ${SRC})

target_link_libraries(r-type_server PRIVATE game_engine)

if (BUILD_TESTS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CRITERION REQUIRED criterion)

    add_executable(server_tests
            tests/server_sending_tests.cpp
            src/ClientSession.cpp
            src/GameServer.cpp
    )

    target_link_libraries(server_tests PRIVATE game_engine ${CRITERION_LINK_LIBRARIES})
    target_include_directories(server_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CRITERION_INCLUDE_DIRS})
    target_compile_options(server_tests PRIVATE ${CRITERION_CFLAGS_OTHER})

    add_test(NAME server_tests COMMAND server_tests)
endif ()
