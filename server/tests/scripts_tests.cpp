#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <optional>
#include <vector>

#include "Enemy.h"
#include "GameProtocol.h"
#include "RTypeScripting.h"
#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "math/Vector2.h"
#include "scripting/ScriptComponent.h"
#include "scripting/ScriptEngine.h"

namespace {
using Catch::Approx;
using Game::Protocol::EntityKind;

constexpr float SCREEN_WIDTH = 1920.f;
constexpr float SCREEN_HEIGHT = 1080.f;
// Exact in binary: the timers of the scripts reach their interval exactly.
constexpr float TICK = 0.25f;

struct SpawnRequest {
    EntityKind kind {};
    RType::Vector2 position;
    std::optional<RType::Vector2> velocity;
};

const char *scriptOf(EntityKind kind)
{
    switch (kind) {
        case EntityKind::Gunner:
            return "gunner.lua";
        case EntityKind::Boss:
            return "boss.lua";
        case EntityKind::Walker:
            return "walker.lua";
        default:
            return "bydo.lua";
    }
}

/**
 * @brief The scripted part of RTypeServer, without the network: scripts
 *        queue spawns, and the enemies spawned at the end of the tick run
 *        bydo.lua.
 */
struct ScriptedGame {
    ScriptedGame() :
        scripts(registry)
    {
        registry.registerComponent<RType::Transform2D>();
        registry.registerComponent<RType::Health>();
        registry.registerComponent<RType::Hitbox>();
        registry.registerComponent<RType::Enemy>();
        registry.registerComponent<RType::Boss>();
        registry.registerComponent<RType::Controllable>();
        RType::bindRTypeScripting(scripts,
            {SCREEN_WIDTH, SCREEN_HEIGHT},
            [this](EntityKind kind,
                RType::Vector2 position,
                std::optional<RType::Vector2>
                    velocity) {
                pending.push_back({kind, position, velocity});
            });
        // waves.lua only spawns enemies while a player is alive.
        player = registry.spawnEntity();
        registry.addComponent(player, RType::Controllable {});
        registry.addComponent(player, RType::Transform2D {{100.f, 500.f}});
        // Bound by RTypeServer: here it only records the hits.
        scripts.lua()["rtype"]["hit_player"] = [this](std::size_t entity) {
            hits.push_back(entity);
        };
    }

    void tick()
    {
        scripts.setDeltaTime(TICK);
        scripts.update(registry.getComponents<RType::ScriptComponent>());
        for (const auto &request : pending) {
            spawned.push_back(request);
            if (request.kind != EntityKind::Enemy &&
                request.kind != EntityKind::Gunner &&
                request.kind != EntityKind::Boss &&
                request.kind != EntityKind::Walker) {
                continue;
            }
            RType::Entity entity = registry.spawnEntity();

            registry.addComponent(
                entity, RType::Transform2D {request.position});
            registry.addComponent(
                entity, RType::Enemy {request.position.y, 0.f, 0.f});
            registry.addComponent(
                entity, RType::ScriptComponent {{scriptOf(request.kind)}});
            if (request.kind == EntityKind::Boss) {
                registry.addComponent(entity, RType::Boss {});
                registry.addComponent(entity, RType::Health {60});
            }
            enemies.push_back(entity);
        }
        pending.clear();
    }

    [[nodiscard]] const RType::Vector2 &position(RType::Entity entity)
    {
        return registry.getComponents<RType::Transform2D>()[entity]->position;
    }

    RType::Registry registry;
    RType::ScriptEngine scripts;
    std::vector<SpawnRequest> pending;
    std::vector<SpawnRequest> spawned;
    std::vector<RType::Entity> enemies;
    std::vector<std::size_t> hits;
    RType::Entity player = RType::Registry::entityFromIndex(0);
};
} // namespace

