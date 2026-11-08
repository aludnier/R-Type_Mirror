/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameBindings.cpp
*/

#include "scripting/GameBindings.h"

#include <cstddef>
#include <string>

#include "components/Enemy.h"
#include "components/ForcePod.h"
#include "components/Player.h"
#include "components/Projectile.h"

namespace Game {
namespace {
    void bindEnemy(sol::state &lua)
    {
        lua.new_usertype<Enemy>("Enemy",
            sol::constructors<Enemy(), Enemy(std::string, float, int, float)>(),
            "enemy_type",
            &Enemy::enemyType,
            "shoot_timer",
            &Enemy::shootTimer,
            "shoot_interval",
            &Enemy::shootInterval,
            "score_value",
            &Enemy::scoreValue,
            "wave_frequency",
            &Enemy::waveFrequency,
            "wave_amplitude",
            &Enemy::waveAmplitude,
            "initial_y",
            &Enemy::initialY,
            "elapsed_time",
            &Enemy::elapsedTime);
    }

    void bindProjectile(sol::state &lua)
    {
        lua.new_usertype<Projectile>("Projectile",
            sol::constructors<Projectile(),
                Projectile(bool, int, bool, float)>(),
            "is_player_bullet",
            &Projectile::isPlayerBullet,
            "damage",
            &Projectile::damage,
            "is_charged_beam",
            &Projectile::isChargedBeam,
            "speed",
            &Projectile::speed);
    }

    void bindPlayer(sol::state &lua)
    {
        lua.new_usertype<Player>("Player",
            sol::constructors<Player(), Player(int, bool)>(),
            "score",
            &Player::score,
            "lives",
            &Player::lives,
            "shoot_cooldown",
            &Player::shootCooldown,
            "charge_time",
            &Player::chargeTime,
            "is_charging",
            &Player::isCharging,
            "has_force_pod",
            &Player::hasForcePod);
    }

    void bindForcePod(sol::state &lua)
    {
        lua.new_usertype<ForcePod>("ForcePod",
            sol::constructors<ForcePod(),
                ForcePod(std::size_t),
                ForcePod(std::size_t, float, float, int)>(),
            "owner_id",
            &ForcePod::ownerId,
            "offset_x",
            &ForcePod::offsetX,
            "offset_y",
            &ForcePod::offsetY,
            "level",
            &ForcePod::level);
    }
} // namespace

void bindGameComponents(RType::ScriptEngine &scripts)
{
    sol::state &lua = scripts.lua();

    bindEnemy(lua);
    bindProjectile(lua);
    bindPlayer(lua);
    bindForcePod(lua);
    scripts.bindComponent<Enemy>("Enemy");
    scripts.bindComponent<Projectile>("Projectile");
    scripts.bindComponent<Player>("Player");
    scripts.bindComponent<ForcePod>("ForcePod");
}
} // namespace Game
