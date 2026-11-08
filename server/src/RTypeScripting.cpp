/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RTypeScripting.cpp
*/

#include "RTypeScripting.h"

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "Enemy.h"

namespace RType {
namespace {
    using Game::Protocol::EntityKind;

    // Players are spawned when a client connects, not by scripts.
    constexpr std::array<std::pair<std::string_view, EntityKind>, 7>
        SPAWNABLE_KINDS = {{
            {"PowerUp", EntityKind::PowerUp},
            {"Enemy", EntityKind::Enemy},
            {"Gunner", EntityKind::Gunner},
            {"Boss", EntityKind::Boss},
            {"PlayerLaser", EntityKind::PlayerLaser},
            {"ChargedBeam", EntityKind::ChargedBeam},
            {"EnemyBullet", EntityKind::EnemyBullet},
        }};

    EntityKind kindFromName(const std::string &name)
    {
        for (const auto &[kindName, kind] : SPAWNABLE_KINDS) {
            if (kindName == name) {
                return kind;
            }
        }
        throw std::runtime_error("rtype.spawn: unknown kind '" + name + "'");
    }
} // namespace

void bindRTypeScripting(
    ScriptEngine &scripts, Vector2 screenSize, SpawnFunction spawn)
{
    sol::state &lua = scripts.lua();

    lua.new_usertype<Enemy>("Enemy",
        sol::constructors<Enemy()>(),
        "base_y",
        &Enemy::baseY,
        "elapsed",
        &Enemy::elapsed,
        "shoot_timer",
        &Enemy::shootTimer);
    scripts.bindComponent<Enemy>("Enemy");
    lua.new_usertype<Boss>("Boss", sol::constructors<Boss()>());
    scripts.bindComponent<Boss>("Boss");
    lua.new_usertype<PowerUp>("PowerUp", sol::constructors<PowerUp()>());
    scripts.bindComponent<PowerUp>("PowerUp");

    sol::table rtype = lua.create_named_table("rtype");

    rtype["SCREEN_WIDTH"] = screenSize.x;
    rtype["SCREEN_HEIGHT"] = screenSize.y;
    rtype.set_function("spawn",
        [spawn = std::move(spawn)](const std::string &kind,
            float x,
            float y,
            std::optional<float>
                vx,
            std::optional<float>
                vy) {
            std::optional<Vector2> velocity;

            if (vx && vy) {
                velocity = Vector2 {*vx, *vy};
            }
            spawn(kindFromName(kind), {x, y}, velocity);
        });
}
} // namespace RType
