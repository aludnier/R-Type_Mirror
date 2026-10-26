/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PlayerWeaponSystem implementation
*/

#include "systems/PlayerWeaponSystem.h"
#include "components/Action.h"
#include "components/Controllable.h"
#include "components/Player.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"

namespace Game {

void PlayerWeaponSystem::addSystem(
    RType::Registry &registry, InputManager &input, float dt)
{
    registry.addSystem<RType::Transform2D, Player, RType::Controllable>(
        [&input, dt](auto &transforms, auto &players, auto &controllables) {
            for (auto [transform, player, controllable] :
                RType::Zipper(transforms, players, controllables)) {
                if (player.shootCooldown > 0.0f) {
                    player.shootCooldown -= dt;
                }

                // R-Type Wave Cannon Charging mechanic
                if (input.isActionDown(Action::Shoot)) {
                    player.isCharging = true;
                    player.chargeTime += dt;
                } else if (player.isCharging) {
                    // Released fire button: the server spawns the laser or
                    // the charged beam and replicates it to every client.
                    player.isCharging = false;
                    player.chargeTime = 0.0f;
                }
            }
        });
}

} // namespace Game
