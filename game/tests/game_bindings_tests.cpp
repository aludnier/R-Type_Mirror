#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "components/Enemy.h"
#include "components/ForcePod.h"
#include "components/Player.h"
#include "components/Projectile.h"
#include "ecs/Registry.h"
#include "scripting/GameBindings.h"
#include "scripting/ScriptEngine.h"

using Catch::Approx;

TEST_CASE("Scripts add and edit the game components", "[game][scripting]")
{
    RType::Registry registry;
    registry.registerComponent<Game::Enemy>();
    registry.registerComponent<Game::Projectile>();
    registry.registerComponent<Game::Player>();
    registry.registerComponent<Game::ForcePod>();
    RType::ScriptEngine scripts(registry);
    Game::bindGameComponents(scripts);

    REQUIRE(scripts.loadString(R"(
        player = world.spawn()
        local p = world.add(player, "Player", Player.new(5, true))
        p.score = p.score + 300
        world.add(player, "ForcePod", ForcePod.new(player, 60, -10, 2))

        bydo = world.spawn()
        local enemy = world.add(bydo, "Enemy", Enemy.new("BydoTank", 1.5, 250, 400))
        enemy.elapsed_time = 0.5

        shot = world.spawn()
        world.add(shot, "Projectile").damage = 3

        assert(world.has(player, "Player") and not world.has(bydo, "Player"))
        assert(world.get(bydo, "Enemy").enemy_type == "BydoTank")
    )"));
    CHECK(scripts.lastError().empty());

    const auto player = scripts.lua()["player"].get<RType::entity_id_type>();
    const auto bydo = scripts.lua()["bydo"].get<RType::entity_id_type>();
    const auto shot = scripts.lua()["shot"].get<RType::entity_id_type>();
    const auto &p = *registry.getComponents<Game::Player>()[player];
    const auto &pod = *registry.getComponents<Game::ForcePod>()[player];
    const auto &enemy = *registry.getComponents<Game::Enemy>()[bydo];
    const auto &projectile = *registry.getComponents<Game::Projectile>()[shot];

    CHECK(p.lives == 5);
    CHECK(p.hasForcePod);
    CHECK(p.score == 300);
    CHECK(pod.ownerId == player);
    CHECK(pod.offsetX == Approx(60.f));
    CHECK(pod.level == 2);
    CHECK(enemy.enemyType == "BydoTank");
    CHECK(enemy.scoreValue == 250);
    CHECK(enemy.initialY == Approx(400.f));
    CHECK(enemy.elapsedTime == Approx(0.5f));
    CHECK(projectile.damage == 3);
    CHECK(projectile.isPlayerBullet);
}
