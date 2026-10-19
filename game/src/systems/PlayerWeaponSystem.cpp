/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PlayerWeaponSystem implementation
*/

#include "systems/PlayerWeaponSystem.h"
#include "ecs/Entity.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"
#include "components/Player.h"
#include "components/Rigidbody2D.h"
#include "components/Projectile.h"
#include "components/Hitbox.h"
#include "components/Transform2D.h"
#include "components/Controllable.h"
#include "components/Action.h"

namespace Game {

    void PlayerWeaponSystem::addSystem(RType::Registry &registry, InputManager &input, float dt)
    {
        registry.addSystem<RType::Transform2D, Player, RType::Controllable>(
            [&registry, &input, dt](auto &transforms, auto &players, auto &controllables) {
                for (auto [transform, player, controllable] : RType::Zipper(transforms, players, controllables)) {
                    if (player.shootCooldown > 0.0f) {
                        player.shootCooldown -= dt;
                    }

                    bool shootPressed = input.isActionDown(Action::Shoot);

                    // R-Type Wave Cannon Charging mechanic
                    if (shootPressed) {
                        player.isCharging = true;
                        player.chargeTime += dt;
                    } else if (player.isCharging) {
                        // Released fire button -> Shoot standard bullet or charged beam
                        player.isCharging = false;
                        auto bullet = registry.spawnEntity();

                        if (player.chargeTime >= 1.5f) {
                            // Charged Wave Cannon Beam!
                            registry.addComponent<RType::Transform2D>(bullet, RType::Transform2D({transform.position.x + 35.0f, transform.position.y}));
                            RType::Physics2D::Rigidbody2D body;
                            body.velocity = {900.0f, 0.0f};
                            registry.addComponent<RType::Physics2D::Rigidbody2D>(bullet, std::move(body));
                            registry.addComponent<Projectile>(bullet, Projectile(true, 5, true, 900.0f));
                            registry.addComponent<Hitbox>(bullet, Hitbox(RType::Vector2{48.0f, 24.0f}));
                        } else if (player.shootCooldown <= 0.0f) {
                            // Standard primary laser
                            registry.addComponent<RType::Transform2D>(bullet, RType::Transform2D({transform.position.x + 25.0f, transform.position.y}));
                            RType::Physics2D::Rigidbody2D body;
                            body.velocity = {700.0f, 0.0f};
                            registry.addComponent<RType::Physics2D::Rigidbody2D>(bullet, std::move(body));
                            registry.addComponent<Projectile>(bullet, Projectile(true, 1, false, 700.0f));
                            registry.addComponent<Hitbox>(bullet, Hitbox(RType::Vector2{16.0f, 8.0f}));
                            player.shootCooldown = 0.15f;
                        }
                        player.chargeTime = 0.0f;
                    }
                }
            });
    }

} // namespace Game
