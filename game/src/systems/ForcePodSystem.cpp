/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ForcePodSystem implementation
*/

#include "systems/ForcePodSystem.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"
#include "components/ForcePod.h"
#include "components/Transform2D.h"

namespace Game {

    void ForcePodSystem::addSystem(RType::Registry &registry)
    {
        registry.addSystem<RType::Transform2D, ForcePod>(
            [&registry](auto &transforms, auto &pods) {
                for (auto [transform, pod] : RType::Zipper(transforms, pods)) {
                    if (pod.ownerId < transforms.size() && transforms[pod.ownerId].has_value()) {
                        const auto &playerPos = transforms[pod.ownerId]->position;
                        transform.position = RType::Vector2{playerPos.x + pod.offsetX, playerPos.y + pod.offsetY};
                    }
                }
            });
    }

} // namespace Game
