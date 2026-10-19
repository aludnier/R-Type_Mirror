/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ParallaxSystem header
*/

#ifndef ENVIRONMENT2D_PARALLAXSYSTEM_H
#define ENVIRONMENT2D_PARALLAXSYSTEM_H

#include "ecs/Registry.h"
#include "components/ParallaxBackground.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"

namespace RType::Environment {

    /**
     * @brief System responsible for infinite parallax background scrolling.
     */
    inline void addParallaxSystem(Registry &registry, float dt) {
        registry.addSystem<Transform2D, ParallaxBackground>(
            [dt](auto &transforms, auto &backgrounds) {
                for (auto [transform, bg] : Zipper(transforms, backgrounds)) {
                    transform.position.x -= bg.scrollSpeed * dt;
                    if (transform.position.x <= bg.resetX) {
                        transform.position.x = bg.startX;
                    }
                }
            });
    }

    class ParallaxSystem {
    public:
        static void addSystem(Registry &registry, float dt) {
            addParallaxSystem(registry, dt);
        }
    };

} // namespace RType::Environment

namespace Game {
    using ParallaxSystem = RType::Environment::ParallaxSystem;
}

#endif // ENVIRONMENT2D_PARALLAXSYSTEM_H