TEST_CASE(
    "The server scripts spawn Bydo scouts that fly left", "[server][scripting]")
{
    ScriptedGame game;
    game.scripts.loadConfigFile(SCRIPTS_CONFIG_FILE);

    // waves.lua: one enemy every 1.5 s, past the right edge.
    for (int i = 0; i < 5; ++i) {
        game.tick();
    }
    CHECK(game.spawned.empty());
    game.tick();
    REQUIRE(game.spawned.size() == 1);
    CHECK(game.spawned[0].kind == EntityKind::Enemy);
    CHECK(game.spawned[0].position.x == Approx(SCREEN_WIDTH + 40.f));
    CHECK(game.spawned[0].position.y >= 100.f);
    CHECK(game.spawned[0].position.y <= SCREEN_HEIGHT - 100.f);

    // bydo.lua: flies left on a sine wave around its spawn height.
    const RType::Entity bydo = game.enemies.at(0);
    const float baseY = game.spawned[0].position.y;
    game.tick();
    CHECK(game.position(bydo).x == Approx(SCREEN_WIDTH + 40.f - 150.f * TICK));
    CHECK(game.position(bydo).y != Approx(baseY));
    CHECK(game.position(bydo).y >= baseY - 60.f);
    CHECK(game.position(bydo).y <= baseY + 60.f);

    // Scouts do not shoot: only the next enemy comes, 1.5 s after the
    // first one.
    for (int i = 0; i < 10; ++i) {
        game.tick();
    }
    for (const auto &request : game.spawned) {
        CHECK(request.kind == EntityKind::Enemy);
    }
    CHECK(game.spawned.size() == 2);
    CHECK(game.scripts.lastError().empty());
}

TEST_CASE("Gunners shoot once on screen", "[server][scripting]")
{
    ScriptedGame game;
    game.scripts.loadConfigFile(SCRIPTS_CONFIG_FILE);
    REQUIRE(game.scripts.loadString(R"(rtype.spawn("Gunner", 1800, 540))"));
    game.tick();
    REQUIRE_FALSE(game.enemies.empty());
    const RType::Entity gunner = game.enemies.front();

    // 1.5 s between two shots: the first one after six ticks of 0.25 s.
    std::size_t bullets = 0;
    for (int i = 0; i < 6; ++i) {
        game.tick();
    }
    for (const auto &request : game.spawned) {
        if (request.kind == EntityKind::EnemyBullet) {
            ++bullets;
            CHECK(request.position.x == Approx(game.position(gunner).x - 25.f));
        }
    }
    CHECK(bullets == 1);
    CHECK(game.position(gunner).x < 1800.f);
    CHECK(game.scripts.lastError().empty());
}

TEST_CASE("The boss shoots a fan of bullets", "[server][scripting]")
{
    ScriptedGame game;
    game.scripts.loadConfigFile(SCRIPTS_CONFIG_FILE);
    REQUIRE(game.scripts.loadString(R"(rtype.spawn("Boss", 1500, 540))"));
    game.tick();
    REQUIRE_FALSE(game.enemies.empty());
    const RType::Entity boss = game.enemies.front();

    for (int i = 0; i < 8; ++i) {
        game.tick();
    }
    std::vector<SpawnRequest> bullets;
    for (const auto &request : game.spawned) {
        if (request.kind == EntityKind::EnemyBullet) {
            bullets.push_back(request);
        }
    }
    REQUIRE(bullets.size() == 5);
    for (const auto &bullet : bullets) {
        REQUIRE(bullet.velocity.has_value());
        CHECK(bullet.velocity->x < 0.f);
    }
    CHECK(bullets.front().velocity->y < 0.f);
    CHECK(bullets.back().velocity->y > 0.f);
    // Its health was set from the level.
    CHECK(game.registry.getComponents<RType::Health>()[boss]->max == 60);
    CHECK(game.scripts.lastError().empty());
}

TEST_CASE(
    "A level ends with a boss, then the next one starts", "[server][scripting]")
{
    ScriptedGame game;
    game.scripts.loadConfigFile(SCRIPTS_CONFIG_FILE);

    // 45 s of waves, then the boss 4 s later.
    std::optional<RType::Entity> boss;
    for (int i = 0; i < 250 && !boss; ++i) {
        game.tick();
        for (std::size_t j = 0; j < game.spawned.size(); ++j) {
            if (game.spawned[j].kind == EntityKind::Boss) {
                boss = game.enemies.back();
            }
        }
    }
    REQUIRE(boss.has_value());
    CHECK(game.scripts.loadString("assert(rtype.level == 1)"));

    game.tick();
    game.registry.killEntity(*boss);
    game.registry.killAllEntities();
    game.tick();
    CHECK(game.scripts.loadString(
        "assert(rtype.level == 2 and rtype.speed_scale > 1)"));
    CHECK(game.scripts.lastError().empty());
}

