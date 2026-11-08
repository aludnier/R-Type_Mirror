#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <vector>

#include "Enemy.h"
#include "GameProtocol.h"
#include "RTypeScripting.h"
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
};

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
        RType::bindRTypeScripting(scripts,
            {SCREEN_WIDTH, SCREEN_HEIGHT},
            [this](EntityKind kind, RType::Vector2 position) {
                pending.push_back({kind, position});
            });
    }

    void tick()
    {
        scripts.setDeltaTime(TICK);
        scripts.update(registry.getComponents<RType::ScriptComponent>());
        for (const auto &request : pending) {
            spawned.push_back(request);
            if (request.kind != EntityKind::Enemy) {
                continue;
            }
            RType::Entity entity = registry.spawnEntity();

            registry.addComponent(
                entity, RType::Transform2D {request.position});
            registry.addComponent(
                entity, RType::Enemy {request.position.y, 0.f, 0.f});
            registry.addComponent(
                entity, RType::ScriptComponent {{"bydo.lua"}});
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
};
} // namespace

TEST_CASE("The server scripts spawn Bydos that fly left and shoot",
    "[server][scripting]")
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

    // Shoots 2 s after it appeared, once on screen. A second enemy came
    // 1.5 s after the first one.
    for (int i = 0; i < 6; ++i) {
        game.tick();
    }
    CHECK(game.spawned.size() == 2);
    game.tick();
    REQUIRE(game.spawned.size() == 3);
    CHECK(game.spawned[1].kind == EntityKind::Enemy);
    CHECK(game.spawned[2].kind == EntityKind::EnemyBullet);
    CHECK(game.spawned[2].position.x == Approx(game.position(bydo).x - 20.f));
    CHECK(game.spawned[2].position.y == Approx(game.position(bydo).y));
    CHECK(game.scripts.lastError().empty());
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

    CHECK_FALSE(game.scripts.loadString(R"(rtype.spawn("Player", 0, 0))"));
    CHECK_THAT(game.scripts.lastError(),
        Catch::Matchers::ContainsSubstring("unknown kind 'Player'"));
    CHECK(game.pending.size() == 1);
}
