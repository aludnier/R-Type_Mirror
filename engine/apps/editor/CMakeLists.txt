# editor: the engine's scene editor.
add_executable(editor main.cpp)

target_link_libraries(editor PRIVATE Eng::editor Eng::physics2d_scripting)

# The modules listed in the game config are loaded at start, from the
# build's module directory.
target_compile_definitions(editor PRIVATE ENGINE_MODULE_DIR="${ENGINE_MODULE_DIR}")
add_dependencies(editor physics2d_module environment2d_module lifetime_module)
