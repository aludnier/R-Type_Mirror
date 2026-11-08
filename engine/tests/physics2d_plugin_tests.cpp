#include <catch2/catch_test_macros.hpp>

#include "components/BoxCollider2D.h"
#include "components/CircleCollider2D.h"
#include "components/CollisionEvents2D.h"
#include "components/PhysicsSettings2D.h"
#include "components/Rigidbody2D.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "library/PluginManager.h"
#include "reflection/Reflection.h"

TEST_CASE("PluginManager loads and runs the production Physics2D module",
    "[plugins][physics2d]")
{
    RType::PluginManager manager;
    RType::Registry registry;

    registry.registerComponent<RType::Transform2D>();
    manager.loadModule(PHYSICS2D_MODULE_PATH, registry);

    REQUIRE(manager.isLoaded("physics2d"));
    REQUIRE(registry.getComponents<RType::Transform2D>().size() == 0);
    REQUIRE(
        registry.getComponents<RType::Physics2D::BoxCollider2D>().size() == 0);
    REQUIRE(
        registry.getComponents<RType::Physics2D::CircleCollider2D>().size() ==
        0);
    REQUIRE(
        registry.getComponents<RType::Physics2D::CollisionEvents2D>().size() ==
        0);
    REQUIRE(
        registry.getComponents<RType::Physics2D::Rigidbody2D>().size() == 0);
    REQUIRE(
        registry.getComponents<RType::Physics2D::PhysicsSettings2D>().size() ==
        0);

    const auto movingEntity = registry.spawnEntity();
    registry.emplaceComponent<RType::Transform2D>(movingEntity);
    registry.emplaceComponent<RType::Physics2D::Rigidbody2D>(movingEntity)
        ->velocity = {2.f, -1.f};
    registry
        .emplaceComponent<RType::Physics2D::PhysicsSettings2D>(movingEntity)
        ->dt = 0.5f;

    const auto staticEntity = registry.spawnEntity();
    registry.emplaceComponent<RType::Transform2D>(staticEntity);
    registry.emplaceComponent<RType::Physics2D::Rigidbody2D>(staticEntity)
        ->isStatic = true;
    registry.emplaceComponent<RType::Physics2D::BoxCollider2D>(staticEntity);
    registry.emplaceComponent<RType::Physics2D::CollisionEvents2D>(
        staticEntity);

    const auto collidingEntity = registry.spawnEntity();
    registry.emplaceComponent<RType::Transform2D>(collidingEntity)
        .value()
        .position = {0.5f, 0.f};
    registry.emplaceComponent<RType::Physics2D::Rigidbody2D>(collidingEntity);
    registry.emplaceComponent<RType::Physics2D::BoxCollider2D>(collidingEntity);
    registry.emplaceComponent<RType::Physics2D::CollisionEvents2D>(
        collidingEntity);

    const auto circleEntity = registry.spawnEntity();
    registry.emplaceComponent<RType::Transform2D>(circleEntity)
        .value()
        .position = {-0.25f, 0.f};
    registry.emplaceComponent<RType::Physics2D::Rigidbody2D>(circleEntity);
    registry.emplaceComponent<RType::Physics2D::CircleCollider2D>(circleEntity);
    registry.emplaceComponent<RType::Physics2D::CollisionEvents2D>(
        circleEntity);

    registry.runSystems();

    REQUIRE(
        registry.getComponents<RType::Transform2D>()[movingEntity]->position ==
        RType::Vector2 {1.f, -0.5f});
    REQUIRE(
        registry
            .getComponents<RType::Physics2D::CollisionEvents2D>()[staticEntity]
            ->events.size() == 2);
    REQUIRE(registry
                .getComponents<
                    RType::Physics2D::CollisionEvents2D>()[collidingEntity]
                ->events.size() == 2);
    REQUIRE(
        registry
            .getComponents<RType::Physics2D::CollisionEvents2D>()[circleEntity]
            ->events.size() == 2);
    REQUIRE(registry.getComponents<RType::Transform2D>()[collidingEntity]
                ->position.x != 0.5f);
    REQUIRE(registry.getComponents<RType::Transform2D>()[circleEntity]
                ->position.x != -0.25f);
}

TEST_CASE("The Physics2D module describes its components to the editor",
    "[plugins][physics2d][reflection]")
{
    RType::PluginManager manager;
    RType::Registry registry;
    RType::Reflection reflection;

    manager.loadModule(PHYSICS2D_MODULE_PATH, registry);
    manager.reflectModules(reflection);

    for (const char *name : {"Rigidbody2D",
             "BoxCollider2D",
             "CircleCollider2D",
             "PhysicsSettings2D"})
        CHECK(reflection.find(name) != nullptr);
    CHECK(reflection.find("CollisionEvents2D") == nullptr);

    const RType::ComponentInfo *body = reflection.find("Rigidbody2D");
    REQUIRE(body != nullptr);
    const auto entity = registry.spawnEntity();
    body->add(registry, entity);
    REQUIRE(body->has(registry, entity));
    for (const RType::FieldInfo &field : body->fields)
        if (field.name == "mass")
            field.as<float>(body->get(registry, entity)) = 3.f;
    CHECK(
        registry.getComponents<RType::Physics2D::Rigidbody2D>()[entity]->mass ==
        3.f);
}

TEST_CASE("reflectModules adds nothing when no module is loaded",
    "[plugins][reflection]")
{
    RType::PluginManager manager;
    RType::Registry registry;
    RType::Reflection reflection;

    manager.reflectModules(reflection);

    CHECK(reflection.components().empty());
}