TEST_CASE("Walkers walk on the ground and shoot upwards", "[server][scripting]")
{
    ScriptedGame game;
    game.scripts.loadConfigFile(SCRIPTS_CONFIG_FILE);
    REQUIRE(game.scripts.loadString(R"(rtype.spawn("Walker", 1800, 500))"));
    game.tick();
    REQUIRE_FALSE(game.enemies.empty());
    const RType::Entity walker = game.enemies.front();

    game.tick();
    CHECK(game.position(walker).y == Approx(Game::Protocol::GROUND_TOP - 21.f));
    CHECK(game.position(walker).x < 1800.f);
    for (int i = 0; i < 9; ++i) {
        game.tick();
    }
    bool shot = false;
    for (const auto &request : game.spawned) {
        if (request.kind == EntityKind::EnemyBullet) {
            shot = true;
            REQUIRE(request.velocity.has_value());
            CHECK(request.velocity->x < 0.f);
            CHECK(request.velocity->y < 0.f);
        }
    }
    CHECK(shot);
    CHECK(game.scripts.lastError().empty());
}

TEST_CASE("A player that touches the ground is hit", "[server][scripting]")
{
    ScriptedGame game;
    game.scripts.loadConfigFile(SCRIPTS_CONFIG_FILE);
    const RType::Entity flying = game.registry.spawnEntity();
    const RType::Entity low = game.registry.spawnEntity();
    for (const auto &[entity, y] : {std::pair {flying, 500.f},
             std::pair {low, Game::Protocol::GROUND_TOP - 10.f}}) {
        game.registry.addComponent(entity, RType::Controllable {});
        game.registry.addComponent(entity, RType::Transform2D {{100.f, y}});
    }

    game.tick();

    CHECK(game.hits == std::vector<std::size_t> {low});
    CHECK(game.scripts.lastError().empty());
}

TEST_CASE("Without players the game waits at level 1", "[server][scripting]")
{
    ScriptedGame game;
    game.scripts.loadConfigFile(SCRIPTS_CONFIG_FILE);
    REQUIRE(game.scripts.loadString("rtype.level = 3"));

    game.registry.killEntity(game.player);
    game.registry.killAllEntities();
    for (int i = 0; i < 12; ++i) {
        game.tick();
    }

    CHECK(game.spawned.empty());
    CHECK(game.scripts.loadString(
        "assert(rtype.level == 1 and rtype.speed_scale == 1)"));
    CHECK(game.scripts.lastError().empty());
}

TEST_CASE("rtype.spawn takes an optional velocity", "[server][scripting]")
{
    ScriptedGame game;

    REQUIRE(game.scripts.loadString(
        R"(rtype.spawn("EnemyBullet", 10, 20, -100, 50))"));
    REQUIRE(game.pending.size() == 1);
    REQUIRE(game.pending[0].velocity.has_value());
    CHECK(game.pending[0].velocity->x == Approx(-100.f));
    CHECK(game.pending[0].velocity->y == Approx(50.f));
}

TEST_CASE("rtype.spawn only accepts the kinds scripts may spawn",
    "[server][scripting]")
{
    ScriptedGame game;

    REQUIRE(game.scripts.loadString(R"(
        assert(rtype.SCREEN_WIDTH == 1920 and rtype.SCREEN_HEIGHT == 1080)
        rtype.spawn("EnemyBullet", 10, 20)
    )"));
    REQUIRE(game.pending.size() == 1);
    CHECK(game.pending[0].kind == EntityKind::EnemyBullet);
    CHECK(game.pending[0].position.x == Approx(10.f));
    CHECK(game.pending[0].position.y == Approx(20.f));

    REQUIRE(game.scripts.loadString(R"(rtype.spawn("PowerUp", 30, 40))"));
    REQUIRE(game.pending.size() == 2);
    CHECK(game.pending[1].kind == EntityKind::PowerUp);
    game.pending.pop_back();

    CHECK_FALSE(game.scripts.loadString(R"(rtype.spawn("Player", 0, 0))"));
    CHECK_THAT(game.scripts.lastError(),
        Catch::Matchers::ContainsSubstring("unknown kind 'Player'"));
    CHECK(game.pending.size() == 1);
}
