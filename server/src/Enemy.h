/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Enemy.h
*/

#ifndef ENEMY_H
#define ENEMY_H

namespace RType {
/**
 * @brief A Bydo. Its movement is scripted (server/scripts/bydo.lua), these
 *        fields are its state, readable from Lua as `base_y`, `elapsed` and
 *        `shoot_timer`.
 */
struct Enemy {
    /** @brief Height it was spawned at, the center of its wave. */
    float baseY = 0.f;
    /** @brief Seconds since it was spawned. */
    float elapsed = 0.f;
    /** @brief Seconds since its last shot. */
    float shootTimer = 0.f;
};

/** @brief Marks the boss, so waves.lua knows when it is shot down. */
struct Boss {};

/** @brief Marks a pickup that gives the double shot to the player. */
struct PowerUp {};
} // namespace RType

#endif // ENEMY_H
