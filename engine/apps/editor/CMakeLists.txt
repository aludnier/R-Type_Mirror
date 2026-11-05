# editor: the engine's scene editor.
add_executable(editor main.cpp)

target_link_libraries(editor PRIVATE Eng::editor)
