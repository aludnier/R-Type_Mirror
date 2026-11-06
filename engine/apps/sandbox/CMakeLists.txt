# sandbox: small program to try the engine features without a game.
add_executable(sandbox main.cpp)

target_link_libraries(sandbox PRIVATE Eng::render2d)
target_compile_definitions(sandbox PRIVATE
        SANDBOX_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
