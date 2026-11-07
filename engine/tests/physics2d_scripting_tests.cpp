#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Physics2DBindings.h"
#include "components/BoxCollider2D.h"
#include "components/CircleCollider2D.h"
#include "components/CollisionEvents2D.h"
#include "components/PhysicsSettings2D.h"
#include "components/Rigidbody2D.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "library/PluginManager.h"
#include "scripting/ScriptEngine.h"
#include "scripting/ScriptSystem.h"

using Catch::Approx;

TEST_CASE("Scripts edit the physics2D components", "[scripting][physics2d]")
{
    RType::Registry registry;
    registry.registerComponent<RType::Physics2D::Rigidbody2D>();
    registry.registerComponent<RType::Physics2D::BoxCollider2D>();
    registry.registerComponent<RType::Physics2D::CircleCollider2D>();
    registry.registerComponent<RType::Physics2D::PhysicsSettings2D>();
    RType::ScriptEngine scripts(registry);
    RType::Physics2D::bindScripting(scripts);

    REQUIRE(scripts.loadString(R"(
        e = world.spawn()
        local body = world.add(e, "Rigidbody2D")
        body.mass = 2
        body.velocity.x = 3
        body:add_impulse(Vector2.new(4, 0))
        body:add_force(Vector2.new(0, 10))
        local box = world.add(e, "BoxCollider2D", BoxCollider2D.new(Vector2.new(32, 16)))
        box.offset.y = 4
        box.is_trigger = true
        box.layer = 2
        world.add(e, "CircleCollider2D", CircleCollider2D.new(8))
        world.add(e, "PhysicsSettings2D").gravity = Vector2.new(0, 9.8)
    )"));

    const auto id = scripts.lua()["e"].get<RType::entity_id_type>();
    const auto &body =
        *registry.getComponents<RType::Physics2D::Rigidbody2D>()[id];
    const auto &box =
        *registry.getComponents<RType::Physics2D::BoxCollider2D>()[id];
    CHECK(body.mass == Approx(2.f));
    CHECK(body.velocity.x == Approx(5.f));
    CHECK(body.force().y == Approx(10.f));
    CHECK(box.size.x == Approx(32.f));
    CHECK(box.offset.y == Approx(4.f));
    CHECK(box.isTrigger);
    CHECK(box.layer == 2);
    CHECK(registry.getComponents<RType::Physics2D::CircleCollider2D>()[id]
              ->radius == Approx(8.f));
    CHECK(registry.getComponents<RType::Physics2D::PhysicsSettings2D>()[id]
              ->gravity.y == Approx(9.8f));
}

TEST_CASE("Scripts read the collisions of the loaded physics2D module",
    "[scripting][physics2d]")
{
    RType::PluginManager manager;
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    manager.loadModule(PHYSICS2D_MODULE_PATH, registry);
    REQUIRE(manager.isLoaded(RType::Physics2D::MODULE_NAME));
    RType::ScriptEngine scripts(registry);
    RType::Physics2D::bindScripting(scripts);
    // After the module systems: the script sees the events of this frame.
    RType::addScriptSystem(registry, scripts);

    REQUIRE(scripts.loadString(R"(
        world.add(world.spawn(), "PhysicsSettings2D").dt = 0.5

        mover = world.spawn()
        world.add(mover, "Transform2D")
        world.add(mover, "Rigidbody2D").velocity = Vector2.new(2, -1)

        wall = world.spawn()
        world.add(wall, "Transform2D")
        world.add(wall, "Rigidbody2D").is_static = true
        world.add(wall, "BoxCollider2D")
        world.add(wall, "CollisionEvents2D")

        ball = world.spawn()
        world.add(ball, "Transform2D", Transform2D.new(Vector2.new(0.5, 0)))
        world.add(ball, "Rigidbody2D")
        world.add(ball, "CircleCollider2D")

        hits = {}
        world.add_system("read_collisions", function(dt)
            local collisions = world.get(wall, "CollisionEvents2D")
            for _, event in ipairs(collisions.events) do
                if event.kind == CollisionKind.Enter and not event.is_trigger then
                    table.insert(hits, event.other)
                end
            end
        end)
    )"));
    registry.runSystems();

    CHECK(scripts.lastError().empty());
    const auto mover = scripts.lua()["mover"].get<RType::entity_id_type>();
    const auto &position =
        registry.getComponents<RType::Transform2D>()[mover]->position;
    CHECK(position.x == Approx(1.f));
    CHECK(position.y == Approx(-0.5f));
    const sol::table hits = scripts.lua()["hits"];
    REQUIRE(hits.size() == 1);
    CHECK(hits.get<RType::entity_id_type>(1) ==
        scripts.lua()["ball"].get<RType::entity_id_type>());
}
