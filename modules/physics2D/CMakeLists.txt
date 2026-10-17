cmake_minimum_required(VERSION 3.10.0)

add_library(physics2d_module SHARED
        src/Physics2DModule.cpp
        src/components/PhysicsSettings2D.cpp
        src/systems/OverlapTests.cpp
        src/systems/MovementSystem.cpp
        src/systems/CollisionDetectionSystem.cpp
        src/systems/CollisionEventEmitter.cpp
        src/systems/CollisionResponseSystem.cpp
)

target_include_directories(physics2d_module PUBLIC
        ${PROJECT_SOURCE_DIR}/game_engine/src
)

target_link_libraries(physics2d_module PRIVATE game_engine)

target_compile_features(physics2d_module PRIVATE cxx_std_23)

set_target_properties(physics2d_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "Physics2D_Module"
)