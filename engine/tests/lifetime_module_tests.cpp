#include <catch2/catch_test_macros.hpp>

#include "components/Lifetime.h"
#include "components/Timer.h"
#include "components/TimerEvent.h"
#include "components/TimerSettings.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "library/PluginManager.h"

TEST_CASE("Lifetime module registers its ECS components", "[plugins][lifetime]")
{
    RType::PluginManager manager;
    RType::Registry registry;
    manager.loadModule(LIFETIME_MODULE_PATH, registry);

    REQUIRE(manager.isLoaded("lifetime"));
    REQUIRE(registry.isRegistered<RType::Lifetime::Lifetime>());
    REQUIRE(registry.isRegistered<RType::Lifetime::Timer>());
    REQUIRE(registry.isRegistered<RType::Lifetime::TimerEvents>());
    REQUIRE(registry.isRegistered<RType::Lifetime::TimerSettings>());
}

TEST_CASE("Lifetime system schedules destruction after elapsed duration",
    "[plugins][lifetime]")
{
    RType::PluginManager manager;
    RType::Registry registry;
    manager.loadModule(LIFETIME_MODULE_PATH, registry);
    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<RType::Lifetime::TimerSettings>(entity)->dt =
        0.5f;
    registry.emplaceComponent<RType::Lifetime::Lifetime>(entity)->duration =
        1.0f;

    registry.runSystems();
    REQUIRE(registry.isAlive(entity));
    registry.runSystems();
    REQUIRE_FALSE(registry.isAlive(entity));
}

TEST_CASE("Timer system emits one named event on expiry", "[plugins][lifetime]")
{
    RType::PluginManager manager;
    RType::Registry registry;
    manager.loadModule(LIFETIME_MODULE_PATH, registry);
    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<RType::Lifetime::TimerSettings>(entity)->dt =
        1.0f;
    registry.emplaceComponent<RType::Lifetime::Timer>(entity)->duration = 1.0f;
    registry.getComponents<RType::Lifetime::Timer>()[entity]->eventName =
        "expired";
    registry.emplaceComponent<RType::Lifetime::TimerEvents>(entity);

    registry.runSystems();
    const auto &events =
        registry.getComponents<RType::Lifetime::TimerEvents>()[entity]->events;
    REQUIRE(events.size() == 1);
    REQUIRE(events.front().name == "expired");
    registry.runSystems();
    REQUIRE(events.empty());
}
