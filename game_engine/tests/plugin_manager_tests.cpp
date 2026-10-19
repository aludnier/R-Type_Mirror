#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>
#include <vector>

#include "library/engine.h"
#include "plugins/TestPluginComponent.h"

TEST_CASE("Engine loads plugins that provide declared ECS components and systems",
          "[engine][plugins]") {
    EngineLib engine;

    engine.loadPlugins(TEST_PLUGIN_DIRECTORY,
                       {"TestPluginComponent"},
                       {"TestPluginSystem"});

    auto entity = engine.registry().spawnEntity();
    engine.registry().emplaceComponent<TestPluginComponent>(entity, 41);
    engine.registry().runSystems();

    REQUIRE(engine.registry()
                .getComponents<TestPluginComponent>()
                    [static_cast<RType::entity_id_type>(entity)]
                ->value == 42);
}

TEST_CASE("Plugin loading reports components with no owning plugin",
          "[engine][plugins]") {
    EngineLib engine;

    REQUIRE_THROWS_AS(
        engine.loadPlugins(TEST_PLUGIN_DIRECTORY, {"MissingComponent"}, {}),
        std::runtime_error);
}

TEST_CASE("Plugin requirements can only be configured once", "[engine][plugins]") {
    EngineLib engine;
    engine.loadPlugins(TEST_PLUGIN_DIRECTORY, {}, {});

    REQUIRE_THROWS_AS(
        engine.loadPlugins(TEST_PLUGIN_DIRECTORY, {}, {}),
        std::logic_error);
}
