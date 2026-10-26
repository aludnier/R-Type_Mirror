/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** EnemyAISystem implementation
*/

#include "systems/EnemyAISystem.h"
#include <cmath>
#include "components/Enemy.h"
#include "components/Hitbox.h"
#include "components/Projectile.h"
#include "components/Rigidbody2D.h"
#include "components/Transform2D.h"
#include "ecs/Entity.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"

namespace Game {

void EnemyAISystem::addSystem(RType::Registry &registry, float dt)
{
    registry.addSystem<RType::Transform2D, Enemy>(
        [&registry, dt](auto &transforms, auto &enemies) {
            for (auto [transform, enemy] : RType::Zipper(transforms, enemies)) {
                enemy.elapsedTime += dt;
                // Classic R-Type sinusoidal wave flight path
                transform.position.y = enemy.initialY +
                    std::sin(enemy.elapsedTime * enemy.waveFrequency) *
                        enemy.waveAmplitude;

                // Enemy firing logic
                enemy.shootTimer += dt;
                if (enemy.shootTimer >= enemy.shootInterval) {
                    enemy.shootTimer = 0.0f;

                    // Spawn enemy plasma bullet moving left
                    auto bullet = registry.spawnEntity();
                    registry.addComponent<RType::Transform2D>(bullet,
                        RType::Transform2D({transform.position.x - 20.0f,
                            transform.position.y}));
                    RType::Physics2D::Rigidbody2D body;
                    body.velocity = {-450.0f, 0.0f};
                    registry.emplaceComponent<RType::Physics2D::Rigidbody2D>(
                        bullet, body);
                    registry.addComponent<Projectile>(
                        bullet, Projectile(false, 1, false, 450.0f));
                    registry.addComponent<Hitbox>(
                        bullet, Hitbox(RType::Vector2 {12.0f, 12.0f}));

                    // Note: Feature not available in engine_client: Sprite
                    // component definition is currently anonymous in engine.h,
                    // so game code cannot directly instantiate it without
                    // engine helper or custom Sprite component.
                }
            }
        });
}

} // namespace Game
