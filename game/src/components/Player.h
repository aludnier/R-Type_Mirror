/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Player component header
*/

#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include <cstdint>

namespace Game {

/**
 * @brief Identifies the player entity and stores player state.
 */
struct Player {
    uint32_t score = 0;
    int lives = 3;
    float shootCooldown = 0.0f;
    float chargeTime = 0.0f; // R-Type iconic wave cannon charge time
    bool isCharging = false;
    bool hasForcePod = false;

    Player() = default;
    Player(int lives, bool hasForcePod);
};

} // namespace Game

#endif // GAME_PLAYER_H
