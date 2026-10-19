/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionSystem header
*/

#ifndef GAME_COLLISIONSYSTEM_H
#define GAME_COLLISIONSYSTEM_H

#include "ecs/Registry.h"

namespace Game {

    /**
     * @brief System responsible for AABB bounding box collision checks between entities.
     */
    class CollisionSystem {
    public:
        static void addSystem(RType::Registry &registry);
    };

} // namespace Game

#endif // GAME_COLLISIONSYSTEM_H
