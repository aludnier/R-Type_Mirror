/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Projectile component implementation
*/

#include "components/Projectile.h"

namespace Game {

    Projectile::Projectile(bool isPlayer, int dmg, bool charged, float spd)
        : isPlayerBullet(isPlayer), damage(dmg), isChargedBeam(charged), speed(spd)
    {
    }

} // namespace Game
