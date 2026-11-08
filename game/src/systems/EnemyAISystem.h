/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** EnemyAISystem header
*/

#ifndef GAME_ENEMYAISYSTEM_H
#define GAME_ENEMYAISYSTEM_H

#include "ecs/Registry.h"

namespace Game {

/**
 * @brief System responsible for enemy wave motion patterns and projectile
 * firing.
 */
class EnemyAISystem {
public:
    static void addSystem(RType::Registry &registry, float dt);
};

} // namespace Game

#endif // GAME_ENEMYAISYSTEM_H
