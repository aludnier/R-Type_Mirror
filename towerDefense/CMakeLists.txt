add_executable(tower_defense
        src/main.cpp
        src/TowerDefense.cpp
        src/TowerDefense.h
)

target_link_libraries(tower_defense PRIVATE
        Eng::render2d
        Eng::scripting
        Eng::physics2d_scripting
)
target_compile_definitions(tower_defense PRIVATE
        ENGINE_MODULE_DIR="${ENGINE_MODULE_DIR}"
        TOWER_DEFENSE_ROOT="${CMAKE_SOURCE_DIR}"
)
add_dependencies(tower_defense physics2d_module environment2d_module)
set_target_properties(tower_defense PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/game"
)

if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(VCPKG_RUNTIME_BIN_DIR
                "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin")
    else ()
        set(VCPKG_RUNTIME_BIN_DIR
                "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    endif ()
    file(GLOB VCPKG_RUNTIME_DLLS
            "${VCPKG_RUNTIME_BIN_DIR}/*.dll")

    add_custom_command(TARGET tower_defense POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:tower_defense>"
            "$<TARGET_FILE_DIR:tower_defense>"
            COMMAND_EXPAND_LISTS
    )
    foreach (runtime_dll IN LISTS VCPKG_RUNTIME_DLLS)
        add_custom_command(TARGET tower_defense POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dll}"
                "$<TARGET_FILE_DIR:tower_defense>"
        )
    endforeach ()
endif ()
