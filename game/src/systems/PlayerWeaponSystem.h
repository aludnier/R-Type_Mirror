/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PlayerWeaponSystem header
*/

#ifndef GAME_PLAYERWEAPONSYSTEM_H
#define GAME_PLAYERWEAPONSYSTEM_H

#include "ecs/Registry.h"
#include "library/InputManager.h"

namespace Game {

/**
 * @brief System responsible for player firing and R-Type wave cannon beam
 * charging.
 */
class PlayerWeaponSystem {
public:
    static void addSystem(
        RType::Registry &registry, InputManager &input, float dt);
};

} // namespace Game

#endif // GAME_PLAYERWEAPONSYSTEM_H
