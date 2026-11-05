# sandbox: small program to try the engine features without a game.
add_executable(sandbox main.cpp)

target_link_libraries(sandbox PRIVATE Eng::platform)
