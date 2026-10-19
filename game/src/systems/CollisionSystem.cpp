/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionSystem implementation
*/

#include "systems/CollisionSystem.h"
#include "ecs/Entity.h"
#include "ecs/SparseArray.h"
#include "components/Hitbox.h"
#include "components/Health.h"
#include "components/Player.h"
#include "components/Enemy.h"
#include "components/Projectile.h"
#include "components/Transform2D.h"
#include <cmath>

namespace Game {

    void CollisionSystem::addSystem(RType::Registry &registry)
    {
        // Note: Feature not available in engine_client: Collision events / physics pipeline
        // is implemented in modules/physics2D, but client engine standalone lacks built-in
        // collision dispatching. We implement an ECS collision check here.
        registry.addSystem<RType::Transform2D, Hitbox, Health>(
            [&registry](auto &transforms, auto &hitboxes, auto &healths) {
                auto &projectiles = registry.getComponents<Projectile>();
                auto &players = registry.getComponents<Player>();
                auto &enemies = registry.getComponents<Enemy>();

                for (size_t i = 0; i < transforms.size(); ++i) {
                    if (!transforms[i].has_value() || !hitboxes[i].has_value() || !healths[i].has_value())
                        continue;

                    for (size_t j = 0; j < transforms.size(); ++j) {
                        if (i == j || !transforms[j].has_value() || !hitboxes[j].has_value())
                            continue;

                        // Check projectile collision against target
                        if (j < projectiles.size() && projectiles[j].has_value()) {
                            const auto &proj = projectiles[j].value();

                            // Player bullet hits enemy OR Enemy bullet hits player
                            bool isEnemyTarget = (i < enemies.size() && enemies[i].has_value());
                            bool isPlayerTarget = (i < players.size() && players[i].has_value());

                            if ((proj.isPlayerBullet && isEnemyTarget) || (!proj.isPlayerBullet && isPlayerTarget)) {
                                const auto &posA = transforms[i]->position;
                                const auto &sizeA = hitboxes[i]->size;
                                const auto &posB = transforms[j]->position;
                                const auto &sizeB = hitboxes[j]->size;

                                // AABB overlap check
                                bool overlapX = std::abs(posA.x - posB.x) * 2 < (sizeA.x + sizeB.x);
                                bool overlapY = std::abs(posA.y - posB.y) * 2 < (sizeA.y + sizeB.y);

                                if (overlapX && overlapY) {
                                    healths[i]->takeDamage(static_cast<int>(proj.damage));

                                    if (!proj.isChargedBeam) {
                                        registry.killEntity(RType::Registry::entityFromIndex(j));
                                    }

                                    if (healths[i]->isDead()) {
                                        registry.killEntity(RType::Registry::entityFromIndex(i));
                                        // Note: Audio playback feature not available in engine_client.
                                        // Trigger explosion sound / animation here when audio backend is added.
                                    }
                                }
                            }
                        }
                    }
                }
            });
    }

} // namespace Game
