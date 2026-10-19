/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Projectile component header
*/

#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

namespace Game {

    /**
     * @brief Identifies projectiles (player laser, charged beam, enemy plasma).
     */
    struct Projectile {
        bool isPlayerBullet = true;
        int damage = 1;
        bool isChargedBeam = false;
        float speed = 800.0f;

        Projectile() = default;
        Projectile(bool isPlayer, int dmg, bool charged, float spd);
    };

} // namespace Game

#endif // GAME_PROJECTILE_H
