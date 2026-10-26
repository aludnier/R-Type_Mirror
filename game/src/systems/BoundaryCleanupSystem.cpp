/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** BoundaryCleanupSystem implementation
*/

#include "systems/BoundaryCleanupSystem.h"
#include "components/NetworkEntity.h"
#include "components/Projectile.h"
#include "components/Transform2D.h"
#include "ecs/Entity.h"
#include "ecs/SparseArray.h"

namespace Game {

void BoundaryCleanupSystem::addSystem(
    RType::Registry &registry, float screenWidth, float screenHeight)
{
    registry.addSystem<RType::Transform2D, Projectile>(
        [&registry, screenWidth, screenHeight](
            auto &transforms, auto &projectiles) {
            auto &netEntities = registry.getComponents<NetworkEntity>();

            for (size_t i = 0; i < transforms.size(); ++i) {
                // Replicated projectiles are despawned by the server.
                if (i < netEntities.size() && netEntities[i].has_value()) {
                    continue;
                }
                if (transforms[i].has_value() && i < projectiles.size() &&
                    projectiles[i].has_value()) {
                    const auto &pos = transforms[i]->position;
                    if (pos.x < -100.0f || pos.x > screenWidth + 100.0f ||
                        pos.y < -100.0f || pos.y > screenHeight + 100.0f) {
                        registry.killEntity(
                            RType::Registry::entityFromIndex(i));
                    }
                }
            }
        });
}

} // namespace Game
