/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** BoundaryCleanupSystem implementation
*/

#include "systems/BoundaryCleanupSystem.h"
#include "ecs/Entity.h"
#include "ecs/SparseArray.h"
#include "components/Projectile.h"
#include "components/Transform2D.h"

namespace Game {

    void BoundaryCleanupSystem::addSystem(RType::Registry &registry, float screenWidth, float screenHeight)
    {
        registry.addSystem<RType::Transform2D, Projectile>(
            [&registry, screenWidth, screenHeight](auto &transforms, auto &projectiles) {
                for (size_t i = 0; i < transforms.size(); ++i) {
                    if (transforms[i].has_value() && i < projectiles.size() && projectiles[i].has_value()) {
                        const auto &pos = transforms[i]->position;
                        if (pos.x < -100.0f || pos.x > screenWidth + 100.0f || pos.y < -100.0f || pos.y > screenHeight + 100.0f) {
                            registry.killEntity(RType::Registry::entityFromIndex(i));
                        }
                    }
                }
            });
    }

} // namespace Game
