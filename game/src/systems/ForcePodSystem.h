/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ForcePodSystem header
*/

#ifndef GAME_FORCEPODSYSTEM_H
#define GAME_FORCEPODSYSTEM_H

#include "ecs/Registry.h"

namespace Game {

    /**
     * @brief System responsible for positioning Force Pod satellites relative to player ships.
     */
    class ForcePodSystem {
    public:
        static void addSystem(RType::Registry &registry);
    };

} // namespace Game

#endif // GAME_FORCEPODSYSTEM_H
