/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** BoundaryCleanupSystem header
*/

#ifndef GAME_BOUNDARYCLEANUPSYSTEM_H
#define GAME_BOUNDARYCLEANUPSYSTEM_H

#include "ecs/Registry.h"

namespace Game {

/**
 * @brief System responsible for despawning out-of-bounds entities.
 */
class BoundaryCleanupSystem {
public:
    static void addSystem(RType::Registry &registry,
        float screenWidth = 1920.0f, float screenHeight = 1080.0f);
};

} // namespace Game

#endif // GAME_BOUNDARYCLEANUPSYSTEM_H
